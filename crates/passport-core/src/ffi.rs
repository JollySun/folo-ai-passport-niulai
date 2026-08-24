// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

use core::slice;

use crate::{battery_percent_from_voltage, scale_pcm16};

/// C interface for the shared PCM scaler.
///
/// Null buffers are ignored. A zero sample count never dereferences either
/// pointer, matching the slice interface's empty-input behavior.
///
/// # Safety
///
/// For a nonzero sample count, `output` must be writable and `input` readable
/// for `sample_count` contiguous `i16` values. The regions may be identical,
/// but otherwise must not overlap.
#[no_mangle]
pub unsafe extern "C" fn passport_pcm16_scale(
    output: *mut i16,
    input: *const i16,
    sample_count: usize,
    volume: u8,
) {
    if sample_count == 0 || output.is_null() || input.is_null() {
        return;
    }
    let output = slice::from_raw_parts_mut(output, sample_count);
    let input = slice::from_raw_parts(input, sample_count);
    scale_pcm16(output, input, volume);
}

/// C interface for the shared single-cell voltage estimate.
#[no_mangle]
pub extern "C" fn passport_battery_percent_from_voltage(millivolts: i32) -> i32 {
    battery_percent_from_voltage(millivolts)
}
