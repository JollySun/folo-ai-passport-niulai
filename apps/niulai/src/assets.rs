// SPDX-License-Identifier: MIT

use crate::voice_store::Slot;
use core::ffi::c_void;
use passport_platform::ui::{Font, ImageSource};
use passport_platform::Result;

const FRAME_WIDTH: u16 = 240;
const HOME_HEIGHT: u16 = 320;
const ACTIVE_HEIGHT: u16 = 180;

extern "C" {
    #[link_name = "_binary_home_rgb565_start"]
    static HOME_START: u8;
    #[link_name = "_binary_calf_1_rgb565_start"]
    static CALF_1_START: u8;
    #[link_name = "_binary_calf_2_rgb565_start"]
    static CALF_2_START: u8;
    #[link_name = "_binary_mother_1_rgb565_start"]
    static MOTHER_1_START: u8;
    #[link_name = "_binary_mother_2_open_rgb565_start"]
    static MOTHER_2_START: u8;
    #[link_name = "_binary_mama_pcm_start"]
    static MAMA_START: u8;
    #[link_name = "_binary_mama_pcm_end"]
    static MAMA_END: u8;
    #[link_name = "_binary_niulai_pcm_start"]
    static NIULAI_START: u8;
    #[link_name = "_binary_niulai_pcm_end"]
    static NIULAI_END: u8;

    static niulai_font_12: u8;
    static niulai_font_16: u8;
    static niulai_font_22: u8;
}

#[derive(Clone, Copy)]
pub struct Assets {
    pub home: ImageSource,
    pub calf_1: ImageSource,
    pub calf_2: ImageSource,
    pub mother_1: ImageSource,
    pub mother_2: ImageSource,
}

impl Assets {
    pub fn load() -> Result<Self> {
        Ok(Self {
            home: image(core::ptr::addr_of!(HOME_START), FRAME_WIDTH, HOME_HEIGHT)?,
            calf_1: image(
                core::ptr::addr_of!(CALF_1_START),
                FRAME_WIDTH,
                ACTIVE_HEIGHT,
            )?,
            calf_2: image(
                core::ptr::addr_of!(CALF_2_START),
                FRAME_WIDTH,
                ACTIVE_HEIGHT,
            )?,
            mother_1: image(
                core::ptr::addr_of!(MOTHER_1_START),
                FRAME_WIDTH,
                ACTIVE_HEIGHT,
            )?,
            mother_2: image(
                core::ptr::addr_of!(MOTHER_2_START),
                FRAME_WIDTH,
                ACTIVE_HEIGHT,
            )?,
        })
    }
}

pub fn font_12() -> Font {
    unsafe { Font::from_symbol(core::ptr::addr_of!(niulai_font_12).cast::<c_void>()) }
}

pub fn font_16() -> Font {
    unsafe { Font::from_symbol(core::ptr::addr_of!(niulai_font_16).cast::<c_void>()) }
}

pub fn font_22() -> Font {
    unsafe { Font::from_symbol(core::ptr::addr_of!(niulai_font_22).cast::<c_void>()) }
}

pub fn default_voice(slot: Slot) -> &'static [u8] {
    let (start, end) = match slot {
        Slot::Calf => (
            core::ptr::addr_of!(MAMA_START),
            core::ptr::addr_of!(MAMA_END),
        ),
        Slot::Mother => (
            core::ptr::addr_of!(NIULAI_START),
            core::ptr::addr_of!(NIULAI_END),
        ),
    };
    let length = end as usize - start as usize;
    unsafe { core::slice::from_raw_parts(start, length) }
}

fn image(start: *const u8, width: u16, height: u16) -> Result<ImageSource> {
    let bytes = usize::from(width) * usize::from(height) * 2;
    let pixels = unsafe { core::slice::from_raw_parts(start, bytes) };
    ImageSource::rgb565(pixels, width, height)
}
