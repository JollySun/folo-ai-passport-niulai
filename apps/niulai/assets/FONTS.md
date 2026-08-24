# Font notice

The generated `apps/niulai/firmware/niulai_font_12.c`,
`apps/niulai/firmware/niulai_font_16.c`, and
`apps/niulai/firmware/niulai_font_22.c` files contain only the Chinese glyphs
used by the on-device interface.

They were generated with `lv_font_conv` from Adobe Source Han Sans SC Regular,
which is available under the SIL Open Font License 1.1:

- Project: <https://github.com/adobe-fonts/source-han-sans>
- License: <https://openfontlicense.org/open-font-license-official-text/>

The generated font files use LVGL's built-in Montserrat fonts as fallback for
ASCII digits and symbols.
