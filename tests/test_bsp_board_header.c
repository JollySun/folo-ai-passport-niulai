// SPDX-License-Identifier: MIT

#include "bsp_pins.h"

_Static_assert(BSP_LCD_W > 0, "display width must be positive");
_Static_assert(BSP_LCD_H > 0, "display height must be positive");
_Static_assert(BSP_I2C_SDA >= 0, "I2C SDA pin must be assigned");
_Static_assert(BSP_I2C_SCL >= 0, "I2C SCL pin must be assigned");

int main(void)
{
    return 0;
}
