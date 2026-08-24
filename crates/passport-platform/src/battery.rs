// SPDX-License-Identifier: MIT

use crate::error::{check, Error, Result};

extern "C" {
    fn bsp_battery_init() -> i32;
    fn bsp_battery_soc() -> i32;
    fn bsp_battery_mv() -> i32;
}

pub struct Battery;

impl Battery {
    pub fn init() -> Result<Self> {
        check(unsafe { bsp_battery_init() })?;
        Ok(Self)
    }

    pub fn state_of_charge(&self) -> Result<u8> {
        let raw = unsafe { bsp_battery_soc() };
        let millivolts = self.millivolts()?;
        normalize_state_of_charge(raw, millivolts)
    }

    pub fn millivolts(&self) -> Result<i32> {
        let value = unsafe { bsp_battery_mv() };
        if value < 0 {
            Err(Error(value))
        } else {
            Ok(value)
        }
    }
}

fn normalize_state_of_charge(raw: i32, millivolts: i32) -> Result<u8> {
    if (0..=100).contains(&raw) && !(raw == 0 && millivolts >= 3300) {
        return Ok(raw as u8);
    }
    if !(2500..=4500).contains(&millivolts) {
        return Err(Error(raw));
    }
    Ok(passport_core::battery_percent_from_voltage(millivolts) as u8)
}

#[cfg(test)]
mod tests {
    use super::normalize_state_of_charge;

    #[test]
    fn keeps_valid_gauge_value_and_falls_back_when_gauge_is_not_ready() {
        assert_eq!(normalize_state_of_charge(62, 3900), Ok(62));
        assert_eq!(normalize_state_of_charge(0, 3750), Ok(37));
        assert_eq!(normalize_state_of_charge(255, 4300), Ok(100));
        assert!(normalize_state_of_charge(255, 5000).is_err());
    }
}
