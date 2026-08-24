// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

/// Scales signed 16-bit PCM samples by a percentage clamped to 0..=100.
///
/// The output slice determines how many input samples are consumed. Callers
/// must provide at least that many input samples.
pub fn scale_pcm16(output: &mut [i16], input: &[i16], volume: u8) {
    assert!(input.len() >= output.len());
    let gain = i32::from(volume.min(100));
    for (destination, source) in output.iter_mut().zip(input) {
        *destination = ((i32::from(*source) * gain) / 100) as i16;
    }
}

#[cfg(test)]
mod tests {
    use super::scale_pcm16;

    #[test]
    fn scales_and_clamps_volume() {
        let source = [-30_000, -1_000, 0, 1_000, 30_000];
        let mut output = [0; 5];

        scale_pcm16(&mut output, &source, 100);
        assert_eq!(output, source);

        scale_pcm16(&mut output, &source, 255);
        assert_eq!(output, source);

        scale_pcm16(&mut output, &source, 50);
        assert_eq!(output, [-15_000, -500, 0, 500, 15_000]);

        scale_pcm16(&mut output, &source, 0);
        assert_eq!(output, [0; 5]);
    }
}
