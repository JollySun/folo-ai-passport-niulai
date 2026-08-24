// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![no_std]

mod audio;
mod battery;

pub use audio::scale_pcm16;
pub use battery::battery_percent_from_voltage;
