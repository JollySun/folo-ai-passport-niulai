// SPDX-License-Identifier: MIT

use crate::assets::{font_12, font_16, font_22, Assets};
use crate::{Page, RecordState};
use core::fmt::Write;
use passport_platform::log::Text;
use passport_platform::ui::{self, Object, TextAlign};
use passport_platform::Result;

const ANIMATION_PERIOD_MS: u32 = 120;
const VOLUME_TRACK_WIDTH: i32 = 184;

const COLOR_BG: u32 = 0x071A1E;
const COLOR_SURFACE: u32 = 0x10292D;
const COLOR_SURFACE_ALT: u32 = 0x18383A;
const COLOR_TEXT: u32 = 0xF7F2E8;
const COLOR_MUTED: u32 = 0xA7C2BE;
const COLOR_ACCENT: u32 = 0xE7A35B;
const COLOR_EDGE: u32 = 0x3C7772;
const COLOR_TRACK: u32 = 0x294A4B;

#[derive(Clone, Copy)]
pub struct ViewState {
    pub page: Page,
    pub volume: u8,
    pub audio_ok: bool,
    pub battery_soc: Option<u8>,
    pub record_state: RecordState,
    pub record_page: bool,
    pub custom_voice_count: u8,
    pub storage_available: bool,
}

pub struct Ui {
    assets: Assets,
    screen: Object,
    image: Object,
    panel: Object,
    title: Object,
    phrase: Object,
    hint: Object,
    battery_body: Object,
    battery_fill: Object,
    battery_cap: Object,
    battery_value: Object,
    record_dot: Object,
    volume_label: Object,
    volume_value: Object,
    volume_track: Object,
    volume_fill: Object,
    volume_help: Object,
    settings_divider: Object,
    voice_label: Object,
    reset_panel: Object,
    reset_label: Object,
    rendered_page: Option<Page>,
}

impl Ui {
    pub fn build(assets: Assets) -> Result<Self> {
        let screen = Object::screen()?;
        screen.set_scrollable(false);
        screen.set_border(COLOR_BG, 0);
        screen.set_padding(0);
        screen.set_background(COLOR_BG, 255);

        let image = Object::image(screen)?;
        let panel = Object::panel(screen)?;
        panel.set_scrollable(false);
        panel.set_padding(0);
        panel.set_shadow(COLOR_BG, 102, 10, 3);

        let title = Object::label(screen, font_22(), COLOR_TEXT)?;
        let phrase = Object::label(screen, font_16(), COLOR_TEXT)?;
        let hint = Object::label(screen, font_12(), COLOR_MUTED)?;

        let battery_body = Object::panel(screen)?;
        battery_body.set_pos(10, 10);
        battery_body.set_size(40, 18);
        battery_body.set_scrollable(false);
        battery_body.set_padding(0);
        battery_body.set_radius(6);
        battery_body.set_border(COLOR_EDGE, 1);
        battery_body.set_background(COLOR_BG, 204);

        let battery_fill = Object::panel(battery_body)?;
        battery_fill.set_pos(2, 2);
        battery_fill.set_size(0, 12);
        battery_fill.set_scrollable(false);
        battery_fill.set_padding(0);
        battery_fill.set_border(COLOR_BG, 0);
        battery_fill.set_radius(4);
        battery_fill.set_background(COLOR_ACCENT, 255);

        let battery_value = Object::label(battery_body, font_12(), COLOR_TEXT)?;
        battery_value.set_width(38);
        battery_value.set_label_layout(font_12(), COLOR_TEXT, TextAlign::Center, 0);
        battery_value.set_background(COLOR_BG, 0);
        battery_value.center();

        let battery_cap = Object::panel(screen)?;
        battery_cap.set_pos(51, 15);
        battery_cap.set_size(3, 8);
        battery_cap.set_padding(0);
        battery_cap.set_border(COLOR_BG, 0);
        battery_cap.set_radius(2);
        battery_cap.set_background(COLOR_EDGE, 255);

        let record_dot = Object::panel(screen)?;
        record_dot.set_pos(214, 18);
        record_dot.set_size(8, 8);
        record_dot.set_scrollable(false);
        record_dot.set_padding(0);
        record_dot.set_border(COLOR_BG, 0);
        record_dot.set_circle();
        record_dot.set_background(COLOR_ACCENT, 255);

        let volume_label = Object::label(screen, font_16(), COLOR_TEXT)?;
        set_label_layout(
            volume_label,
            font_16(),
            COLOR_TEXT,
            28,
            66,
            100,
            24,
            TextAlign::Left,
        );
        let volume_value = Object::label(screen, font_22(), COLOR_ACCENT)?;
        set_label_layout(
            volume_value,
            font_22(),
            COLOR_ACCENT,
            156,
            61,
            56,
            30,
            TextAlign::Right,
        );

        let volume_track = Object::panel(screen)?;
        volume_track.set_pos(28, 99);
        volume_track.set_size(VOLUME_TRACK_WIDTH, 10);
        volume_track.set_scrollable(false);
        volume_track.set_padding(0);
        volume_track.set_border(COLOR_BG, 0);
        volume_track.set_radius(5);
        volume_track.set_background(COLOR_TRACK, 255);

        let volume_fill = Object::panel(volume_track)?;
        volume_fill.set_pos(0, 0);
        volume_fill.set_size(0, 10);
        volume_fill.set_scrollable(false);
        volume_fill.set_padding(0);
        volume_fill.set_border(COLOR_BG, 0);
        volume_fill.set_radius(5);
        volume_fill.set_background(COLOR_ACCENT, 255);

        let volume_help = Object::label(screen, font_12(), COLOR_MUTED)?;
        set_label_layout(
            volume_help,
            font_12(),
            COLOR_MUTED,
            28,
            116,
            184,
            18,
            TextAlign::Left,
        );

        let settings_divider = Object::panel(screen)?;
        settings_divider.set_pos(28, 141);
        settings_divider.set_size(184, 1);
        settings_divider.set_padding(0);
        settings_divider.set_border(COLOR_BG, 0);
        settings_divider.set_background(COLOR_EDGE, 102);

        let voice_label = Object::label(screen, font_12(), COLOR_MUTED)?;
        set_label_layout(
            voice_label,
            font_12(),
            COLOR_MUTED,
            28,
            153,
            184,
            18,
            TextAlign::Left,
        );

        let reset_panel = Object::panel(screen)?;
        reset_panel.set_pos(24, 212);
        reset_panel.set_size(192, 42);
        reset_panel.set_scrollable(false);
        reset_panel.set_padding(0);
        style_surface(reset_panel, COLOR_SURFACE_ALT, COLOR_ACCENT, 255, 11, 1);
        let reset_label = Object::label(reset_panel, font_12(), COLOR_TEXT)?;
        reset_label.set_size(176, 18);
        reset_label.set_label_layout(font_12(), COLOR_TEXT, TextAlign::Center, 0);
        reset_label.center();

        Ok(Self {
            assets,
            screen,
            image,
            panel,
            title,
            phrase,
            hint,
            battery_body,
            battery_fill,
            battery_cap,
            battery_value,
            record_dot,
            volume_label,
            volume_value,
            volume_track,
            volume_fill,
            volume_help,
            settings_divider,
            voice_label,
            reset_panel,
            reset_label,
            rendered_page: None,
        })
    }

    pub fn load(&self) {
        ui::create_timer(crate::app::animation_tick, ANIMATION_PERIOD_MS);
        self.screen.load();
    }

    pub fn render(&mut self, state: ViewState) {
        let page_changed = self.rendered_page != Some(state.page);
        match state.page {
            Page::Home => self.show_home(state),
            Page::Settings => self.show_settings(state),
            Page::Calf | Page::Mother => self.show_active(state),
        }
        if page_changed {
            self.panel.fade_in(160);
            if state.page != Page::Settings {
                self.image.fade_in(160);
            }
            self.rendered_page = Some(state.page);
        }
    }

    pub fn update_battery(&self, percent: Option<u8>) {
        let Some(percent) = percent else {
            self.battery_fill.set_width(0);
            self.battery_value.set_text(c"");
            self.battery_body.set_border(COLOR_EDGE, 1);
            self.battery_body.set_background(COLOR_BG, 153);
            return;
        };
        let fill_width = (i32::from(percent.min(100)) * 34 + 99) / 100;
        self.battery_fill.set_width(fill_width);
        self.battery_fill.set_background(COLOR_ACCENT, 255);
        self.battery_body.set_border(COLOR_EDGE, 1);
        self.battery_body.set_background(COLOR_BG, 204);
        let mut text = Text::<8>::new();
        let _ = write!(text, "{}", percent.min(100));
        self.battery_value.set_text(text.as_c_str());
    }

    pub fn set_animation_frame(&self, page: Page, second: bool) {
        let source = match (page, second) {
            (Page::Calf, false) => self.assets.calf_1,
            (Page::Calf, true) => self.assets.calf_2,
            (_, false) => self.assets.mother_1,
            (_, true) => self.assets.mother_2,
        };
        self.image.set_image_source(source);
    }

    pub fn set_record_pulse(&self, on: bool) {
        self.record_dot
            .set_background(COLOR_ACCENT, if on { 255 } else { 77 });
    }

    fn show_home(&self, state: ViewState) {
        self.image.set_hidden(false);
        self.record_dot.set_hidden(true);
        self.set_settings_hidden(true);
        self.set_battery_hidden(false);
        self.image.set_pos(0, 0);
        self.image.set_image_source(self.assets.home);
        self.screen.set_background(COLOR_BG, 255);
        self.panel.set_pos(8, 188);
        self.panel.set_size(224, 124);
        style_surface(self.panel, COLOR_SURFACE, COLOR_EDGE, 230, 16, 1);
        set_label_layout(
            self.title,
            font_22(),
            COLOR_TEXT,
            20,
            197,
            200,
            28,
            TextAlign::Left,
        );
        set_label_layout(
            self.phrase,
            font_16(),
            COLOR_TEXT,
            20,
            229,
            200,
            23,
            TextAlign::Left,
        );
        set_label_layout(
            self.hint,
            font_12(),
            COLOR_MUTED,
            20,
            260,
            200,
            42,
            TextAlign::Left,
        );
        self.title.set_text(c"牛来");
        self.phrase.set_text(c"上键  牛来   ·   下键  妈妈");
        self.hint.set_text(if state.audio_ok {
            c"长按对应按键可录音\n确认键进入设置"
        } else {
            c"声音暂不可用\n确认键进入设置"
        });
        self.update_battery(state.battery_soc);
    }

    fn show_active(&self, state: ViewState) {
        let calf = state.page == Page::Calf;
        self.image.set_hidden(false);
        self.set_settings_hidden(true);
        self.set_battery_hidden(true);
        self.screen.set_background(COLOR_BG, 255);
        self.image.set_pos(0, 42);
        self.set_animation_frame(state.page, false);
        self.panel.set_pos(8, 224);
        self.panel.set_size(224, 88);
        style_surface(self.panel, COLOR_SURFACE, COLOR_ACCENT, 255, 16, 1);
        set_label_layout(
            self.title,
            font_22(),
            COLOR_TEXT,
            16,
            10,
            208,
            28,
            TextAlign::Left,
        );
        set_label_layout(
            self.phrase,
            font_22(),
            COLOR_TEXT,
            20,
            241,
            200,
            32,
            TextAlign::Center,
        );
        set_label_layout(
            self.hint,
            font_12(),
            COLOR_MUTED,
            20,
            286,
            200,
            18,
            TextAlign::Center,
        );
        self.title.set_text(if calf { c"牛来" } else { c"妈妈" });
        self.phrase.set_text(if state.record_page {
            match state.record_state {
                RecordState::Preparing => c"准备录音",
                RecordState::Active => c"正在录音",
                RecordState::Saving => c"正在保存",
                RecordState::Saved => c"录音已保存",
                RecordState::Failed => c"录音失败",
                _ => {
                    if calf {
                        c"妈妈～～"
                    } else {
                        c"牛来！"
                    }
                }
            }
        } else if calf {
            c"妈妈～～"
        } else {
            c"牛来！"
        });
        let recording = state.record_page
            && matches!(
                state.record_state,
                RecordState::Preparing | RecordState::Active | RecordState::Saving
            );
        self.hint
            .set_text(if recording && state.record_state != RecordState::Saving {
                c"松开按键保存"
            } else {
                c"长按确认键返回首页"
            });
        self.record_dot.set_hidden(!recording);
        if recording {
            self.set_record_pulse(true);
        }
    }

    fn show_settings(&self, state: ViewState) {
        self.image.set_hidden(true);
        self.record_dot.set_hidden(true);
        self.set_settings_hidden(false);
        self.set_battery_hidden(true);
        self.screen.set_background(COLOR_BG, 255);
        self.panel.set_pos(12, 50);
        self.panel.set_size(216, 246);
        style_surface(self.panel, COLOR_SURFACE, COLOR_EDGE, 255, 18, 1);
        set_label_layout(
            self.title,
            font_22(),
            COLOR_TEXT,
            18,
            14,
            204,
            30,
            TextAlign::Left,
        );
        set_label_layout(
            self.phrase,
            font_16(),
            COLOR_TEXT,
            28,
            176,
            184,
            24,
            TextAlign::Left,
        );
        set_label_layout(
            self.hint,
            font_12(),
            COLOR_MUTED,
            28,
            270,
            184,
            18,
            TextAlign::Center,
        );
        self.title.set_text(c"设置");
        self.volume_label.set_text(c"音量");
        let mut volume = Text::<8>::new();
        let _ = write!(volume, "{}%", state.volume);
        self.volume_value.set_text(volume.as_c_str());
        self.volume_fill
            .set_width((i32::from(state.volume) * VOLUME_TRACK_WIDTH + 99) / 100);
        self.volume_help.set_text(c"上键增加  ·  下键减少");
        self.voice_label.set_text(c"自定义声音");
        if state.record_state == RecordState::Preparing {
            self.phrase.set_text(c"正在恢复");
        } else if state.record_state == RecordState::Failed {
            self.phrase.set_text(c"恢复失败");
        } else if !state.storage_available {
            self.phrase.set_text(c"录音存储不可用");
        } else if state.record_state == RecordState::ResetDone {
            self.phrase.set_text(c"已恢复默认声音");
        } else if state.custom_voice_count > 0 {
            let mut text = Text::<24>::new();
            let _ = write!(text, "已保存 {} 个", state.custom_voice_count);
            self.phrase.set_text(text.as_c_str());
        } else {
            self.phrase.set_text(c"使用默认声音");
        }
        self.reset_label.set_text(c"双击确认键恢复默认声音");
        self.hint.set_text(c"确认键返回");
    }

    fn set_settings_hidden(&self, hidden: bool) {
        for object in [
            self.volume_label,
            self.volume_value,
            self.volume_track,
            self.volume_help,
            self.settings_divider,
            self.voice_label,
            self.reset_panel,
        ] {
            object.set_hidden(hidden);
        }
    }

    fn set_battery_hidden(&self, hidden: bool) {
        self.battery_body.set_hidden(hidden);
        self.battery_cap.set_hidden(hidden);
    }
}

fn set_label_layout(
    label: Object,
    font: passport_platform::ui::Font,
    color: u32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    align: TextAlign,
) {
    label.set_pos(x, y);
    label.set_size(width, height);
    label.set_label_layout(font, color, align, 3);
}

fn style_surface(
    object: Object,
    background: u32,
    border: u32,
    opacity: u8,
    radius: i32,
    width: i32,
) {
    object.set_background(background, opacity);
    object.set_border(border, width);
    object.set_radius(radius);
}
