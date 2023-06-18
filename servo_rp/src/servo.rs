use seeeduino_xiao_rp2040 as bsp;
use bsp::{hal::{ prelude::*, pwm::{FreeRunning, InputHighRunning, Slices, Channel, Pwm5}, gpio::{Pin, Function, OptionalPinId} }, pac::Peripherals };

use embedded_hal::{PwmPin};

pub struct Servo<'a> {
    pwm_channel: &'a mut Channel<Pwm5, FreeRunning, bsp::hal::pwm::A>,
    pub position: i32,
}

impl<'a> Servo<'a> {
    pub fn new(pwm_channel: &'a mut Channel<Pwm5, FreeRunning, bsp::hal::pwm::A>) -> Servo<'a> {
        let mut servo = Servo{
            pwm_channel,
            position: 0,
        };

        servo
    }

    pub fn set_position(&mut self, position: i32) {
        self.position = position;
        self.rotate();
    }

    fn rotate(&mut self) {
        let d = (self.position + 90) * 150;
        self.pwm_channel.set_duty(d as u16);
    }
}
