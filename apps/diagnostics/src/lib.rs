// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![cfg_attr(not(test), no_std)]

#[cfg(not(test))]
use core::panic::PanicInfo;

pub use passport_core as shared_core;

#[cfg(not(test))]
extern "C" {
    fn abort() -> !;
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    unsafe { abort() }
}

#[cfg(not(test))]
#[no_mangle]
extern "C" fn rust_eh_personality() {}

/// Application-level smoke check proving that the selected firmware archive
/// contains and can call the shared Rust core.
#[no_mangle]
extern "C" fn diagnostics_voltage_percent(millivolts: i32) -> i32 {
    passport_core::battery_percent_from_voltage(millivolts)
}

#[cfg(test)]
mod tests {
    use super::diagnostics_voltage_percent;

    #[test]
    fn delegates_voltage_estimation_to_shared_core() {
        assert_eq!(diagnostics_voltage_percent(3750), 37);
        assert_eq!(diagnostics_voltage_percent(4300), 100);
    }
}
