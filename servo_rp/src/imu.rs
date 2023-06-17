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

pub fn init(i2c: &mut I2C<I2C1, (Sda, Scl)>) {
    // 加速度センサーの初期化
    i2c.write(IMU_ADDR_ACCL, &[0x0f, 0x03]).unwrap();
    i2c.write(IMU_ADDR_ACCL, &[0x10, 0x08]).unwrap();
    i2c.write(IMU_ADDR_ACCL, &[0x11, 0x00]).unwrap();

    // ジャイロセンサーの初期化
    i2c.write(IMU_ADDR_GYRO, &[0x0f, 0x04]).unwrap();
    i2c.write(IMU_ADDR_GYRO, &[0x10, 0x07]).unwrap();
    i2c.write(IMU_ADDR_GYRO, &[0x11, 0x00]).unwrap();

    // 地磁気センサーの初期化
    i2c.write(IMU_ADDR_MAG, &[0x4b, 0x83]).unwrap();
    i2c.write(IMU_ADDR_MAG, &[0x4b, 0x01]).unwrap();
    i2c.write(IMU_ADDR_MAG, &[0x4c, 0x00]).unwrap();
    i2c.write(IMU_ADDR_MAG, &[0x4e, 0x84]).unwrap();
    i2c.write(IMU_ADDR_MAG, &[0x51, 0x04]).unwrap();
    i2c.write(IMU_ADDR_MAG, &[0x52, 0x16]).unwrap();

    info!("IMU init done");
}
