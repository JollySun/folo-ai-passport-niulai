// SPDX-License-Identifier: MIT

use crate::error::{check, Error, Result};
use core::ffi::{c_char, c_void, CStr};
use core::ptr::NonNull;

extern "C" {
    fn bsp_storage_open(label: *const c_char) -> *mut c_void;
    fn bsp_storage_size(storage: *mut c_void) -> usize;
    fn bsp_storage_read(
        storage: *mut c_void,
        offset: usize,
        buffer: *mut c_void,
        bytes: usize,
    ) -> i32;
    fn bsp_storage_write(
        storage: *mut c_void,
        offset: usize,
        buffer: *const c_void,
        bytes: usize,
    ) -> i32;
    fn bsp_storage_erase(storage: *mut c_void, offset: usize, bytes: usize) -> i32;
}

#[derive(Clone, Copy)]
pub struct Storage {
    handle: NonNull<c_void>,
}

unsafe impl Send for Storage {}
unsafe impl Sync for Storage {}

impl Storage {
    pub fn open(label: &CStr) -> Result<Self> {
        let handle = NonNull::new(unsafe { bsp_storage_open(label.as_ptr()) }).ok_or(Error(-1))?;
        Ok(Self { handle })
    }

    pub fn size(&self) -> usize {
        unsafe { bsp_storage_size(self.handle.as_ptr()) }
    }

    pub fn read(&self, offset: usize, buffer: &mut [u8]) -> Result<()> {
        check(unsafe {
            bsp_storage_read(
                self.handle.as_ptr(),
                offset,
                buffer.as_mut_ptr().cast(),
                buffer.len(),
            )
        })
    }

    pub fn write(&self, offset: usize, buffer: &[u8]) -> Result<()> {
        check(unsafe {
            bsp_storage_write(
                self.handle.as_ptr(),
                offset,
                buffer.as_ptr().cast(),
                buffer.len(),
            )
        })
    }

    pub fn erase(&self, offset: usize, bytes: usize) -> Result<()> {
        check(unsafe { bsp_storage_erase(self.handle.as_ptr(), offset, bytes) })
    }
}
