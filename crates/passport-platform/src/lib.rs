// SPDX-License-Identifier: MIT

#![cfg_attr(target_os = "espidf", no_std)]

pub mod audio;
pub mod battery;
pub mod button;
pub mod display;
pub mod error;
pub mod i2c;
pub mod log;
pub mod runtime;
pub mod storage;
pub mod ui;

pub use error::{Error, Result};

#[cfg(target_os = "espidf")]
mod firmware_support {
    use core::panic::PanicInfo;

    extern "C" {
        fn abort() -> !;
    }

    #[panic_handler]
    fn panic(_info: &PanicInfo<'_>) -> ! {
        unsafe { abort() }
    }

    #[no_mangle]
    extern "C" fn rust_eh_personality() {}
}
