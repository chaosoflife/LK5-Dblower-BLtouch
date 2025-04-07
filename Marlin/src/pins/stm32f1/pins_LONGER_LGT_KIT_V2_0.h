/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * Longer3D LK1/LK2 & Alfawise U20/U30 (STM32F103VET6) board pin assignments
 */

#if NOT_TARGET(__STM32F1__, STM32F1xx)
  #error "Oops! Select a STM32F1 board in 'Tools > Board.'"
#elif HOTENDS > 1 || E_STEPPERS > 1
  #error "Longer3D only supports one hotend / E-stepper. Comment out this line to continue."
#elif HAS_FSMC_TFT
  #error "MKS Robin nano v2 doesn't support FSMC-based TFT displays."
#endif

#define BOARD_INFO_NAME "LGT_KIT_V2.0"

#define BOARD_NO_NATIVE_USB // use IC(CH34X) rather than USB CDC

// Avoid conflict with TIMER_SERVO when using the STM32 HAL
#define TEMP_TIMER                             5

//#define DISABLE_DEBUG                           //  We still want to debug with STLINK...
#define DISABLE_JTAG                              //  We free the jtag pins (PA15) but keep STLINK
                                                  //  Release PB4 (STEP_X_PIN) from JTAG NRST role.
//
// Limit Switches
//
#define X_STOP_PIN                           PE6   // pin 16
// #define X_MAX_PIN                           PE5   // pin 15 (Filament sensor on Alfawise setup)
#define Y_STOP_PIN                           PE4  // pin 9
// #define Y_MAX_PIN                           PE3  // pin 8 (Unused in stock Alfawise setup)
#define Z_STOP_PIN                           PE2   // pin 5 Standard Endstop or Z_Probe endstop function
// #define Z_MAX_PIN                           PE1   // pin 4 (Unused in stock Alfawise setup)
                                 // May be used for BLTouch Servo function on older variants (<= V08)
#ifndef Z_MIN_PROBE_PIN
  #define Z_MIN_PROBE_PIN                   PE1   // BLTouch IN
#endif

#define ONBOARD_ENDSTOPPULLUPS

//
// Filament Sensor
//
#ifndef FIL_RUNOUT_PIN
  #define FIL_RUNOUT_PIN                    PE5   // XMAX plug on PCB used as filament runout sensor on Alfawise boards (inverting true)
#endif

//
// Steppers
//
#define E0_ENABLE_PIN                       PE0   // pin 3
#define E0_STEP_PIN                         PB9   // pin 2
#define E0_DIR_PIN                          PB8//PE2   // pin 1

#define X_ENABLE_PIN                        PD4   // pin 91
#define X_STEP_PIN                          PD3   // pin 90
#define X_DIR_PIN                           PD2//PB3   // pin 89

#define Y_ENABLE_PIN                        PD7   // pin 95
#define Y_STEP_PIN                          PD6//PB7   // pin 93
#define Y_DIR_PIN                           PD5//PB6   // pin 92

#define Z_ENABLE_PIN                        PB5   // pin 98
#define Z_STEP_PIN                          PB4//PE0   // pin 97
#define Z_DIR_PIN                           PB3//PB9   // pin 96



//
// Temperature Sensors
//
#define TEMP_0_PIN                          PC1   // pin 23 (Nozzle 100K/3950 thermistor)
#define TEMP_BED_PIN                        PC0   // pin 24 (Hot Bed 100K/3950 thermistor)

//
// Heaters / Fans
//
#define HEATER_0_PIN                        PA7//PD3   // pin 84 (Nozzle Heat Mosfet)
#define HEATER_BED_PIN                      PA0//PA8   // pin 67 (Hot Bed Mosfet)

#define FAN_PIN                             PA1//PA15  // pin 77 (4cm Fan)

#define FAN_SOFT_PWM                        
#define FAN_MIN_PWM                         80
#define FAN_MAX_PWM                         255

// 
// LED for running
//
#define LED_PIN                             PD0//PC2   // pin 17

//
// PWM for a servo probe
//
#if HAS_Z_SERVO_PROBE
  #define SERVO0_PIN                        PA8//PD13  // Open drain PWM pin on the V0G (GND or floating 5V)
#endif


//
// Persistent Storage
// If no option is selected below the I2C_EEPROM will be used

#define I2C_EEPROM
// #define FLASH_EEPROM_EMULATION
//#define SDCARD_EEPROM_EMULATION

#if ENABLED(FLASH_EEPROM_EMULATION)
  #define EEPROM_PAGE_SIZE     (0x800U)           // 2KB
  #define EEPROM_START_ADDRESS (0x8000000UL + (STM32_FLASH_SIZE) * 1024UL - (EEPROM_PAGE_SIZE) * 2UL)
  #define MARLIN_EEPROM_SIZE (EEPROM_PAGE_SIZE)
#elif EITHER(I2C_EEPROM, NO_EEPROM_SELECTED)
  #if DISABLED(I2C_EEPROM)
    #define I2C_EEPROM
  #endif

  #define SOFT_I2C_EEPROM                              // Force the use of Software I2C
  #define MARLIN_EEPROM_SIZE (0x800U)                  // AT24C16C 16 Kbit = 2 KBytes
  #define I2C_SDA_PIN                      SDA
  #define I2C_SCL_PIN                      SCL
  #undef NO_EEPROM_SELECTED
#endif

//
// SD Card
//
#ifndef SDCARD_CONNECTION
  #define SDCARD_CONNECTION                 ONBOARD
#endif

  #define SPI_DEVICE                        2
  #define SD_SCK_PIN                        PB13
  #define SD_MISO_PIN                       PB14
  #define SD_MOSI_PIN                       PB15
  #define SD_SS_PIN                         PB12
  #define SDSS                              SD_SS_PIN
  #define SD_DETECT_PIN                     PB11

// #if SD_CONNECTION_IS(ONBOARD)
//   #define SD_DETECT_PIN                     PB11
//   #define SD_SCK_PIN                        PB13
//   #define SD_MISO_PIN                       PB14
//   #define SD_MOSI_PIN                       PB15

//   #define ONBOARD_SPI_DEVICE                     2  // SPI1
//   #define ONBOARD_SD_CS_PIN                   PB12   // Chip select for "System" SD card
//   #define SDSS                              ONBOARD_SD_CS_PIN
// #endif