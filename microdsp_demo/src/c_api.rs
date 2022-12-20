use crate::{AppMessage, DemoApp, DemoAppType};
use alloc::{boxed::Box, slice, vec, vec::Vec};
use core::mem::transmute;

#[no_mangle]
pub extern "C" fn demo_app_create(sample_rate: f32) -> *mut DemoAppType {
    let vec: Vec<f32> = vec![0.0; 10];
    let app = DemoAppType::new(sample_rate);
    unsafe { transmute(Box::new(app)) }
}

#[no_mangle]
pub fn demo_app_process(
    demo_app_ptr: *mut DemoAppType,
    tx_ptr: *mut f32,
    rx_ptr: *const f32,
    sample_count: usize,
) {
    let (demo_app, tx, rx) = unsafe {
        (
            &mut *demo_app_ptr,
            slice::from_raw_parts_mut(tx_ptr, sample_count),
            slice::from_raw_parts(rx_ptr, sample_count),
        )
    };
    demo_app.process(rx, tx);
}

#[no_mangle]
pub fn demo_app_handle_message(demo_app_ptr: *mut DemoAppType, message: AppMessage) {
    let demo_app = unsafe { &mut *demo_app_ptr };
    demo_app.handle_message(message);
}

#[no_mangle]
pub fn demo_app_next_outgoing_message(demo_app_ptr: *mut DemoAppType) -> u8 {
    let demo_app = unsafe { &mut *demo_app_ptr };
    if let Some(message) = demo_app.next_outgoing_message() {
        message as u8
    } else {
        0
    }
}
