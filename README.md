# LK5-Dblower-BLtouch-Marlin2.0-Firmware

## Build

As the firmware is based on Marlin2.0.x which is built on the core of PlatformIO, the buid compiling steps are the same as Marlin2.0.x. You can directly using [PlatformIO Shell Commands](https://docs.platformio.org/en/latest/core/installation.html#piocore-install-shell-commands), or using IDEs contain built-in PlatformIO Core(CLI), for example, [VSCode](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode) and [Atom](https://docs.platformio.org/en/latest/integration/ide/atom.html). VSCode is recommended.

## Update firmware:

- Enter the `.pio\build\mks_robin_nano35` directory, change the `Robin_nano35.bin` to `firmware.bin` and copy to the sd card
- Insert SD card to the motherboard, and you can see the update interface after power on.   
