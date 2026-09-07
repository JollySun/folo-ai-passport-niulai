# Niu Lai asset sources

The firmware uses short, low-resolution excerpts from publicly posted promotional/news material for a Niu Lai derivative device experience. On 2026-08-26, the project owner confirmed that the film rights holder authorized this secondary development. The written authorization and its distribution terms are maintained outside this repository.

These media files are **not licensed under the repository's MIT License**. This file records provenance; it does not grant permission to copy, publish, or distribute the underlying movie content.

- Poster source: <https://k.sina.cn/article_1750353854_685447be01901dk7q.html>
- Dialogue and character-frame source: <https://www.facebook.com/eranews50/videos/2530789267363673/>
- `mother-2-open-preview.png` is a mouth-only animation derivative created with
  OpenAI's built-in ImageGen from the source mother frame, then composited over
  the original frame to preserve the character, background, and frame alignment.
- `home-ui-preview.png` is a documentation-only UI mockup created with OpenAI's
  built-in ImageGen from `home-preview.png`; firmware continues to embed the
  original poster and renders the interface with LVGL at runtime.
- `docs/screenshots/home.png`, `niulai.png`, `mama.png`, and `settings.png` are
  software-rendered documentation captures of the current 240x320 LVGL layout,
  composed from the firmware UI geometry and the corresponding embedded frames.
- Processed dialogue assets from the public news clip:
  - `mama.pcm`: 4.09 seconds, 12 kHz/16-bit/mono.
  - `niulai.pcm`: 1.35 seconds, 12 kHz/16-bit/mono.
- `mama-preview.wav` and `niulai-preview.wav` contain the same samples as the
  firmware PCM files in a WAV container for desktop preview.

The repository stores only the processed poster/character frames and two short
character audio excerpts, not the downloaded news video. Before public
distribution, verify that the intended release channel and territory are
covered by the external authorization.

The on-device Chinese font subsets are covered separately in [the font notice](../FONTS.md).
