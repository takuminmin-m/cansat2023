use embedded_hal::PwmPin;
use seeeduino_xiao_rp2040 as bsp;
use bsp::hal::pwm::{  Channel, SliceId, ChannelId, SliceMode, ValidSliceMode };


pub struct Servo<'a, T: PwmPin>
where
    <T as PwmPin>::Duty: From<u16>,
{
    pwm_channel: &'a mut T,
    pub position: i32,
}

impl<'a, T: PwmPin> Servo<'a, T>
where
    <T as PwmPin>::Duty: From<u16>,
{
    pub fn new(pwm_channel: &'a mut T) -> Servo<'a, T> {
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
        self.pwm_channel.set_duty((d as u16).into());
    }
}
