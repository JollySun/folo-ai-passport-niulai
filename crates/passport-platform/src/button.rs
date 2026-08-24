// SPDX-License-Identifier: MIT

use crate::error::{check, Result};
use crate::runtime::StaticCell;
use core::ffi::c_void;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Button {
    Up,
    Down,
    Ok,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Event {
    Press,
    Release,
    Click,
    Double,
    Long,
}

type Callback = fn(Button, Event);
static CALLBACK: StaticCell<Callback> = StaticCell::new();

extern "C" {
    fn bsp_button_init(callback: extern "C" fn(i32, i32, *mut c_void), user: *mut c_void) -> i32;
}

extern "C" fn trampoline(button: i32, event: i32, _user: *mut c_void) {
    let button = match button {
        0 => Button::Up,
        1 => Button::Down,
        _ => Button::Ok,
    };
    let event = match event {
        0 => Event::Press,
        1 => Event::Release,
        2 => Event::Click,
        3 => Event::Double,
        _ => Event::Long,
    };
    if let Some(callback) = CALLBACK.get() {
        callback(button, event);
    }
}

pub fn init(callback: Callback) -> Result<()> {
    CALLBACK.init(callback)?;
    check(unsafe { bsp_button_init(trampoline, core::ptr::null_mut()) })
}
