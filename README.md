# STM32 Data Logger V1

Target: STM32F103C8T6 / genericSTM32F103V8, STM32Cube HAL, PlatformIO.

## Hardware mapping

- I2C1: PB6=SCL, PB7=SDA
- USART1: PA9=TX, PA10=RX
- SPI1: PA5=SCK, PA6=MISO, PA7=MOSI, PA4=SD CS
- DS1302: PB12=CLK, PB13=DAT, PB14=RST/CE
- BME280: I2C address 0x76
- SSD1306: I2C address 0x3C

## FatFs

FatFs is middleware, not the SD driver. This project supplies the SD SPI block driver and the FatFs `diskio.c` glue. The FatFs core (`ff.c`, `ff.h`, `diskio.h`, configuration) is copied from the STM32CubeF1 package already installed by PlatformIO, so the project uses a matching vendor version.

After the first PlatformIO build has downloaded the framework, run from the project root:

    ./tools/prepare_fatfs.sh

The script copies the matching `ff.c`, `ff.h`, `diskio.h` and `ffconf.h`/template into `lib/fatfs/src`.

The ST/STM32CubeF1 package includes FatFs middleware; PlatformIO documents that STM32Cube framework packages can supply middleware components. The STM32CubeF1 package itself includes FatFs among its middleware. Do not copy the vendor `diskio.c`; this project has its own `src/fatfs/diskio.c`.

## Build

    pio run

Upload:

    pio run -t upload

Monitor:

    pio device monitor -b 115200

## RTC

If the DS1302 contains an invalid time, the firmware applies the default values in `include/app_config.h`.
Change `RTC_DEFAULT_*` before the first run if needed.

## SD

The SD module is expected to provide proper 3.3 V logic-level compatibility on SPI even if its VCC input is 5 V. Verify the exact module before connecting it.

## Log

The logger creates/appends:

    LOG0001.CSV

Header:

    timestamp,sequence,temperature_c,humidity_percent,pressure_pa

The file is synced after each record in V1 to prioritize data durability.
