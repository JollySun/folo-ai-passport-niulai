// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#![cfg_attr(target_os = "espidf", no_std)]

pub use passport_core as shared_core;
use passport_platform::log::{self, Level};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Report {
    pub millivolts: i32,
    pub gauge_percent: u8,
    pub estimated_percent: i32,
}

impl Report {
    pub fn new(millivolts: i32, gauge_percent: u8) -> Self {
        Self {
            millivolts,
            gauge_percent,
            estimated_percent: passport_core::battery_percent_from_voltage(millivolts),
        }
    }
}

#[no_mangle]
pub extern "C" fn passport_app_main() -> i32 {
    let tag = c"diagnostics";
    log::write(Level::Info, tag, c"Starting board diagnostics");

    if let Err(error) = passport_platform::i2c::init() {
        log::write_args(
            Level::Error,
            tag,
            format_args!("I2C initialization failed: {}", error.0),
        );
        return error.0;
    }
    if let Err(error) = passport_platform::i2c::scan() {
        log::write_args(
            Level::Warn,
            tag,
            format_args!("I2C scan failed: {}", error.0),
        );
    }

    let battery = match passport_platform::battery::Battery::init() {
        Ok(battery) => battery,
        Err(error) => {
            log::write_args(
                Level::Warn,
                tag,
                format_args!(
                    "Battery gauge unavailable: {}; voltage estimate at 3750mV={}%%",
                    error.0,
                    passport_core::battery_percent_from_voltage(3750)
                ),
            );
            return 0;
        }
    };

    let millivolts = battery.millivolts().unwrap_or(-1);
    let gauge_percent = battery.state_of_charge().unwrap_or(0);
    let report = Report::new(millivolts, gauge_percent);
    log::write_args(
        Level::Info,
        tag,
        format_args!(
            "Battery: {}mV, SOC={}%%, voltage estimate={}%%",
            report.millivolts, report.gauge_percent, report.estimated_percent
        ),
    );
    0
}

#[cfg(test)]
mod tests {
    use super::Report;

    #[test]
    fn report_combines_gauge_and_shared_voltage_estimate() {
        assert_eq!(
            Report::new(3750, 42),
            Report {
                millivolts: 3750,
                gauge_percent: 42,
                estimated_percent: 37,
            }
        );
    }
}
