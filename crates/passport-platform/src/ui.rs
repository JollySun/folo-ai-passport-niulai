// SPDX-License-Identifier: MIT

use crate::{Error, Result};
use core::ffi::{c_char, c_void, CStr};
use core::ptr::NonNull;

#[derive(Clone, Copy)]
pub struct Object(NonNull<c_void>);

#[derive(Clone, Copy)]
pub struct ImageSource(NonNull<c_void>);

#[derive(Clone, Copy)]
pub struct Font(*const c_void);

#[derive(Clone, Copy)]
#[repr(i32)]
pub enum TextAlign {
    Left = 0,
    Center = 1,
    Right = 2,
}

pub type TimerCallback = extern "C" fn();

extern "C" {
    fn bsp_ui_screen_create() -> *mut c_void;
    fn bsp_ui_panel_create(parent: *mut c_void) -> *mut c_void;
    fn bsp_ui_label_create(parent: *mut c_void, font: *const c_void, color: u32) -> *mut c_void;
    fn bsp_ui_image_create(parent: *mut c_void) -> *mut c_void;
    fn bsp_ui_image_source_create(pixels: *const c_void, width: u16, height: u16) -> *mut c_void;
    fn bsp_ui_screen_load(screen: *mut c_void);
    fn bsp_ui_timer_create(callback: TimerCallback, period_ms: u32);
    fn bsp_ui_set_pos(object: *mut c_void, x: i32, y: i32);
    fn bsp_ui_set_size(object: *mut c_void, width: i32, height: i32);
    fn bsp_ui_set_width(object: *mut c_void, width: i32);
    fn bsp_ui_set_hidden(object: *mut c_void, hidden: bool);
    fn bsp_ui_set_scrollable(object: *mut c_void, scrollable: bool);
    fn bsp_ui_center(object: *mut c_void);
    fn bsp_ui_fade_in(object: *mut c_void, duration_ms: u32);
    fn bsp_ui_set_background(object: *mut c_void, color: u32, opacity: u8);
    fn bsp_ui_set_border(object: *mut c_void, color: u32, width: i32);
    fn bsp_ui_set_radius(object: *mut c_void, radius: i32);
    fn bsp_ui_set_circle(object: *mut c_void);
    fn bsp_ui_set_padding(object: *mut c_void, padding: i32);
    fn bsp_ui_set_shadow(object: *mut c_void, color: u32, opacity: u8, width: i32, offset_y: i32);
    fn bsp_ui_label_set_layout(
        label: *mut c_void,
        font: *const c_void,
        color: u32,
        align: TextAlign,
        line_space: i32,
    );
    fn bsp_ui_label_set_text(label: *mut c_void, text: *const c_char);
    fn bsp_ui_image_set_source(image: *mut c_void, source: *const c_void);
}

impl Font {
    /// # Safety
    /// `symbol` must point to a live LVGL font descriptor for the firmware lifetime.
    pub const unsafe fn from_symbol(symbol: *const c_void) -> Self {
        Self(symbol)
    }
}

impl ImageSource {
    pub fn rgb565(pixels: &'static [u8], width: u16, height: u16) -> Result<Self> {
        let expected = usize::from(width) * usize::from(height) * 2;
        if pixels.len() != expected {
            return Err(Error(-1));
        }
        NonNull::new(unsafe { bsp_ui_image_source_create(pixels.as_ptr().cast(), width, height) })
            .map(Self)
            .ok_or(Error(-1))
    }
}

impl Object {
    pub fn screen() -> Result<Self> {
        NonNull::new(unsafe { bsp_ui_screen_create() })
            .map(Self)
            .ok_or(Error(-1))
    }

    pub fn panel(parent: Self) -> Result<Self> {
        NonNull::new(unsafe { bsp_ui_panel_create(parent.0.as_ptr()) })
            .map(Self)
            .ok_or(Error(-1))
    }

    pub fn label(parent: Self, font: Font, color: u32) -> Result<Self> {
        NonNull::new(unsafe { bsp_ui_label_create(parent.0.as_ptr(), font.0, color) })
            .map(Self)
            .ok_or(Error(-1))
    }

    pub fn image(parent: Self) -> Result<Self> {
        NonNull::new(unsafe { bsp_ui_image_create(parent.0.as_ptr()) })
            .map(Self)
            .ok_or(Error(-1))
    }

    pub fn load(self) {
        unsafe { bsp_ui_screen_load(self.0.as_ptr()) }
    }
    pub fn set_pos(self, x: i32, y: i32) {
        unsafe { bsp_ui_set_pos(self.0.as_ptr(), x, y) }
    }
    pub fn set_size(self, width: i32, height: i32) {
        unsafe { bsp_ui_set_size(self.0.as_ptr(), width, height) }
    }
    pub fn set_width(self, width: i32) {
        unsafe { bsp_ui_set_width(self.0.as_ptr(), width) }
    }
    pub fn set_hidden(self, hidden: bool) {
        unsafe { bsp_ui_set_hidden(self.0.as_ptr(), hidden) }
    }
    pub fn set_scrollable(self, scrollable: bool) {
        unsafe { bsp_ui_set_scrollable(self.0.as_ptr(), scrollable) }
    }
    pub fn center(self) {
        unsafe { bsp_ui_center(self.0.as_ptr()) }
    }
    pub fn fade_in(self, duration_ms: u32) {
        unsafe { bsp_ui_fade_in(self.0.as_ptr(), duration_ms) }
    }
    pub fn set_background(self, color: u32, opacity: u8) {
        unsafe { bsp_ui_set_background(self.0.as_ptr(), color, opacity) }
    }
    pub fn set_border(self, color: u32, width: i32) {
        unsafe { bsp_ui_set_border(self.0.as_ptr(), color, width) }
    }
    pub fn set_radius(self, radius: i32) {
        unsafe { bsp_ui_set_radius(self.0.as_ptr(), radius) }
    }
    pub fn set_circle(self) {
        unsafe { bsp_ui_set_circle(self.0.as_ptr()) }
    }
    pub fn set_padding(self, padding: i32) {
        unsafe { bsp_ui_set_padding(self.0.as_ptr(), padding) }
    }
    pub fn set_shadow(self, color: u32, opacity: u8, width: i32, offset_y: i32) {
        unsafe { bsp_ui_set_shadow(self.0.as_ptr(), color, opacity, width, offset_y) }
    }
    pub fn set_label_layout(self, font: Font, color: u32, align: TextAlign, line_space: i32) {
        unsafe { bsp_ui_label_set_layout(self.0.as_ptr(), font.0, color, align, line_space) }
    }
    pub fn set_text(self, text: &CStr) {
        unsafe { bsp_ui_label_set_text(self.0.as_ptr(), text.as_ptr()) }
    }
    pub fn set_image_source(self, source: ImageSource) {
        unsafe { bsp_ui_image_set_source(self.0.as_ptr(), source.0.as_ptr()) }
    }
}

pub fn create_timer(callback: TimerCallback, period_ms: u32) {
    unsafe { bsp_ui_timer_create(callback, period_ms) }
}

unsafe impl Send for Object {}
unsafe impl Send for ImageSource {}
unsafe impl Send for Font {}
