// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

const VOLTAGE_CURVE: [(i32, i32); 9] = [
    (3300, 0),
    (3500, 5),
    (3600, 10),
    (3700, 25),
    (3800, 50),
    (3900, 65),
    (4000, 80),
    (4100, 92),
    (4200, 100),
];

/// Returns a conservative 0..=100 estimate for a single-cell Li-ion voltage.
pub fn battery_percent_from_voltage(millivolts: i32) -> i32 {
    if millivolts <= VOLTAGE_CURVE[0].0 {
        return 0;
    }

    for pair in VOLTAGE_CURVE.windows(2) {
        let (lower_mv, lower_percent) = pair[0];
        let (upper_mv, upper_percent) = pair[1];
        if millivolts <= upper_mv {
            return lower_percent
                + (millivolts - lower_mv) * (upper_percent - lower_percent)
                    / (upper_mv - lower_mv);
        }
    }
    100
}

#[cfg(test)]
mod tests {
    use super::battery_percent_from_voltage;

    #[test]
    fn interpolates_and_clamps_voltage() {
        assert_eq!(battery_percent_from_voltage(3200), 0);
        assert_eq!(battery_percent_from_voltage(3300), 0);
        assert_eq!(battery_percent_from_voltage(3500), 5);
        assert_eq!(battery_percent_from_voltage(3750), 37);
        assert_eq!(battery_percent_from_voltage(4000), 80);
        assert_eq!(battery_percent_from_voltage(4154), 96);
        assert_eq!(battery_percent_from_voltage(4300), 100);
    }
}
