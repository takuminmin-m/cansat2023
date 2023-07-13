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
        let servo = Servo{
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
        // min: top_val * 0.025 = 665
        // max: top_val * 0.116 = 3085
        // let d = 665 + ((self.position + 90) * 2240) / 90;
        let d = self.position;
        self.pwm_channel.set_duty((d as u16).into());
    }
}
