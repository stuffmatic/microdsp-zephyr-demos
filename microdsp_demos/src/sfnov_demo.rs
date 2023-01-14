use alloc::vec::Vec;
use microdsp::{
    common::WindowFunctionType,
    sfnov::{HardKneeCompression, SpectralFluxNoveltyDetector},
};

use crate::{AppMessage, DemoApp};

const DOWNSAMPLING: usize = 4;
const WINDOW_SIZE: usize = 256 / DOWNSAMPLING;
const HOP_SIZE: usize = WINDOW_SIZE;
const DETECTION_THRESHOLD: f32 = 0.005;
const OUT_MSG_BUFFER_SIZE: usize = 16;

pub struct SfnovDemoApp {
    detector: SpectralFluxNoveltyDetector<HardKneeCompression>,
    should_trigger: bool,
    out_msg_buffer: Vec<AppMessage>,
    led_state: bool,
}

impl DemoApp for SfnovDemoApp {
    fn new(_: f32) -> Self {
        let comp_func = HardKneeCompression::from_options(0.05, 0.9);
        SfnovDemoApp {
            detector: SpectralFluxNoveltyDetector::from_options(
                WindowFunctionType::Hann,
                comp_func,
                WINDOW_SIZE,
                DOWNSAMPLING,
                HOP_SIZE,
            ),
            should_trigger: true,
            out_msg_buffer: Vec::with_capacity(OUT_MSG_BUFFER_SIZE),
            led_state: false,
        }
    }
    fn process(&mut self, rx: &[f32], _: &mut [f32]) {
        let should_trigger = &mut self.should_trigger;
        let out_msg_buffer = &mut self.out_msg_buffer;

        self.detector.process(rx, |result| {
            if result.novelty() > DETECTION_THRESHOLD {
                if *should_trigger {
                    self.led_state = !self.led_state;
                    out_msg_buffer.push(if self.led_state {
                        AppMessage::Led0On
                    } else {
                        AppMessage::Led0Off
                    });
                    *should_trigger = false;
                }
            } else {
                *should_trigger = true;
            }
        })
    }
    fn handle_message(&mut self, _: crate::AppMessage) {}
    fn next_outgoing_message(&mut self) -> Option<crate::AppMessage> {
        self.out_msg_buffer.pop()
    }
}
