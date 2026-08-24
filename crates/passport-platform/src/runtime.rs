// SPDX-License-Identifier: MIT

use crate::error::{check, Error, Result};
use core::cell::UnsafeCell;
use core::ffi::{c_char, c_void, CStr};
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::ptr::NonNull;
use core::sync::atomic::{AtomicPtr, AtomicU8, Ordering};

pub const WAIT_FOREVER: u32 = u32::MAX;
pub type TaskEntry = extern "C" fn(*mut c_void);

extern "C" {
    fn bsp_task_start(
        entry: TaskEntry,
        name: *const c_char,
        stack_size: u32,
        priority: u32,
        argument: *mut c_void,
    ) -> i32;
    fn bsp_delay_ms(milliseconds: u32);
    fn bsp_queue_create(item_size: usize, length: usize) -> *mut c_void;
    fn bsp_queue_delete(queue: *mut c_void);
    fn bsp_queue_overwrite(queue: *mut c_void, item: *const c_void) -> bool;
    fn bsp_queue_receive(queue: *mut c_void, item: *mut c_void, timeout_ms: u32) -> bool;
    fn bsp_mutex_create() -> *mut c_void;
    fn bsp_mutex_delete(mutex: *mut c_void);
    fn bsp_mutex_lock(mutex: *mut c_void, timeout_ms: u32) -> bool;
    fn bsp_mutex_unlock(mutex: *mut c_void);
}

pub fn spawn(entry: TaskEntry, name: &CStr, stack_size: u32, priority: u32) -> Result<()> {
    check(unsafe {
        bsp_task_start(
            entry,
            name.as_ptr(),
            stack_size,
            priority,
            core::ptr::null_mut(),
        )
    })
}

pub fn delay_ms(milliseconds: u32) {
    unsafe { bsp_delay_ms(milliseconds) }
}

pub struct Queue<T: Copy> {
    handle: NonNull<c_void>,
    _item: PhantomData<T>,
}

impl<T: Copy> Queue<T> {
    pub fn new(length: usize) -> Result<Self> {
        let handle = NonNull::new(unsafe { bsp_queue_create(core::mem::size_of::<T>(), length) })
            .ok_or(Error(-1))?;
        Ok(Self {
            handle,
            _item: PhantomData,
        })
    }

    pub fn overwrite(&self, item: T) -> bool {
        unsafe { bsp_queue_overwrite(self.handle.as_ptr(), (&item as *const T).cast()) }
    }

    pub fn receive(&self, timeout_ms: u32) -> Option<T> {
        let mut item = core::mem::MaybeUninit::<T>::uninit();
        if unsafe { bsp_queue_receive(self.handle.as_ptr(), item.as_mut_ptr().cast(), timeout_ms) }
        {
            Some(unsafe { item.assume_init() })
        } else {
            None
        }
    }
}

impl<T: Copy> Drop for Queue<T> {
    fn drop(&mut self) {
        unsafe { bsp_queue_delete(self.handle.as_ptr()) }
    }
}

unsafe impl<T: Copy + Send> Send for Queue<T> {}
unsafe impl<T: Copy + Send> Sync for Queue<T> {}

pub struct StaticCell<T> {
    state: AtomicU8,
    value: UnsafeCell<MaybeUninit<T>>,
}

impl<T> StaticCell<T> {
    pub const fn new() -> Self {
        Self {
            state: AtomicU8::new(0),
            value: UnsafeCell::new(MaybeUninit::uninit()),
        }
    }

    pub fn init(&'static self, value: T) -> Result<&'static T> {
        self.state
            .compare_exchange(0, 1, Ordering::Acquire, Ordering::Relaxed)
            .map_err(|_| Error(-1))?;
        unsafe { (*self.value.get()).write(value) };
        self.state.store(2, Ordering::Release);
        Ok(unsafe { (&*self.value.get()).assume_init_ref() })
    }

    pub fn get(&'static self) -> Option<&'static T> {
        (self.state.load(Ordering::Acquire) == 2)
            .then(|| unsafe { (&*self.value.get()).assume_init_ref() })
    }
}

impl<T> Default for StaticCell<T> {
    fn default() -> Self {
        Self::new()
    }
}

unsafe impl<T: Send + Sync> Sync for StaticCell<T> {}

pub struct StaticMutex<T> {
    handle: AtomicPtr<c_void>,
    value: UnsafeCell<MaybeUninit<T>>,
}

pub struct MutexGuard<'a, T> {
    mutex: &'a StaticMutex<T>,
}

impl<T> StaticMutex<T> {
    pub const fn new() -> Self {
        Self {
            handle: AtomicPtr::new(core::ptr::null_mut()),
            value: UnsafeCell::new(MaybeUninit::uninit()),
        }
    }

    pub fn init(&'static self, value: T) -> Result<()> {
        let handle = NonNull::new(unsafe { bsp_mutex_create() }).ok_or(Error(-1))?;
        let pending = NonNull::<u8>::dangling().as_ptr().cast::<c_void>();
        if self
            .handle
            .compare_exchange(
                core::ptr::null_mut(),
                pending,
                Ordering::Acquire,
                Ordering::Relaxed,
            )
            .is_err()
        {
            unsafe { bsp_mutex_delete(handle.as_ptr()) };
            return Err(Error(-1));
        }
        unsafe { (*self.value.get()).write(value) };
        self.handle.store(handle.as_ptr(), Ordering::Release);
        Ok(())
    }

    pub fn lock(&self, timeout_ms: u32) -> Result<MutexGuard<'_, T>> {
        let handle = self.handle.load(Ordering::Acquire);
        let pending = NonNull::<u8>::dangling().as_ptr().cast::<c_void>();
        if handle.is_null() || handle == pending || !unsafe { bsp_mutex_lock(handle, timeout_ms) } {
            return Err(Error(-1));
        }
        Ok(MutexGuard { mutex: self })
    }
}

impl<T> Default for StaticMutex<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T> core::ops::Deref for MutexGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { (&*self.mutex.value.get()).assume_init_ref() }
    }
}

impl<T> core::ops::DerefMut for MutexGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { (&mut *self.mutex.value.get()).assume_init_mut() }
    }
}

impl<T> Drop for MutexGuard<'_, T> {
    fn drop(&mut self) {
        let handle = self.mutex.handle.load(Ordering::Acquire);
        unsafe { bsp_mutex_unlock(handle) };
    }
}

unsafe impl<T: Send> Sync for StaticMutex<T> {}
