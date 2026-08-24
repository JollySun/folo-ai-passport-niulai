// SPDX-License-Identifier: MIT

use passport_platform::storage::Storage;

#[cfg(target_os = "espidf")]
const PARTITION_LABEL: &core::ffi::CStr = c"recordings";
const VOICE_MAGIC: u32 = 0x4E4C_5652;
const VOICE_VERSION: u32 = 1;
const SLOT_BYTES: usize = 0x10_0000;
const BANK_BYTES: usize = 0x8_0000;
const HEADER_BYTES: usize = 32;
const MIN_BYTES: usize = 3200;
pub const SAMPLE_RATE: u32 = 16_000;
pub const MAX_SECONDS: usize = 10;
pub const MAX_BYTES: usize = SAMPLE_RATE as usize * 2 * MAX_SECONDS;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Slot {
    Calf = 0,
    Mother = 1,
}

impl Slot {
    const ALL: [Self; 2] = [Self::Calf, Self::Mother];

    const fn index(self) -> usize {
        self as usize
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StoreError {
    Unavailable,
    Invalid,
    TooShort,
    Platform(i32),
}

pub type Result<T> = core::result::Result<T, StoreError>;

pub(crate) trait Backend {
    fn size(&self) -> usize;
    fn read(&self, offset: usize, buffer: &mut [u8]) -> Result<()>;
    fn write(&mut self, offset: usize, buffer: &[u8]) -> Result<()>;
    fn erase(&mut self, offset: usize, bytes: usize) -> Result<()>;
}

impl Backend for Storage {
    fn size(&self) -> usize {
        Storage::size(self)
    }

    fn read(&self, offset: usize, buffer: &mut [u8]) -> Result<()> {
        Storage::read(self, offset, buffer).map_err(|error| StoreError::Platform(error.0))
    }

    fn write(&mut self, offset: usize, buffer: &[u8]) -> Result<()> {
        Storage::write(self, offset, buffer).map_err(|error| StoreError::Platform(error.0))
    }

    fn erase(&mut self, offset: usize, bytes: usize) -> Result<()> {
        Storage::erase(self, offset, bytes).map_err(|error| StoreError::Platform(error.0))
    }
}

#[derive(Clone, Copy, Default)]
struct Header {
    length: u32,
    sequence: u32,
}

impl Header {
    fn valid(self) -> bool {
        let length = self.length as usize;
        (MIN_BYTES..=MAX_BYTES).contains(&length) && length % 2 == 0
    }

    fn decode(bytes: &[u8; HEADER_BYTES]) -> Option<Self> {
        let word = |offset| {
            u32::from_le_bytes(
                bytes[offset..offset + 4]
                    .try_into()
                    .expect("four-byte word"),
            )
        };
        let header = Self {
            length: word(8),
            sequence: word(16),
        };
        (word(0) == VOICE_MAGIC
            && word(4) == VOICE_VERSION
            && word(12) == SAMPLE_RATE
            && header.valid())
        .then_some(header)
    }

    fn encode(self) -> [u8; HEADER_BYTES] {
        let mut bytes = [0; HEADER_BYTES];
        for (offset, value) in [
            (0, VOICE_MAGIC),
            (4, VOICE_VERSION),
            (8, self.length),
            (12, SAMPLE_RATE),
            (16, self.sequence),
        ] {
            bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        bytes
    }
}

#[derive(Clone, Copy)]
struct Recording {
    slot: Slot,
    bank: usize,
    bytes: usize,
}

pub(crate) struct VoiceStore<B = Storage> {
    backend: B,
    headers: [Header; 2],
    active_banks: [Option<usize>; 2],
    recording: Option<Recording>,
}

#[cfg(target_os = "espidf")]
impl VoiceStore<Storage> {
    pub fn open() -> Result<Self> {
        let storage = Storage::open(PARTITION_LABEL).map_err(|_| StoreError::Unavailable)?;
        Self::from_backend(storage)
    }
}

impl<B: Backend> VoiceStore<B> {
    fn from_backend(backend: B) -> Result<Self> {
        if backend.size() < SLOT_BYTES * Slot::ALL.len() {
            return Err(StoreError::Unavailable);
        }
        let mut store = Self {
            backend,
            headers: [Header::default(); 2],
            active_banks: [None; 2],
            recording: None,
        };
        for slot in Slot::ALL {
            for bank in 0..2 {
                let mut bytes = [0; HEADER_BYTES];
                if store
                    .backend
                    .read(bank_offset(slot, bank), &mut bytes)
                    .is_ok()
                {
                    if let Some(header) = Header::decode(&bytes) {
                        let index = slot.index();
                        if store.active_banks[index].is_none()
                            || header.sequence > store.headers[index].sequence
                        {
                            store.headers[index] = header;
                            store.active_banks[index] = Some(bank);
                        }
                    }
                }
            }
        }
        Ok(store)
    }

    pub fn has(&self, slot: Slot) -> bool {
        self.active_banks[slot.index()].is_some() && self.headers[slot.index()].valid()
    }

    pub fn length(&self, slot: Slot) -> usize {
        if self.has(slot) {
            self.headers[slot.index()].length as usize
        } else {
            0
        }
    }

    pub fn read(&self, slot: Slot, offset: usize, buffer: &mut [u8]) -> Result<()> {
        let length = self.length(slot);
        let bank = self.active_banks[slot.index()].ok_or(StoreError::Invalid)?;
        if buffer.is_empty() || offset > length || buffer.len() > length - offset {
            return Err(StoreError::Invalid);
        }
        self.backend
            .read(bank_offset(slot, bank) + HEADER_BYTES + offset, buffer)
    }

    pub fn begin(&mut self, slot: Slot) -> Result<()> {
        let bank = usize::from(self.active_banks[slot.index()] == Some(0));
        self.backend.erase(bank_offset(slot, bank), BANK_BYTES)?;
        self.recording = Some(Recording {
            slot,
            bank,
            bytes: 0,
        });
        Ok(())
    }

    pub fn append(&mut self, pcm: &[i16]) -> Result<()> {
        let mut recording = self.recording.ok_or(StoreError::Invalid)?;
        let bytes = core::mem::size_of_val(pcm);
        if bytes == 0 || bytes > MAX_BYTES - recording.bytes {
            return Err(StoreError::Invalid);
        }
        let pcm_bytes = unsafe { core::slice::from_raw_parts(pcm.as_ptr().cast(), bytes) };
        self.backend.write(
            bank_offset(recording.slot, recording.bank) + HEADER_BYTES + recording.bytes,
            pcm_bytes,
        )?;
        recording.bytes += bytes;
        self.recording = Some(recording);
        Ok(())
    }

    pub fn finish(&mut self) -> Result<()> {
        let recording = self.recording.take().ok_or(StoreError::Invalid)?;
        if recording.bytes < MIN_BYTES {
            return Err(StoreError::TooShort);
        }
        let index = recording.slot.index();
        let header = Header {
            length: recording.bytes as u32,
            sequence: self.active_banks[index]
                .map_or(1, |_| self.headers[index].sequence.saturating_add(1)),
        };
        self.backend.write(
            bank_offset(recording.slot, recording.bank),
            &header.encode(),
        )?;
        self.headers[index] = header;
        self.active_banks[index] = Some(recording.bank);
        Ok(())
    }

    pub fn cancel(&mut self) {
        self.recording = None;
    }

    pub fn reset(&mut self) -> Result<()> {
        self.cancel();
        self.backend.erase(0, self.backend.size())?;
        self.headers = [Header::default(); 2];
        self.active_banks = [None; 2];
        Ok(())
    }
}

const fn bank_offset(slot: Slot, bank: usize) -> usize {
    slot.index() * SLOT_BYTES + bank * BANK_BYTES
}

#[cfg(test)]
mod tests {
    use super::{Backend, Result, Slot, VoiceStore, BANK_BYTES, HEADER_BYTES, SLOT_BYTES};

    struct MemoryBackend(Vec<u8>);

    impl MemoryBackend {
        fn new() -> Self {
            Self(vec![0xff; SLOT_BYTES * 2])
        }
    }

    impl Backend for MemoryBackend {
        fn size(&self) -> usize {
            self.0.len()
        }
        fn read(&self, offset: usize, buffer: &mut [u8]) -> Result<()> {
            buffer.copy_from_slice(&self.0[offset..offset + buffer.len()]);
            Ok(())
        }
        fn write(&mut self, offset: usize, buffer: &[u8]) -> Result<()> {
            self.0[offset..offset + buffer.len()].copy_from_slice(buffer);
            Ok(())
        }
        fn erase(&mut self, offset: usize, bytes: usize) -> Result<()> {
            self.0[offset..offset + bytes].fill(0xff);
            Ok(())
        }
    }

    #[test]
    fn commits_header_last_and_recovers_newest_bank() {
        let mut store = VoiceStore::from_backend(MemoryBackend::new()).unwrap();
        assert!(!store.has(Slot::Calf));

        store.begin(Slot::Calf).unwrap();
        store.append(&[7; 1600]).unwrap();
        store.finish().unwrap();
        assert_eq!(store.length(Slot::Calf), 3200);

        store.begin(Slot::Calf).unwrap();
        store.append(&[9; 2000]).unwrap();
        store.finish().unwrap();

        let backend = store.backend;
        let mut recovered = VoiceStore::from_backend(backend).unwrap();
        assert_eq!(recovered.length(Slot::Calf), 4000);
        let mut bytes = [0; 4];
        recovered.read(Slot::Calf, 0, &mut bytes).unwrap();
        assert_eq!(bytes, [9, 0, 9, 0]);
        recovered.reset().unwrap();
        assert!(!recovered.has(Slot::Calf));
    }

    #[test]
    fn unfinished_bank_does_not_replace_active_recording() {
        let mut store = VoiceStore::from_backend(MemoryBackend::new()).unwrap();
        store.begin(Slot::Mother).unwrap();
        store.append(&[3; 1600]).unwrap();
        store.finish().unwrap();
        store.begin(Slot::Mother).unwrap();
        store.append(&[4; 1800]).unwrap();
        store.cancel();

        let backend = store.backend;
        let recovered = VoiceStore::from_backend(backend).unwrap();
        assert_eq!(recovered.length(Slot::Mother), 3200);
        assert_eq!(super::bank_offset(Slot::Mother, 1), SLOT_BYTES + BANK_BYTES);
        assert_eq!(HEADER_BYTES, 32);
    }
}
