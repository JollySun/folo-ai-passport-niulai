// SPDX-License-Identifier: MIT

use crate::error::{check, Error, Result};
use core::ffi::c_void;
use core::ptr::NonNull;

extern "C" {
    fn bsp_display_init() -> i32;
    fn bsp_display_backlight(percent: u8);
    fn bsp_lvgl_init() -> *mut c_void;
    fn bsp_lvgl_lock(timeout_ms: i32) -> bool;
    fn bsp_lvgl_unlock();
}

pub struct Display;

pub struct UiGuard;

impl Display {
    pub fn init() -> Result<Self> {
        check(unsafe { bsp_display_init() })?;
        NonNull::new(unsafe { bsp_lvgl_init() }).ok_or(Error(-1))?;
        Ok(Self)
    }

    pub fn set_backlight(&mut self, percent: u8) {
        unsafe { bsp_display_backlight(percent.min(100)) }
    }
}

pub fn lock(timeout_ms: i32) -> Result<UiGuard> {
    if unsafe { bsp_lvgl_lock(timeout_ms) } {
        Ok(UiGuard)
    } else {
        Err(Error(-1))
    }
}

impl Drop for UiGuard {
    fn drop(&mut self) {
        unsafe { bsp_lvgl_unlock() }
    }
}
