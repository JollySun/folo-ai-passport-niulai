# Niu Lai asset sources

The firmware uses short, low-resolution excerpts from publicly posted promotional/news material solely for the requested prototype.

- Poster source: <https://k.sina.cn/article_1750353854_685447be01901dk7q.html>
- Dialogue and character-frame source: <https://www.facebook.com/eranews50/videos/2530789267363673/>
- `mother-2-open-preview.png` is a mouth-only animation derivative created with
  OpenAI's built-in ImageGen from the source mother frame, then composited over
  the original frame to preserve the character, background, and frame alignment.
- Extracted dialogue windows from the public news clip:
  - `mama.pcm`: approximately 00:00.92–00:03.47
  - `niulai.pcm`: approximately 00:06.15–00:07.50

The repository stores only the processed 240-pixel preview frames and the two short 16 kHz mono PCM excerpts, not the downloaded news video. Confirm distribution rights before publishing binaries containing these assets.
