# Dev-Libraries
The main repository for RTU HPR Team libraries

# Instalation
### Add **https://github.com/RTU-HPR/Dev-Libraries** to lib_deps in *platformio.ini* file
It is recommended that you specify the used version of the library. This way you can make sure that the functionality will always stay the same until you choose to use a different version.
Currently, the recommended way is to choose a specific commit in the main (stable) branch and copy its hash which you append to the repo link in the platformio.ini file.
> libdeps =
>   https://github.com/RTU-HPR/Dev-Libraries#1161f3706fcd53de7609e7d96142f3f91c97b18b   <- __REPLACE WITH THE REQUIRED COMMIT HASH__

# Library overview
## RadioLib wrapper
### Tested radio modules
- RFM96W - Everything works as expected
- SX1268 - Everything works as expected

## SD card wrapper
### Tested platforms
- PI PICO (earlephilhower) - Works as expected tested on:
    - PFC V1
- ESP32  - Untested

## Ranging wrapper
Does ranging calls and calculates the masters position based on multiple ping locaitons.
The calculation part currently isn't tested and properly implemented (don't use)
### Tested radio modules
- SX1280 - Works as expected
