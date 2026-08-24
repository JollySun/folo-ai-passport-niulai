// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![no_std]

pub const DEFAULT_VOLUME: u8 = 85;
pub const VOLUME_STEP: u8 = 5;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Page {
    Home,
    Calf,
    Mother,
    Settings,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Input {
    None,
    UpClick,
    DownClick,
    OkClick,
    OkLong,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Action {
    None,
    PlayMama,
    PlayNiulai,
    StopAudio,
    VolumeChanged,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Model {
    page: Page,
    return_page: Page,
    volume: u8,
}

impl Default for Model {
    fn default() -> Self {
        Self {
            page: Page::Home,
            return_page: Page::Home,
            volume: DEFAULT_VOLUME,
        }
    }
}

impl Model {
    pub const fn from_parts(page: Page, return_page: Page, volume: u8) -> Self {
        Self {
            page,
            return_page,
            volume,
        }
    }

    pub const fn page(&self) -> Page {
        self.page
    }

    pub const fn return_page(&self) -> Page {
        self.return_page
    }

    pub const fn volume(&self) -> u8 {
        self.volume
    }

    pub fn apply(&mut self, input: Input) -> Action {
        if input == Input::OkLong {
            self.page = Page::Home;
            self.return_page = Page::Home;
            return Action::StopAudio;
        }

        if input == Input::OkClick {
            if self.page == Page::Settings {
                self.page = self.return_page;
            } else {
                self.return_page = self.page;
                self.page = Page::Settings;
            }
            return Action::None;
        }

        if self.page == Page::Settings {
            return match input {
                Input::UpClick if self.volume < 100 => {
                    self.volume = self.volume.saturating_add(VOLUME_STEP).min(100);
                    Action::VolumeChanged
                }
                Input::DownClick if self.volume > 0 => {
                    self.volume = self.volume.saturating_sub(VOLUME_STEP);
                    Action::VolumeChanged
                }
                _ => Action::None,
            };
        }

        match input {
            Input::UpClick => {
                self.page = Page::Calf;
                Action::PlayMama
            }
            Input::DownClick => {
                self.page = Page::Mother;
                Action::PlayNiulai
            }
            _ => Action::None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{Action, Input, Model, Page, DEFAULT_VOLUME};

    #[test]
    fn settings_clamp_volume_and_return_to_previous_page() {
        let mut model = Model::default();
        assert_eq!(model.page(), Page::Home);
        assert_eq!(model.volume(), DEFAULT_VOLUME);

        assert_eq!(model.apply(Input::OkClick), Action::None);
        assert_eq!(model.page(), Page::Settings);
        assert_eq!(model.apply(Input::UpClick), Action::VolumeChanged);
        assert_eq!(model.volume(), 90);
        assert_eq!(model.apply(Input::DownClick), Action::VolumeChanged);
        assert_eq!(model.volume(), DEFAULT_VOLUME);

        for _ in 0..10 {
            model.apply(Input::UpClick);
        }
        assert_eq!(model.volume(), 100);
        assert_eq!(model.apply(Input::UpClick), Action::None);

        for _ in 0..20 {
            model.apply(Input::DownClick);
        }
        assert_eq!(model.volume(), 0);
        assert_eq!(model.apply(Input::DownClick), Action::None);
        assert_eq!(model.apply(Input::OkClick), Action::None);
        assert_eq!(model.page(), Page::Home);
    }

    #[test]
    fn character_inputs_select_page_and_audio() {
        let mut model = Model::default();
        assert_eq!(model.apply(Input::UpClick), Action::PlayMama);
        assert_eq!(model.page(), Page::Calf);
        assert_eq!(model.apply(Input::OkClick), Action::None);
        assert_eq!(model.page(), Page::Settings);
        assert_eq!(model.apply(Input::OkClick), Action::None);
        assert_eq!(model.page(), Page::Calf);

        assert_eq!(model.apply(Input::DownClick), Action::PlayNiulai);
        assert_eq!(model.page(), Page::Mother);
        assert_eq!(model.apply(Input::OkLong), Action::StopAudio);
        assert_eq!(model.page(), Page::Home);
        assert_eq!(model.return_page(), Page::Home);
        assert_eq!(model.apply(Input::None), Action::None);
    }
}
