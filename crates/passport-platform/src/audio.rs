// SPDX-License-Identifier: MIT

use crate::error::{check, Result};

extern "C" {
    fn bsp_audio_init() -> i32;
    fn bsp_audio_set_format(hz: u32, bits: u8, channels: u8) -> i32;
    fn bsp_audio_write(pcm: *const u8, bytes: usize) -> i32;
    fn bsp_audio_read(pcm: *mut u8, bytes: usize) -> i32;
    fn bsp_audio_set_volume(percent: u8);
}

pub struct Audio;

impl Audio {
    pub fn init() -> Result<Self> {
        check(unsafe { bsp_audio_init() })?;
        Ok(Self)
    }

    pub fn set_format(&mut self, hz: u32, bits: u8, channels: u8) -> Result<()> {
        check(unsafe { bsp_audio_set_format(hz, bits, channels) })
    }

    pub fn write(&mut self, pcm: &[i16]) -> Result<()> {
        check(unsafe { bsp_audio_write(pcm.as_ptr().cast(), core::mem::size_of_val(pcm)) })
    }

    pub fn read(&mut self, pcm: &mut [i16]) -> Result<()> {
        check(unsafe { bsp_audio_read(pcm.as_mut_ptr().cast(), core::mem::size_of_val(pcm)) })
    }

    pub fn set_volume(&mut self, percent: u8) {
        unsafe { bsp_audio_set_volume(percent.min(100)) }
    }
}
