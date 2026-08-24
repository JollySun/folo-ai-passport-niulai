// SPDX-License-Identifier: MIT

use core::ffi::{c_char, CStr};
use core::fmt::{self, Write};

#[derive(Clone, Copy)]
#[repr(u32)]
pub enum Level {
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
}

extern "C" {
    fn bsp_log_write(level: u32, tag: *const c_char, message: *const c_char);
}

pub fn write(level: Level, tag: &CStr, message: &CStr) {
    unsafe { bsp_log_write(level as u32, tag.as_ptr(), message.as_ptr()) }
}

pub fn write_args(level: Level, tag: &CStr, arguments: fmt::Arguments<'_>) {
    let mut message = Text::<192>::new();
    let _ = message.write_fmt(arguments);
    write(level, tag, message.as_c_str());
}

pub struct Text<const N: usize> {
    bytes: [u8; N],
    length: usize,
}

impl<const N: usize> Text<N> {
    pub const fn new() -> Self {
        Self {
            bytes: [0; N],
            length: 0,
        }
    }

    pub fn as_c_str(&mut self) -> &CStr {
        let terminator = self.length.min(N.saturating_sub(1));
        self.bytes[terminator] = 0;
        unsafe { CStr::from_bytes_with_nul_unchecked(&self.bytes[..=terminator]) }
    }
}

impl<const N: usize> Default for Text<N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<const N: usize> Write for Text<N> {
    fn write_str(&mut self, value: &str) -> fmt::Result {
        if N == 0 {
            return Err(fmt::Error);
        }
        let available = N - 1 - self.length.min(N - 1);
        let bytes = value.as_bytes();
        let bytes = bytes
            .iter()
            .position(|byte| *byte == 0)
            .map_or(bytes, |nul| &bytes[..nul]);
        let count = available.min(bytes.len());
        self.bytes[self.length..self.length + count].copy_from_slice(&bytes[..count]);
        self.length += count;
        if count == bytes.len() {
            Ok(())
        } else {
            Err(fmt::Error)
        }
    }
}
