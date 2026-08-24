// SPDX-License-Identifier: MIT

#include <stdint.h>

// Stable firmware seam: every Rust application exports this symbol. Adding an
// application must not require a new C entry point or a public registry.
extern int32_t passport_app_main(void);

void app_main(void)
{
    (void)passport_app_main();
}
