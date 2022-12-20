use alloc::vec::Vec;
use microdsp::mpm::MpmPitchDetector;
use micromath::F32Ext;

use crate::{AppMessage, DemoApp};

const FREQUENCY_COUNT: usize = 4;
const FREQUENCIES_TO_DETECT: [f32; FREQUENCY_COUNT] = [
    // Ukulele strings
    440.0, 330.0, 262.0, 392.0
];
const MAX_FREQ_ERROR: f32 = 1.0;
const OUT_MSG_BUFFER_SIZE: usize = 32;
const DOWNSAMPLING: usize = 2;
const WINDOW_SIZE: usize = 256 / DOWNSAMPLING;
const LAG_COUNT: usize = WINDOW_SIZE / 2;
const HOP_SIZE: usize = WINDOW_SIZE;

pub struct MpmDemoApp {
    detector: MpmPitchDetector,
    out_msg_buffer: Vec<AppMessage>,
    detection_states: [bool; FREQUENCY_COUNT],
}

impl DemoApp for MpmDemoApp {
    fn new(sample_rate: f32) -> Self {
        MpmDemoApp {
            detector: MpmPitchDetector::from_options(
                sample_rate,
                WINDOW_SIZE,
                HOP_SIZE,
                LAG_COUNT,
                DOWNSAMPLING,
            ),
            out_msg_buffer: Vec::with_capacity(OUT_MSG_BUFFER_SIZE),
            detection_states: [false; 4],
        }
    }
    fn process(&mut self, rx: &[f32], _: &mut [f32]) {
        let led_msgs = [
            (AppMessage::Led0Off, AppMessage::Led0On),
            (AppMessage::Led1Off, AppMessage::Led0On),
            (AppMessage::Led2Off, AppMessage::Led0On),
            (AppMessage::Led3Off, AppMessage::Led0On),
        ];

        let out_msg_buffer = &mut self.out_msg_buffer;
        self.detector.process(rx, |result| {
            let result_is_tone = result.is_tone();
            for (i, f) in FREQUENCIES_TO_DETECT.iter().enumerate() {
                let freq_error = F32Ext::abs(result.frequency - *f);
                let tone_decected = result_is_tone && freq_error < MAX_FREQ_ERROR;
                if !self.detection_states[i] && tone_decected {
                    // Started
                    if out_msg_buffer.len() < OUT_MSG_BUFFER_SIZE {
                        out_msg_buffer.push(led_msgs[i].1)
                    }
                } else if self.detection_states[i] && !tone_decected {
                    // Ended
                    if out_msg_buffer.len() < OUT_MSG_BUFFER_SIZE {
                        out_msg_buffer.push(led_msgs[i].0)
                    }
                }
                self.detection_states[i] = tone_decected;
            }
        });
    }

    fn handle_message(&mut self, _: crate::AppMessage) {}

    fn next_outgoing_message(&mut self) -> Option<crate::AppMessage> {
        self.out_msg_buffer.pop()
    }
}
