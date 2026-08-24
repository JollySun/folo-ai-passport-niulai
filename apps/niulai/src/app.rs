// SPDX-License-Identifier: MIT

use crate::assets::{self, Assets};
use crate::ui::{Ui, ViewState};
use crate::voice_store::{Slot, VoiceStore, MAX_BYTES, SAMPLE_RATE};
use crate::{Action, Input, Model, Page, RecordState};
use core::ffi::c_void;
use passport_platform::audio::Audio;
use passport_platform::battery::Battery;
use passport_platform::button::{self, Button, Event};
use passport_platform::display::{self, Display};
use passport_platform::log::{self, Level};
use passport_platform::runtime::{self, Queue, StaticCell, StaticMutex, WAIT_FOREVER};
use passport_platform::Result;

const AUDIO_CHUNK_SAMPLES: usize = 512;
const MAMA_ANIMATION_MS: u32 = 2550;
const NIULAI_ANIMATION_MS: u32 = 1350;
const ANIMATION_PERIOD_MS: u32 = 120;
const BATTERY_REFRESH_MS: u32 = 1000;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
enum AudioCommand {
    Stop,
    PlayMama,
    PlayNiulai,
    RecordCalf,
    RecordMother,
    StopRecording,
    ResetVoices,
}

struct AppState {
    model: Model,
    ui: Ui,
    store: Option<VoiceStore>,
    audio_ok: bool,
    battery_soc: Option<u8>,
    record_state: RecordState,
    record_slot: Slot,
    record_button_down: bool,
    record_button: Button,
    animation_active: bool,
    animation_frame: bool,
    animation_left_ms: u32,
    record_pulse_ms: u32,
    record_pulse_on: bool,
}

impl AppState {
    fn view(&self) -> ViewState {
        let custom_voice_count = self.store.as_ref().map_or(0, |store| {
            u8::from(store.has(Slot::Calf)) + u8::from(store.has(Slot::Mother))
        });
        let record_page = matches!(
            (self.model.page(), self.record_slot),
            (Page::Calf, Slot::Calf) | (Page::Mother, Slot::Mother)
        );
        ViewState {
            page: self.model.page(),
            volume: self.model.volume(),
            audio_ok: self.audio_ok,
            battery_soc: self.battery_soc,
            record_state: self.record_state,
            record_page,
            custom_voice_count,
            storage_available: self.store.is_some(),
        }
    }

    fn start_animation(&mut self, duration_ms: u32) {
        self.animation_active = true;
        self.animation_frame = false;
        self.animation_left_ms = duration_ms;
        self.ui.set_animation_frame(self.model.page(), false);
    }
}

static STATE: StaticMutex<AppState> = StaticMutex::new();
static AUDIO_QUEUE: StaticCell<Queue<AudioCommand>> = StaticCell::new();

pub fn start() -> Result<()> {
    let tag = c"niulai";
    log::write(Level::Info, tag, c"Starting Rust application");
    passport_platform::i2c::init()?;

    let mut display = Display::init()?;
    display.set_backlight(100);
    let store = VoiceStore::open().ok();
    let audio_ok = Audio::init().is_ok();
    let battery_soc = Battery::init()
        .ok()
        .and_then(|battery| battery.state_of_charge().ok());

    let assets = Assets::load()?;
    let ui = {
        let _guard = display::lock(1000)?;
        Ui::build(assets)?
    };
    STATE.init(AppState {
        model: Model::default(),
        ui,
        store,
        audio_ok,
        battery_soc,
        record_state: RecordState::Idle,
        record_slot: Slot::Calf,
        record_button_down: false,
        record_button: Button::Up,
        animation_active: false,
        animation_frame: false,
        animation_left_ms: 0,
        record_pulse_ms: 0,
        record_pulse_on: false,
    })?;

    render_and_load()?;

    if audio_ok {
        match Queue::new(1).and_then(|queue| AUDIO_QUEUE.init(queue).map(|_| ())) {
            Ok(()) => {
                if let Err(error) = runtime::spawn(audio_task, c"niulai_audio", 6144, 4) {
                    set_audio_unavailable();
                    log::write_args(
                        Level::Error,
                        tag,
                        format_args!("Audio task failed: {}", error.0),
                    );
                }
            }
            Err(error) => {
                set_audio_unavailable();
                log::write_args(
                    Level::Error,
                    tag,
                    format_args!("Audio queue failed: {}", error.0),
                );
            }
        }
    }

    if let Err(error) = runtime::spawn(battery_task, c"niulai_battery", 3072, 2) {
        log::write_args(
            Level::Warn,
            tag,
            format_args!("Battery task failed: {}", error.0),
        );
    }
    button::init(on_button)?;
    log::write(Level::Info, tag, c"Rust application ready");
    Ok(())
}

fn render_and_load() -> Result<()> {
    let _ui_guard = display::lock(1000)?;
    let mut state = STATE.lock(WAIT_FOREVER)?;
    let view = state.view();
    state.ui.render(view);
    state.ui.load();
    Ok(())
}

fn render() {
    let Ok(_ui_guard) = display::lock(500) else {
        return;
    };
    let Ok(mut state) = STATE.lock(WAIT_FOREVER) else {
        return;
    };
    let view = state.view();
    state.ui.render(view);
}

fn set_audio_unavailable() {
    if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
        state.audio_ok = false;
    }
    render();
}

fn set_record_state(record_state: RecordState) {
    if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
        state.record_state = record_state;
    }
    render();
}

fn audio_request(command: AudioCommand) {
    if let Some(queue) = AUDIO_QUEUE.get() {
        queue.overwrite(command);
    }
}

fn on_button(button: Button, event: Event) {
    let mut command = None;
    let Ok(mut state) = STATE.lock(WAIT_FOREVER) else {
        return;
    };

    if event == Event::Release && state.record_button_down && button == state.record_button {
        state.record_button_down = false;
        state.record_state = RecordState::Saving;
        command = Some(AudioCommand::StopRecording);
    } else {
        let record_calf =
            event == Event::Long && button == Button::Up && state.model.page() == Page::Calf;
        let record_mother =
            event == Event::Long && button == Button::Down && state.model.page() == Page::Mother;
        if record_calf || record_mother {
            state.record_slot = if record_calf {
                Slot::Calf
            } else {
                Slot::Mother
            };
            state.record_button = button;
            state.record_state = if state.audio_ok && state.store.is_some() {
                RecordState::Preparing
            } else {
                RecordState::Failed
            };
            state.record_button_down = state.record_state == RecordState::Preparing;
            if state.record_button_down {
                command = Some(if record_calf {
                    AudioCommand::RecordCalf
                } else {
                    AudioCommand::RecordMother
                });
            }
        } else if event == Event::Double
            && button == Button::Ok
            && state.model.page() == Page::Settings
        {
            state.record_state = if AUDIO_QUEUE.get().is_some() && state.store.is_some() {
                RecordState::Preparing
            } else {
                RecordState::Failed
            };
            if state.record_state == RecordState::Preparing {
                command = Some(AudioCommand::ResetVoices);
            }
        } else if let Some(input) = map_input(button, event) {
            if matches!(
                state.record_state,
                RecordState::Saved | RecordState::Failed | RecordState::ResetDone
            ) {
                state.record_state = RecordState::Idle;
            }
            let action = state.model.apply(input);
            match action {
                Action::PlayMama => {
                    state.start_animation(MAMA_ANIMATION_MS);
                    command = Some(AudioCommand::PlayMama);
                }
                Action::PlayNiulai => {
                    state.start_animation(NIULAI_ANIMATION_MS);
                    command = Some(AudioCommand::PlayNiulai);
                }
                Action::StopAudio => command = Some(AudioCommand::Stop),
                Action::None | Action::VolumeChanged => {}
            }
        }
    }
    drop(state);
    render();
    if let Some(command) = command {
        audio_request(command);
    }
}

fn map_input(button: Button, event: Event) -> Option<Input> {
    match (button, event) {
        (Button::Up, Event::Click) => Some(Input::UpClick),
        (Button::Down, Event::Click) => Some(Input::DownClick),
        (Button::Ok, Event::Click) => Some(Input::OkClick),
        (Button::Ok, Event::Long) => Some(Input::OkLong),
        _ => None,
    }
}

extern "C" fn audio_task(_argument: *mut c_void) {
    let Some(queue) = AUDIO_QUEUE.get() else {
        return;
    };
    let Ok(mut audio) = Audio::init() else {
        set_audio_unavailable();
        return;
    };
    loop {
        let Some(mut command) = queue.receive(WAIT_FOREVER) else {
            continue;
        };
        while command != AudioCommand::Stop {
            command = match command {
                AudioCommand::PlayMama | AudioCommand::PlayNiulai => {
                    play_voice(&mut audio, command)
                }
                AudioCommand::RecordCalf | AudioCommand::RecordMother => {
                    record_voice(&mut audio, command)
                }
                AudioCommand::ResetVoices => {
                    let success = STATE
                        .lock(WAIT_FOREVER)
                        .ok()
                        .and_then(|mut state| {
                            state.store.as_mut().map(|store| store.reset().is_ok())
                        })
                        .unwrap_or(false);
                    set_record_state(if success {
                        RecordState::ResetDone
                    } else {
                        RecordState::Failed
                    });
                    AudioCommand::Stop
                }
                _ => AudioCommand::Stop,
            };
        }
    }
}

fn play_voice(audio: &mut Audio, command: AudioCommand) -> AudioCommand {
    let slot = if command == AudioCommand::PlayMama {
        Slot::Calf
    } else {
        Slot::Mother
    };
    let (custom, total, volume) = STATE.lock(WAIT_FOREVER).map_or((false, 0, 100), |state| {
        let custom = state.store.as_ref().is_some_and(|store| store.has(slot));
        let total = if custom {
            state.store.as_ref().map_or(0, |store| store.length(slot))
        } else {
            assets::default_voice(slot).len()
        };
        (custom, total, state.model.volume())
    });
    if audio.set_format(SAMPLE_RATE, 16, 1).is_err() {
        return AudioCommand::Stop;
    }
    audio.set_volume(100);
    let mut raw = [0i16; AUDIO_CHUNK_SAMPLES];
    let mut scaled = [0i16; AUDIO_CHUNK_SAMPLES];
    let default = assets::default_voice(slot);

    let mut offset = 0;
    while offset < total {
        let bytes = (total - offset).min(core::mem::size_of_val(&raw));
        let samples = bytes / 2;
        let raw_bytes = unsafe { core::slice::from_raw_parts_mut(raw.as_mut_ptr().cast(), bytes) };
        let loaded = if custom {
            STATE.lock(WAIT_FOREVER).is_ok_and(|state| {
                state
                    .store
                    .as_ref()
                    .is_some_and(|store| store.read(slot, offset, raw_bytes).is_ok())
            })
        } else {
            raw_bytes.copy_from_slice(&default[offset..offset + bytes]);
            true
        };
        if !loaded {
            return AudioCommand::Stop;
        }
        passport_core::scale_pcm16(&mut scaled[..samples], &raw[..samples], volume);
        if audio.write(&scaled[..samples]).is_err() {
            return AudioCommand::Stop;
        }
        offset += bytes;
        if let Some(next) = AUDIO_QUEUE.get().and_then(|queue| queue.receive(0)) {
            return next;
        }
    }
    AudioCommand::Stop
}

fn record_voice(audio: &mut Audio, command: AudioCommand) -> AudioCommand {
    let slot = if command == AudioCommand::RecordCalf {
        Slot::Calf
    } else {
        Slot::Mother
    };
    if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
        state.record_slot = slot;
    }
    set_record_state(RecordState::Preparing);
    let begun = STATE.lock(WAIT_FOREVER).is_ok_and(|mut state| {
        state
            .store
            .as_mut()
            .is_some_and(|store| store.begin(slot).is_ok())
    });
    if !begun || audio.set_format(SAMPLE_RATE, 16, 1).is_err() {
        cancel_recording();
        set_record_state(RecordState::Failed);
        return AudioCommand::Stop;
    }

    set_record_state(RecordState::Active);
    let mut pcm = [0i16; AUDIO_CHUNK_SAMPLES];
    let mut total = 0;
    let mut next = AudioCommand::Stop;
    let mut failed = false;
    while total < MAX_BYTES {
        let samples = ((MAX_BYTES - total) / 2).min(pcm.len());
        if audio.read(&mut pcm[..samples]).is_err()
            || !STATE.lock(WAIT_FOREVER).is_ok_and(|mut state| {
                state
                    .store
                    .as_mut()
                    .is_some_and(|store| store.append(&pcm[..samples]).is_ok())
            })
        {
            failed = true;
            break;
        }
        total += samples * 2;
        if let Some(command) = AUDIO_QUEUE.get().and_then(|queue| queue.receive(0)) {
            next = command;
            break;
        }
    }

    set_record_state(RecordState::Saving);
    let saved = !failed
        && STATE.lock(WAIT_FOREVER).is_ok_and(|mut state| {
            state
                .store
                .as_mut()
                .is_some_and(|store| store.finish().is_ok())
        });
    if !saved {
        cancel_recording();
    }
    if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
        state.record_button_down = false;
    }
    set_record_state(if saved {
        RecordState::Saved
    } else {
        RecordState::Failed
    });
    if next == AudioCommand::StopRecording {
        AudioCommand::Stop
    } else {
        next
    }
}

fn cancel_recording() {
    if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
        if let Some(store) = state.store.as_mut() {
            store.cancel();
        }
        state.record_button_down = false;
    }
}

extern "C" fn battery_task(_argument: *mut c_void) {
    let mut battery = Battery::init().ok();
    loop {
        if battery.is_none() {
            battery = Battery::init().ok();
        }
        let percent = battery
            .as_ref()
            .and_then(|battery| battery.state_of_charge().ok());
        if let Ok(mut state) = STATE.lock(WAIT_FOREVER) {
            if percent.is_some() {
                state.battery_soc = percent;
            }
        }
        if let Ok(_ui_guard) = display::lock(500) {
            if let Ok(state) = STATE.lock(WAIT_FOREVER) {
                state.ui.update_battery(state.battery_soc);
            }
        }
        runtime::delay_ms(BATTERY_REFRESH_MS);
    }
}

pub extern "C" fn animation_tick() {
    let Ok(mut state) = STATE.lock(0) else { return };
    if state.record_state == RecordState::Active
        && matches!(state.model.page(), Page::Calf | Page::Mother)
    {
        state.record_pulse_ms += ANIMATION_PERIOD_MS;
        if state.record_pulse_ms >= 480 {
            state.record_pulse_ms = 0;
            state.record_pulse_on = !state.record_pulse_on;
            state.ui.set_record_pulse(state.record_pulse_on);
        }
    }
    if !state.animation_active || matches!(state.model.page(), Page::Home | Page::Settings) {
        return;
    }
    state.animation_frame = !state.animation_frame;
    state
        .ui
        .set_animation_frame(state.model.page(), state.animation_frame);
    if state.animation_left_ms <= ANIMATION_PERIOD_MS {
        state.animation_active = false;
        state.animation_frame = false;
        state.ui.set_animation_frame(state.model.page(), false);
    } else {
        state.animation_left_ms -= ANIMATION_PERIOD_MS;
    }
}
