// SPDX-License-Identifier: MIT

use crate::error::{check, Result};

extern "C" {
    fn bsp_i2c_init() -> i32;
    fn bsp_i2c_scan() -> i32;
}

pub fn init() -> Result<()> {
    check(unsafe { bsp_i2c_init() })
}

pub fn scan() -> Result<()> {
    check(unsafe { bsp_i2c_scan() })
}
