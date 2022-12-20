use alloc::vec;
use alloc::vec::Vec;


#[derive(Clone, Copy, Debug)]
enum LooperState {
    Playing,
    Recording,
    Stopped
}

pub struct Looper {
    state: LooperState,
    buffer: Vec<f32>,
    buffer_pos: usize,
    recording_length: usize,
    is_playing_test_tone: bool
}

// About a second @44.1kHz
const BUFFER_SIZE: usize = 100;

impl Looper {
    pub fn new() -> Self {
        let vector = vec![10_f32; BUFFER_SIZE];
        Looper {
            state: LooperState::Stopped,
            buffer: vector,
            buffer_pos: 0,
            recording_length: 0,
            is_playing_test_tone: false
        }
    }

    pub fn process(&mut self, rx: &[f32], tx: &mut [f32], sample_count: usize, channel_count: usize) {
        assert!(rx.len() == tx.len());
        match self.state {
            LooperState::Playing => {
                for tx_sample in tx.iter_mut() {
                    *tx_sample = self.buffer[self.buffer_pos];
                    self.buffer_pos += 1;
                    if self.buffer_pos == BUFFER_SIZE {
                        self.buffer_pos = 0;
                    }
                }
            },
            LooperState::Recording => {
                for rx_sample in rx.iter() {
                    self.buffer[self.buffer_pos] = *rx_sample;
                    self.buffer_pos += 1;
                    if self.buffer_pos == BUFFER_SIZE {
                        self.transition_to_state(LooperState::Stopped);
                        break
                    }
                }
            },
            LooperState::Stopped => {
                //
            }
        }
    }

    fn state(&self) -> LooperState {
        self.state
    }

    fn transition_to_state(&mut self, new_state: LooperState) {

    }
}