/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * feature/runout.h - Runout sensor support
 */

#include "../sd/cardreader.h"
#include "../module/printcounter.h"
#include "../module/planner.h"
#include "../module/stepper.h" // for block_t
#include "../gcode/queue.h"
#include "../feature/pause.h"

#include "../inc/MarlinConfig.h"

#if ENABLED(EXTENSIBLE_UI)
  #include "../lcd/extui/ui_api.h"
#endif

#if ENABLED(LGT_LCD_DW)
  #include "../lcd/lgtdwlcd.h"
#endif

//#define FILAMENT_RUNOUT_SENSOR_DEBUG
#ifndef FILAMENT_RUNOUT_THRESHOLD
  #define FILAMENT_RUNOUT_THRESHOLD 5
#endif

void event_filament_runout(const uint8_t extruder);

template<class RESPONSE_T, class SENSOR_T>
class TFilamentMonitor;
class FilamentSensorEncoder;
class FilamentSensorSwitch;
class RunoutResponseDelayed;
class RunoutResponseDebounced;

/********************************* TEMPLATE SPECIALIZATION *********************************/

typedef TFilamentMonitor<
          TERN(HAS_FILAMENT_RUNOUT_DISTANCE, RunoutResponseDelayed, RunoutResponseDebounced),
          TERN(FILAMENT_MOTION_SENSOR, FilamentSensorEncoder, FilamentSensorSwitch)
        > FilamentMonitor;

extern FilamentMonitor runout;

/*******************************************************************************************/

class FilamentMonitorBase {
  public:
    static bool enabled, filament_ran_out;

    #if ENABLED(HOST_ACTION_COMMANDS)
      static bool host_handling;
    #else
      static constexpr bool host_handling = false;
    #endif
};

template<class RESPONSE_T, class SENSOR_T>
class TFilamentMonitor : public FilamentMonitorBase {
  private:
    typedef RESPONSE_T response_t;
    typedef SENSOR_T   sensor_t;
    static  response_t response;
    static  sensor_t   sensor;

  public:
    static inline void setup() {
      sensor.setup();
      reset();
    }

    static inline void reset() {
      filament_ran_out = false;
      response.reset();
    }

    // Call this method when filament is present,
    // so the response can reset its counter.
    static inline void filament_present(const uint8_t extruder) {
      response.filament_present(extruder);
    }

    #if HAS_FILAMENT_RUNOUT_DISTANCE
      static inline float& runout_distance() { return response.runout_distance_mm; }
      static inline void set_runout_distance(const_float_t mm) { response.runout_distance_mm = mm; }
    #endif

    // Handle a block completion. RunoutResponseDelayed uses this to
    // add up the length of filament moved while the filament is out.
    static inline void block_completed(const block_t * const b) {
      if (enabled) {
        response.block_completed(b);
        sensor.block_completed(b);
      }
    }

    // Give the response a chance to update its counter.
    static inline void run() {
      if (enabled && !filament_ran_out && (printingIsActive() || did_pause_print)
        #if ENABLED(LGT_LCD_DW)
        	&& LGT_is_printing
        #endif
      ) {
        TERN_(HAS_FILAMENT_RUNOUT_DISTANCE, cli()); // Prevent RunoutResponseDelayed::block_completed from accumulating here
        response.run();
        sensor.run();
        const uint8_t runout_flags = response.has_run_out();
        TERN_(HAS_FILAMENT_RUNOUT_DISTANCE, sei());
        #if MULTI_FILAMENT_SENSOR
          #if ENABLED(WATCH_ALL_RUNOUT_SENSORS)
            const bool ran_out = !!runout_flags;  // any sensor triggers
            uint8_t extruder = 0;
            if (ran_out) {
              uint8_t bitmask = runout_flags;
              while (!(bitmask & 1)) {
                bitmask >>= 1;
                extruder++;
              }
            }
          #else
            const bool ran_out = TEST(runout_flags, active_extruder);  // suppress non active extruders
            uint8_t extruder = active_extruder;
          #endif
        #else
          const bool ran_out = !!runout_flags;
          uint8_t extruder = active_extruder;
        #endif

        #if ENABLED(FILAMENT_RUNOUT_SENSOR_DEBUG)
          if (runout_flags) {
            SERIAL_ECHOPGM("Runout Sensors: ");
            LOOP_L_N(i, 8) SERIAL_ECHO('0' + TEST(runout_flags, i));
            SERIAL_ECHOPGM(" -> ", extruder);
            if (ran_out) SERIAL_ECHOPGM(" RUN OUT");
            SERIAL_EOL();
          }
        #endif

        if (ran_out) {
          filament_ran_out = true;
          event_filament_runout(extruder);
          planner.synchronize();
        }
      }
    }
};

/*************************** FILAMENT PRESENCE SENSORS ***************************/

class FilamentSensorBase {
  protected:
    /**
     * Called by FilamentSensorSwitch::run when filament is detected.
     * Called by FilamentSensorEncoder::block_completed when motion is detected.
     */
    static inline void filament_present(const uint8_t extruder) {
      runout.filament_present(extruder); // ...which calls response.filament_present(extruder)
    }

  public:
    static inline void setup() {
      #define _INIT_RUNOUT_PIN(P,S,U,D) do{ if (ENABLED(U)) SET_INPUT_PULLUP(P); else if (ENABLED(D)) SET_INPUT_PULLDOWN(P); else SET_INPUT(P); }while(0)
      #define  INIT_RUNOUT_PIN(N) _INIT_RUNOUT_PIN(FIL_RUNOUT##N##_PIN, FIL_RUNOUT##N##_STATE, FIL_RUNOUT##N##_PULLUP, FIL_RUNOUT##N##_PULLDOWN)
      #if NUM_RUNOUT_SENSORS >= 1
        INIT_RUNOUT_PIN(1);
      #endif
      #if NUM_RUNOUT_SENSORS >= 2
        INIT_RUNOUT_PIN(2);
      #endif
      #if NUM_RUNOUT_SENSORS >= 3
        INIT_RUNOUT_PIN(3);
      #endif
      #if NUM_RUNOUT_SENSORS >= 4
        INIT_RUNOUT_PIN(4);
      #endif
      #if NUM_RUNOUT_SENSORS >= 5
        INIT_RUNOUT_PIN(5);
      #endif
      #if NUM_RUNOUT_SENSORS >= 6
        INIT_RUNOUT_PIN(6);
      #endif
      #if NUM_RUNOUT_SENSORS >= 7
        INIT_RUNOUT_PIN(7);
      #endif
      #if NUM_RUNOUT_SENSORS >= 8
        INIT_RUNOUT_PIN(8);
      #endif
      #undef _INIT_RUNOUT_PIN
      #undef  INIT_RUNOUT_PIN
    }

    // Return a bitmask of runout pin states
    static inline uint8_t poll_runout_pins() {
      #define _OR_RUNOUT(N) | (READ(FIL_RUNOUT##N##_PIN) ? _BV((N) - 1) : 0)
      return (0 REPEAT_1(NUM_RUNOUT_SENSORS, _OR_RUNOUT));
      #undef _OR_RUNOUT
    }

    // Return a bitmask of runout flag states (1 bits always indicates runout)
    static inline uint8_t poll_runout_states() {
      return poll_runout_pins() ^ uint8_t(0
        #if NUM_RUNOUT_SENSORS >= 1
          | (FIL_RUNOUT1_STATE ? 0 : _BV(1 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 2
          | (FIL_RUNOUT2_STATE ? 0 : _BV(2 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 3
          | (FIL_RUNOUT3_STATE ? 0 : _BV(3 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 4
          | (FIL_RUNOUT4_STATE ? 0 : _BV(4 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 5
          | (FIL_RUNOUT5_STATE ? 0 : _BV(5 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 6
          | (FIL_RUNOUT6_STATE ? 0 : _BV(6 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 7
          | (FIL_RUNOUT7_STATE ? 0 : _BV(7 - 1))
        #endif
        #if NUM_RUNOUT_SENSORS >= 8
          | (FIL_RUNOUT8_STATE ? 0 : _BV(8 - 1))
        #endif
      );
    }
};

#if ENABLED(FILAMENT_MOTION_SENSOR)

  /**
   * This sensor uses a magnetic encoder disc and a Hall effect
   * sensor (or a slotted disc and optical sensor). The state
   * will toggle between 0 and 1 on filament movement. It can detect
   * filament runout and stripouts or jams.
   */
  class FilamentSensorEncoder : public FilamentSensorBase {
    private:
      static uint8_t motion_detected;

      static inline void poll_motion_sensor() {
        static uint8_t old_state;
        const uint8_t new_state = poll_runout_pins(),
                      change    = old_state ^ new_state;
        old_state = new_state;

        #if ENABLED(FILAMENT_RUNOUT_SENSOR_DEBUG)
          if (change) {
            SERIAL_ECHOPGM("Motion detected:");
            LOOP_L_N(e, NUM_RUNOUT_SENSORS)
              if (TEST(change, e)) SERIAL_CHAR(' ', '0' + e);
            SERIAL_EOL();
          }
        #endif

        motion_detected |= change;
      }

    public:
      static inline void block_completed(const block_t * const b) {
        // If the sensor wheel has moved since the last call to
        // this method reset the runout counter for the extruder.
        if (TEST(motion_detected, b->extruder))
          filament_present(b->extruder);

        // Clear motion triggers for next block
        motion_detected = 0;
      }

      static inline void run() { poll_motion_sensor(); }
  };

#else

  /**
   * This is a simple endstop switch in the path of the filament.
   * It can detect filament runout, but not stripouts or jams.
   */
  class FilamentSensorSwitch : public FilamentSensorBase {
    private:
      static inline bool poll_runout_state(const uint8_t extruder) {
        const uint8_t runout_states = poll_runout_states();
        #if MULTI_FILAMENT_SENSOR
          if ( !TERN0(DUAL_X_CARRIAGE, idex_is_duplicating())
            && !TERN0(MULTI_NOZZLE_DUPLICATION, extruder_duplication_enabled)
          ) return TEST(runout_states, extruder); // A specific extruder ran out
        #else
          UNUSED(extruder);
        #endif
        return !!runout_states;                   // Any extruder ran out
      }

    public:
      static inline void block_completed(const block_t * const) {}

      static inline void run() {
        LOOP_L_N(s, NUM_RUNOUT_SENSORS) {
          const bool out = poll_runout_state(s);
          if (!out) filament_present(s);
          #if ENABLED(FILAMENT_RUNOUT_SENSOR_DEBUG)
            static uint8_t was_out; // = 0
            if (out != TEST(was_out, s)) {
              TBI(was_out, s);
              SERIAL_ECHOLNPGM_P(PSTR("Filament Sensor "), '0' + s, out ? PSTR(" OUT") : PSTR(" IN"));
            }
          #endif
        }
      }
  };


#endif // !FILAMENT_MOTION_SENSOR

/********************************* RESPONSE TYPE *********************************/

#if HAS_FILAMENT_RUNOUT_DISTANCE

  // RunoutResponseDelayed triggers a runout event only if the length
  // of filament specified by FILAMENT_RUNOUT_DISTANCE_MM has been fed
  // during a runout condition.
  class RunoutResponseDelayed {
    private:
      static volatile float runout_mm_countdown[NUM_RUNOUT_SENSORS];

    public:
      static float runout_distance_mm;

      static inline void reset() {
        LOOP_L_N(i, NUM_RUNOUT_SENSORS) filament_present(i);
      }

      static inline void run() {
        #if ENABLED(FILAMENT_RUNOUT_SENSOR_DEBUG)
          static millis_t t = 0;
          const millis_t ms = millis();
          if (ELAPSED(ms, t)) {
            t = millis() + 1000UL;
            LOOP_L_N(i, NUM_RUNOUT_SENSORS)
              SERIAL_ECHOPGM_P(i ? PSTR(", ") : PSTR("Remaining mm: "), runout_mm_countdown[i]);
            SERIAL_EOL();
          }
        #endif
      }

      static inline uint8_t has_run_out() {
        uint8_t runout_flags = 0;
        LOOP_L_N(i, NUM_RUNOUT_SENSORS) if (runout_mm_countdown[i] < 0) SBI(runout_flags, i);
        return runout_flags;
      }

      static inline void filament_present(const uint8_t extruder) {
        runout_mm_countdown[extruder] = runout_distance_mm;
      }

      static inline void block_completed(const block_t * const b) {
        if (b->steps.x || b->steps.y || b->steps.z || did_pause_print) { // Allow pause purge move to re-trigger runout state
          // Only trigger on extrusion with XYZ movement to allow filament change and retract/recover.
          const uint8_t e = b->extruder;
          const int32_t steps = b->steps.e;
          runout_mm_countdown[e] -= (TEST(b->direction_bits, E_AXIS) ? -steps : steps) * planner.mm_per_step[E_AXIS_N(e)];
        }
      }
  };

#else // !HAS_FILAMENT_RUNOUT_DISTANCE

  // RunoutResponseDebounced triggers a runout event after a runout
  // condition has been detected runout_threshold times in a row.

  class RunoutResponseDebounced {
    private:
      static constexpr int8_t runout_threshold = FILAMENT_RUNOUT_THRESHOLD;
      static int8_t runout_count[NUM_RUNOUT_SENSORS];

    public:
      static inline void reset() {
        LOOP_L_N(i, NUM_RUNOUT_SENSORS) filament_present(i);
      }

      static inline void run() {
        LOOP_L_N(i, NUM_RUNOUT_SENSORS) if (runout_count[i] >= 0) runout_count[i]--;
      }

      static inline uint8_t has_run_out() {
        uint8_t runout_flags = 0;
        LOOP_L_N(i, NUM_RUNOUT_SENSORS) if (runout_count[i] < 0) SBI(runout_flags, i);
        return runout_flags;
      }

      static inline void block_completed(const block_t * const) { }

      static inline void filament_present(const uint8_t extruder) {
        runout_count[extruder] = runout_threshold;
      }
  };

#endif // !HAS_FILAMENT_RUNOUT_DISTANCE
