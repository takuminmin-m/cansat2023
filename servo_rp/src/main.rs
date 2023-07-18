#![no_std]
#![no_main]

use bsp::entry;
use defmt::*;
use defmt_rtt as _;
use embedded_hal::digital::v2::OutputPin;
use panic_probe as _;
use fugit::RateExtU32;
use core::fmt::Write;

// Provide an alias for our BSP so we can switch targets quickly.
// Uncomment the BSP you included in Cargo.toml, the rest of the code does not need to change.
// use rp_pico as bsp;
// use sparkfun_pro_micro_rp2040 as bsp;
use seeeduino_xiao_rp2040 as bsp;

use bsp::hal::{
    i2c::I2C,
    gpio::Pins,
    clocks::{init_clocks_and_plls, Clock},
    pac,
    sio::Sio,
    watchdog::Watchdog,
    uart::{ UartPeripheral, UartConfig, DataBits, StopBits, },
    prelude::*,
    pwm::{InputHighRunning, Slices},
};

use embedded_hal::PwmPin;

mod imu;
use crate::imu::Imu;

mod servo;
use crate::servo::Servo;

#[entry]
fn main() -> ! {
    info!("Program start");
    let mut pac = pac::Peripherals::take().unwrap();
    let core = pac::CorePeripherals::take().unwrap();
    let mut watchdog = Watchdog::new(pac.WATCHDOG);
    let sio = Sio::new(pac.SIO);

    // External high-speed crystal on the pico board is 12Mhz
    let external_xtal_freq_hz = 12_000_000u32;
    let clocks = init_clocks_and_plls(
        external_xtal_freq_hz,
        pac.XOSC,
        pac.CLOCKS,
        pac.PLL_SYS,
        pac.PLL_USB,
        &mut pac.RESETS,
        &mut watchdog,
    )
    .ok()
    .unwrap();

    let mut delay = cortex_m::delay::Delay::new(core.SYST, clocks.system_clock.freq().to_Hz());

    let pins = bsp::Pins::new(
        pac.IO_BANK0,
        pac.PADS_BANK0,
        sio.gpio_bank0,
        &mut pac.RESETS,
    );

    let mut led_blue_pin = pins.led_blue.into_push_pull_output();
    let mut led_green_pin = pins.led_green.into_push_pull_output();
    let mut led_red_pin = pins.led_red.into_push_pull_output();
    led_blue_pin.set_high().unwrap();
    led_green_pin.set_high().unwrap();
    led_red_pin.set_high().unwrap();

    let mut uart = UartPeripheral::new(
        pac.UART0,
        (pins.tx.into_mode(), pins.rx.into_mode()),
        &mut pac.RESETS,
    ).enable(
        UartConfig::new(
            115200.Hz(),
            DataBits::Eight,
            None,
            StopBits::One
        ),
        clocks.peripheral_clock.freq(),
    ).unwrap();

    uart.write_full_blocking(b"Start\r\n");

    uart.write_full_blocking(b"I2C initializing.....");
    let sda_pin = pins.sda;
    let scl_pin = pins.scl;
    let mut i2c = I2C::i2c1(
        pac.I2C1,
        sda_pin.into_mode(),
        scl_pin.into_mode(),
        400.kHz(),
        &mut pac.RESETS,
        clocks.system_clock.freq(),
    );
    uart.write_full_blocking(b"done!\r\n");


    uart.write_full_blocking(b"PWM initializing.....");
    let mut pwm_slices = Slices::new(pac.PWM, &mut pac.RESETS);
    let pwm = &mut pwm_slices.pwm5;
    pwm.set_ph_correct();
    pwm.enable();

    // set frequency for 50Hz
    pwm.set_top(19531);
    pwm.set_div_int(64);
    pwm.set_div_frac(0);

    let channel_a = &mut pwm.channel_a;
    channel_a.output_to(pins.a0);
    uart.write_full_blocking(b"done!\r\n");

    uart.write_full_blocking(b"IMU device initializing.....");
    let mut imu = imu::Imu::new(&mut i2c, &mut delay);
    uart.write_full_blocking(b"done!\r\n");

    uart.write_full_blocking(b"servo device initializing.....");
    let mut servo = servo::Servo::new(channel_a);
    servo.set_position(0);
    uart.write_full_blocking(b"done!\r\n");

    uart.write_full_blocking(b"Init done\r\n");
    led_green_pin.set_low().unwrap();

    let mut servo_pos = 0.0;
    loop {
        imu.update_gyro();
        let z_gyro = imu.z_gyro;
        servo_pos -= z_gyro / 1000.0;
        servo.set_position(servo_pos as i32);
    }
}
