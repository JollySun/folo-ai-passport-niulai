// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![cfg_attr(not(test), no_std)]

#[cfg(not(test))]
use core::panic::PanicInfo;
use core::slice;
use niulai_core::{Action, Input, Model, Page};
use passport_core::{battery_percent_from_voltage, scale_pcm16};

#[repr(C)]
struct CModel {
    page: i32,
    return_page: i32,
    volume: u8,
}

#[cfg(not(test))]
extern "C" {
    fn abort() -> !;
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    unsafe { abort() }
}

// `core` can retain this reference even with panic=abort. A Rust executable
// normally supplies it, but the final linker is C for this transition crate.
#[cfg(not(test))]
#[no_mangle]
extern "C" fn rust_eh_personality() {}

fn page_from_c(value: i32) -> Page {
    match value {
        1 => Page::Calf,
        2 => Page::Mother,
        3 => Page::Settings,
        _ => Page::Home,
    }
}

const fn page_to_c(value: Page) -> i32 {
    match value {
        Page::Home => 0,
        Page::Calf => 1,
        Page::Mother => 2,
        Page::Settings => 3,
    }
}

fn input_from_c(value: i32) -> Input {
    match value {
        1 => Input::UpClick,
        2 => Input::DownClick,
        3 => Input::OkClick,
        4 => Input::OkLong,
        _ => Input::None,
    }
}

const fn action_to_c(value: Action) -> i32 {
    match value {
        Action::None => 0,
        Action::PlayMama => 1,
        Action::PlayNiulai => 2,
        Action::StopAudio => 3,
        Action::VolumeChanged => 4,
    }
}

#[no_mangle]
unsafe extern "C" fn niulai_model_init(model: *mut CModel) {
    let Some(model) = model.as_mut() else {
        return;
    };
    let initial = Model::default();
    model.page = page_to_c(initial.page());
    model.return_page = page_to_c(initial.return_page());
    model.volume = initial.volume();
}

#[no_mangle]
unsafe extern "C" fn niulai_model_apply(model: *mut CModel, input: i32) -> i32 {
    let Some(model) = model.as_mut() else {
        return action_to_c(Action::None);
    };
    let mut state = Model::from_parts(
        page_from_c(model.page),
        page_from_c(model.return_page),
        model.volume,
    );
    let action = state.apply(input_from_c(input));
    model.page = page_to_c(state.page());
    model.return_page = page_to_c(state.return_page());
    model.volume = state.volume();
    action_to_c(action)
}

#[no_mangle]
unsafe extern "C" fn niulai_scale_pcm16(
    output: *mut i16,
    input: *const i16,
    sample_count: usize,
    volume: u8,
) {
    if sample_count == 0 {
        return;
    }
    if output.is_null() || input.is_null() {
        return;
    }
    let output = slice::from_raw_parts_mut(output, sample_count);
    let input = slice::from_raw_parts(input, sample_count);
    scale_pcm16(output, input, volume);
}

#[no_mangle]
extern "C" fn bsp_battery_percent_from_voltage(millivolts: i32) -> i32 {
    battery_percent_from_voltage(millivolts)
}
