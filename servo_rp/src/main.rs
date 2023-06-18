#![no_std]
#![no_main]

use bsp::entry;
use defmt::*;
use defmt_rtt as _;
use embedded_hal::digital::v2::OutputPin;
use panic_probe as _;
use fugit::RateExtU32;

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
};

mod imu;

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

    let sda_pin = pins.sda;
    let scl_pin = pins.scl;

    let mut i2c = I2C::i2c1(
        pac.I2C1,
        sda_pin.into_mode(),
        scl_pin.into_mode(),
        400.kHz(),
        &mut pac.RESETS,
        125_000_000.Hz(),
    );

    imu::init(&mut i2c);

    // This is the correct pin on the Raspberry Pico board. On other boards, even if they have an
    // on-board LED, it might need to be changed.
    // Notably, on the Pico W, the LED is not connected to any of the RP2040 GPIOs but to the cyw43 module instead. If you have
    // a Pico W and want to toggle a LED with a simple GPIO output pin, you can connect an external
    // LED to one of the GPIO pins, and reference that pin here.
    let mut led_blue_pin = pins.led_blue.into_push_pull_output();
    let mut led_green_pin = pins.led_green.into_push_pull_output();
    let mut led_red_pin = pins.led_red.into_push_pull_output();

    loop {
        info!("on!");
        led_blue_pin.set_high().unwrap();
        led_green_pin.set_high().unwrap();
        led_red_pin.set_high().unwrap();
        delay.delay_ms(500);
        info!("off!");
        led_blue_pin.set_low().unwrap();
        led_green_pin.set_low().unwrap();
        led_red_pin.set_low().unwrap();
        delay.delay_ms(500);
    }
}
