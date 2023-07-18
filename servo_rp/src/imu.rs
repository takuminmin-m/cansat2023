use bsp::Scl;
use bsp::Sda;
use bsp::pac::I2C1;
use defmt::info;
use fugit::RateExtU32;
use seeeduino_xiao_rp2040 as bsp;
use bsp::hal::{ i2c::I2C, gpio::Pins, pac, Sio };

use embedded_hal::prelude::_embedded_hal_blocking_i2c_Read;
use embedded_hal::prelude::_embedded_hal_blocking_i2c_Write;
use embedded_hal::prelude::_embedded_hal_blocking_i2c_WriteRead;

const IMU_ADDR_ACCL: u8 = 0x19;
const IMU_ADDR_GYRO: u8 = 0x69;
const IMU_ADDR_MAG: u8 = 0x13;

pub struct Imu<'a> {
    i2c: &'a mut I2C<I2C1, (Sda, Scl)>,

    pub x_accl: f32,
    pub y_accl: f32,
    pub z_accl: f32,

    pub x_gyro: f32,
    pub y_gyro: f32,
    pub z_gyro: f32,

    pub x_mag: i32,
    pub y_mag: i32,
    pub z_mag: i32,
}

impl<'a> Imu<'a> {
    pub fn new(i2c: &'a mut I2C<I2C1, (Sda, Scl)>, delay: &mut cortex_m::delay::Delay) -> Imu<'a> {
        let imu = Imu{
            i2c,
            x_accl: 0.0,
            y_accl: 0.0,
            z_accl: 0.0,
            x_gyro: 0.0,
            y_gyro: 0.0,
            z_gyro: 0.0,
            x_mag: 0,
            y_mag: 0,
            z_mag: 0,
        };

        // 加速度センサーの初期化
        imu.i2c.write(IMU_ADDR_ACCL, &[0x0f, 0x03]).unwrap();
        imu.i2c.write(IMU_ADDR_ACCL, &[0x10, 0x08]).unwrap();
        imu.i2c.write(IMU_ADDR_ACCL, &[0x11, 0x00]).unwrap();
        delay.delay_ms(500);

        // ジャイロセンサーの初期化
        imu.i2c.write(IMU_ADDR_GYRO, &[0x0f, 0x04]).unwrap();
        imu.i2c.write(IMU_ADDR_GYRO, &[0x10, 0x07]).unwrap();
        imu.i2c.write(IMU_ADDR_GYRO, &[0x11, 0x00]).unwrap();
        delay.delay_ms(500);

        // 地磁気センサーの初期化
        imu.i2c.write(IMU_ADDR_MAG, &[0x4b, 0x83]).unwrap();
        imu.i2c.write(IMU_ADDR_MAG, &[0x4b, 0x01]).unwrap();
        imu.i2c.write(IMU_ADDR_MAG, &[0x4c, 0x00]).unwrap();
        imu.i2c.write(IMU_ADDR_MAG, &[0x4e, 0x84]).unwrap();
        imu.i2c.write(IMU_ADDR_MAG, &[0x51, 0x04]).unwrap();
        imu.i2c.write(IMU_ADDR_MAG, &[0x52, 0x16]).unwrap();
        delay.delay_ms(500);

        info!("IMU init done");
        imu
    }

    pub fn update_all(&mut self) {
        self.update_accl();
        self.update_gyro();
        self.update_mag();
    }

    pub fn update_accl(&mut self) {
        let mut data = [0_u8; 6];

        for i in 0..6 {
            let mut buf = [0_u8, 1];
            self.i2c.write_read(IMU_ADDR_ACCL, &[2+i,], &mut buf).unwrap();

            data[i as usize] = buf[0];
        }

        self.x_accl = Self::convert_accl(data[0], data[1]);
        self.y_accl = Self::convert_accl(data[2], data[3]);
        self.z_accl = Self::convert_accl(data[4], data[5]);
    }

    fn convert_accl(byte0: u8, byte1: u8) -> f32 {
        let mut accl = (byte1 as u32 * 256 + ((byte0 & 0xf0)) as u32) as f32 / 16.0;
        if accl > 2047.0 {
            accl -= 4096.0;
        }
        accl * 0.0098
    }


    pub fn update_gyro(&mut self) {
        let mut data = [0_u8; 6];

        for i in 0..6 {
            let mut buf = [0_u8, 1];
            self.i2c.write_read(IMU_ADDR_GYRO, &[2+i,], &mut buf).unwrap();

            data[i as usize] = buf[0];
        }

        self.x_gyro = Self::convert_gyro(data[0], data[1]);
        self.y_gyro = Self::convert_gyro(data[2], data[3]);
        self.z_gyro = Self::convert_gyro(data[4], data[5]);
    }

    fn convert_gyro(byte0: u8, byte1: u8) -> f32 {
        let mut gyro = byte1 as i32 * 256 + byte0 as i32;
        if gyro > 32767 {
            gyro -= 65536;
        }

        gyro as f32 * 0.0038
    }

    pub fn update_mag(&mut self) {
        let mut data = [0_u8; 8];

        for i in 0..8 {
            let mut buf = [0_u8, 1];
            self.i2c.write_read(IMU_ADDR_MAG, &[0x42+i,], &mut buf).unwrap();

            data[i as usize] = buf[0];
        }

        self.x_mag = Self::convert_mag_xy(data[0], data[1]);
        self.y_mag = Self::convert_mag_xy(data[2], data[3]);
        self.z_mag = Self::convert_mag_z(data[4], data[5]);
    }

    fn convert_mag_xy(byte0: u8, byte1: u8) -> i32 {
        let byte0_i32 = byte0 as i32;
        let byte1_i32 = byte1 as i32;
        let mut mag = (byte1_i32 << 8) | (byte0_i32 >> 3);
        if mag > 4095 {
            mag -= 8192;
        }
        mag
    }

    fn convert_mag_z(byte0: u8, byte1: u8) -> i32 {
        let byte0_i32 = byte0 as i32;
        let byte1_i32 = byte1 as i32;
        let mut mag = (byte1_i32 << 8) | (byte0_i32 >> 3);
        if mag > 16383 {
            mag -= 32768;
        }
        mag
    }
}
