use crate::{AppMessage, DemoApp};
use alloc::{vec, vec::Vec};
use microdsp::nlms::NlmsFilter;

const RECORD_BUFFER_SIZE: usize = 4000; // size in samples
const OUT_MSG_BUFFER_SIZE: usize = 16;
const MAX_TX_BUFFER_SIZE: usize = 512;
const OSC_FREQ: f32 = 440.0;

/// Sine-ish oscillator, approximating sin(pi * x) as
// 4x + 4x^2 on [-1, 0]
// 4x - 4x^2 on [0, 1]
struct Oscillator {
    d_phase: f32,
    phase: f32,
    sample_interval: f32,
}

impl Oscillator {
    pub fn new(sample_rate: f32) -> Self {
        Oscillator {
            d_phase: 0.,
            sample_interval: 1.0 / sample_rate,
            phase: -1.,
        }
    }

    fn reset(&mut self) {
        self.phase = -1.;
    }

    fn next_sample(&mut self) -> f32 {
        let a = if self.phase < 0. { 1. } else { -1. };
        debug_assert!(self.phase >= -1. && self.phase <= 1.);

        let value = self.phase + a * self.phase * self.phase;

        self.phase += self.d_phase;

        if self.phase > 1.0 {
            self.phase -= 2.
        }
        -4. * value
    }

    fn set_frequency(&mut self, frequency: f32) {
        self.d_phase = 2. * frequency * self.sample_interval;
    }
}

#[derive(PartialEq)]
enum RecordingState {
    Playing,
    Recording,
    Idle,
}

pub struct NlmsDemoApp {
    filter: NlmsFilter,
    tx_prev: Vec<f32>,
    record_buffer: Vec<f32>,
    record_buffer_pos: usize,
    out_msg_buffer: Vec<AppMessage>,
    filter_is_active: bool,
    oscillator_enabled: bool,
    recording_state: RecordingState,

    tone_osc: Oscillator,
    pitch_lfo: Oscillator,
}

impl NlmsDemoApp {
    fn send_message(&mut self, message: AppMessage) {
        if self.out_msg_buffer.len() < OUT_MSG_BUFFER_SIZE {
            self.out_msg_buffer.push(message)
        }
    }

    fn stop_recording(&mut self) {
        assert!(self.recording_state == RecordingState::Recording);
        self.recording_state = RecordingState::Idle;
        self.send_message(AppMessage::Led2Off);
    }
}

impl DemoApp for NlmsDemoApp {
    fn new(sample_rate: f32) -> Self {
        let mut tone_osc = Oscillator::new(sample_rate);
        tone_osc.set_frequency(OSC_FREQ);
        let mut pitch_lfo = Oscillator::new(sample_rate);
        pitch_lfo.set_frequency(1.0);
        NlmsDemoApp {
            filter: NlmsFilter::new(10, 0.1, 0.0001),
            tx_prev: vec![0.0; MAX_TX_BUFFER_SIZE],
            record_buffer: vec![0.0; RECORD_BUFFER_SIZE],
            record_buffer_pos: 0,
            out_msg_buffer: Vec::with_capacity(OUT_MSG_BUFFER_SIZE),
            filter_is_active: false,
            oscillator_enabled: false,
            recording_state: RecordingState::Idle,
            tone_osc,
            pitch_lfo,
        }
    }

    fn process(&mut self, rx: &[f32], tx: &mut [f32]) {
        assert!(tx.len() < MAX_TX_BUFFER_SIZE);
        if self.oscillator_enabled {
            for tx in tx.iter_mut() {
                let lfo = self.pitch_lfo.next_sample();
                let lfo_depth = 0.1;
                let osc_gain = 0.1;
                let freq = OSC_FREQ * (1.0 + lfo * lfo_depth);
                self.tone_osc.set_frequency(freq);
                *tx += osc_gain * self.tone_osc.next_sample();
            }
        }

        match self.recording_state {
            RecordingState::Idle => {
                // Nothing
            }
            RecordingState::Playing => {
                for tx in tx.iter_mut() {
                    self.record_buffer_pos = if self.record_buffer_pos == RECORD_BUFFER_SIZE - 1 {
                        0
                    } else {
                        self.record_buffer_pos + 1
                    };
                    *tx += self.record_buffer[self.record_buffer_pos];
                }
            }
            RecordingState::Recording => {
                if self.filter_is_active {
                    let tx_prev = &self.tx_prev;
                    for (rx, tx_prev) in rx.iter().zip(tx_prev.iter()) {
                        self.record_buffer[self.record_buffer_pos] =
                            self.filter.update(*tx_prev, *rx);
                        self.record_buffer_pos += 1;
                        if self.record_buffer_pos == RECORD_BUFFER_SIZE {
                            break;
                        }
                    }
                    if self.record_buffer_pos == RECORD_BUFFER_SIZE {
                        self.stop_recording()
                    }
                } else {
                    for rx in rx.iter() {
                        self.record_buffer[self.record_buffer_pos] = *rx;
                        self.record_buffer_pos += 1;
                        if self.record_buffer_pos == RECORD_BUFFER_SIZE {
                            self.stop_recording()
                        }
                    }
                }
            }
        }

        // Store the current tx buffer
        for (tx, tx_prev) in tx.iter().zip(self.tx_prev.iter_mut()) {
            *tx_prev = *tx;
        }
    }

    fn handle_message(&mut self, message: crate::AppMessage) {
        match message {
            AppMessage::Button0Down => {
                // Toggle oscillator
                if self.oscillator_enabled {
                    self.oscillator_enabled = false;
                    self.send_message(AppMessage::Led0Off)
                } else {
                    self.oscillator_enabled = true;
                    self.send_message(AppMessage::Led0On);
                    self.tone_osc.reset();
                    self.pitch_lfo.reset();
                }
            }
            AppMessage::Button1Down => {
                // Toggle filter
                self.filter_is_active = !self.filter_is_active;
                if self.filter_is_active {
                    self.send_message(AppMessage::Led1On)
                } else {
                    self.send_message(AppMessage::Led1Off)
                }
            }
            AppMessage::Button2Down => {
                match self.recording_state {
                    RecordingState::Idle | RecordingState::Playing => {
                        // Start recording
                        self.record_buffer_pos = 0;
                        self.recording_state = RecordingState::Recording;
                        self.filter.reset();
                        self.send_message(AppMessage::Led2On);
                    }
                    RecordingState::Recording => {
                        // Stop recording
                        self.stop_recording()
                    }
                }
            }
            AppMessage::Button3Down => {
                match self.recording_state {
                    RecordingState::Idle => {
                        // Start playback
                        self.record_buffer_pos = 0;
                        self.recording_state = RecordingState::Playing;
                        self.send_message(AppMessage::Led3On);
                    }
                    RecordingState::Playing => {
                        // Stop playback
                        self.recording_state = RecordingState::Idle;
                        self.send_message(AppMessage::Led3Off);
                    }
                    RecordingState::Recording => {
                        // Ignore playback start/stop button if recording is in progress
                    }
                }
            }
            _ => {}
        }
    }

    fn next_outgoing_message(&mut self) -> Option<crate::AppMessage> {
        self.out_msg_buffer.pop()
    }
}
