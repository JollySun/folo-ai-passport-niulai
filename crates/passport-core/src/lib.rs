// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![no_std]

mod audio;
mod battery;
mod ffi;

pub use audio::scale_pcm16;
pub use battery::battery_percent_from_voltage;
pub use ffi::{passport_battery_percent_from_voltage, passport_pcm16_scale};
