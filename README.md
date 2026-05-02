# Smart Red Light Violation Detection System

Embedded real-time system on the **TI Tiva C TM4C123GXL Launchpad** that
detects vehicles crossing a stop line during a red light phase, captures
photographic evidence (placeholder for ESP32-CAM integration), fires an
audio/visual alert, and logs each violation with a timestamp.

Built with **FreeRTOS** — 6 concurrent tasks across 4 priority levels,
queue-based inter-task communication, fan-out dispatch — and the bare
TivaWare driverlib. No IDE required.

## What it does

- Cycles the onboard RGB LED through GREEN (6s) → YELLOW (2s) → RED (5s) on a software timer
- Continuously polls an HC-SR04 ultrasonic sensor for objects within 30 cm
- Applies a software pedestrian filter: only triggers if an object lingers for 150-2000 ms
- During the RED phase, a triggered detection produces a violation event
- Each violation fans out to three handlers: alert (visual flicker), camera (capture request), log (UART message)
- During GREEN or YELLOW, detections are silently ignored (legal pass)

## Hardware

| Component | Where it goes |
|---|---|
| TI Tiva C TM4C123GXL Launchpad | USB to PC (debug port — upper micro-USB) |
| HC-SR04 ultrasonic sensor | 4-pin breakout, 3.3V power |
| 4× female-to-male jumper wires | sensor → Tiva headers |

### Wiring

Read silkscreen labels on the Tiva. Don't count pins.

| HC-SR04 pin | Tiva silkscreen |
|---|---|
| VCC | `+3.3V` |
| TRIG | `PB0` |
| ECHO | `PB1` |
| GND | `GND` |

> **Important:** Power the HC-SR04 from **3.3V**, not 5V. Echo line stays
> within PB1's safe input range. Range still reaches well over a meter.

The onboard RGB LED handles the traffic light cycle and the violation
flicker — no external LEDs needed for the current build.

## Architecture (6 FreeRTOS tasks)

```
                          ┌────────────────┐
                          │   LightTask    │  prio 3
                          │  GREEN→YELLOW  │  RGB on PF1/2/3
                          │   →RED cycle   │  writes g_current_light
                          └────────────────┘

                                 reads
                                   │
┌────────────┐  sensor_event_t   ┌─▼──────────────┐  violation_record_t
│ SensorTask │ ───────────────▶  │ ViolationTask  │ ─────┬─────────────▶ AlertTask  (prio 3)
│  (prio 1)  │  xSensorEvent     │   (prio 4)     │      │              ├─▶ CameraTask (prio 2)
│ HC-SR04    │      Queue        │ pure dispatch  │      │              └─▶ LogTask    (prio 2)
│ PB0/PB1    │                   └────────────────┘    queues:
└────────────┘                                          xAlertQueue
                                                        xCameraQueue
                                                        xLogQueue
```

- **SensorTask** — polls HC-SR04 at 50 Hz, applies dwell-time pedestrian filter, posts `SENSOR_VEHICLE_DETECTED` events
- **LightTask** — owns the RGB LED, cycles phases on software timers, sets `g_current_light`
- **ViolationTask** — pure dispatcher: consumes sensor events, checks light state, fans out violation records to three queues. No GPIO. No UART.
- **AlertTask** — turns violation records into physical alerts (today: flicker; tomorrow: flash LED + buzzer)
- **CameraTask** — turns violation records into camera capture commands (today: log placeholder; tomorrow: ESP32-CAM UART protocol)
- **LogTask** — turns violation records into timestamped UART output

`g_current_light` is a `volatile light_state_t` shared variable — single
writer (LightTask), single reader (ViolationTask), no mutex required.

## Software prerequisites (Linux)

Tested on Ubuntu 22.04 and Linux Mint 21.

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi \
                    libnewlib-arm-none-eabi build-essential git make \
                    libusb-1.0-0-dev pkg-config picocom
```

### Tiva flasher

```bash
cd ~
git clone https://github.com/utzig/lm4tools.git
cd lm4tools/lm4flash
make
sudo cp lm4flash /usr/local/bin/
```

### USB permissions (one-time)

```bash
sudo usermod -aG dialout,plugdev $USER

sudo tee /etc/udev/rules.d/99-tiva-launchpad.rules >/dev/null <<'UDEV'
SUBSYSTEM=="usb", ATTRS{idVendor}=="1cbe", ATTRS{idProduct}=="00fd", \
    GROUP="plugdev", MODE="0660"
KERNEL=="ttyACM*", ATTRS{idVendor}=="1cbe", ATTRS{idProduct}=="00fd", \
    GROUP="dialout", MODE="0660"
UDEV

sudo udevadm control --reload-rules
```

**Log out and back in** for the group change to apply.

### TivaWare

Free TI account required. Download `SW-TM4C-2.2.0.295.exe` from
[ti.com/tool/SW-TM4C](https://www.ti.com/tool/SW-TM4C) (it's a self-extracting zip).

```bash
mkdir -p ~/ti/tivaware
cd ~/ti/tivaware
# move the .exe here, then:
unzip SW-TM4C-2.2.0.295.exe
make    # builds libdriver.a
```

### FreeRTOS kernel

```bash
cd ~
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
cd FreeRTOS-Kernel
git checkout V11.1.0
```

The Makefile expects TivaWare at `~/ti/tivaware` and FreeRTOS at
`~/FreeRTOS-Kernel`. Override:

```bash
make TIVAWARE=/your/path FREERTOS=/your/path
```

## Build and flash

```bash
git clone git@github.com:MarwanMoh11/smart-redlight-tm4c123.git
cd smart-redlight-tm4c123

make           # produces build/redlight.bin
make flash     # writes to a connected Tiva
```

A successful build:

```
   text    data     bss     dec     hex filename
  11000       8   21800   32808    8028 build/redlight.elf
```

## Watch the live UART output

```bash
picocom -b 115200 /dev/ttyACM0
```

Exit picocom: `Ctrl+A` then `Ctrl+X`.

What you'll see:

```
=== Smart Red Light System Online ===
Wave hand at sensor during RED phase to trigger violation.

--- GREEN ---
--- YELLOW ---
--- RED (detection active) ---
>>> VIOLATION #1 at tick=12345 (light=RED)
    [CAMERA] capture_request tick=12345  -> img_12345.jpg
--- GREEN ---
```

If picocom can't open `/dev/ttyACM0`:

- Confirm `groups | grep dialout` shows you in the group (re-login if not)
- Try `ls /dev/ttyACM*` — if no device, replug the Tiva's debug USB

## Demo behavior

| Phase | Action | Result |
|---|---|---|
| GREEN | hand wave at sensor | ignored (legal pass) |
| YELLOW | hand wave at sensor | ignored (legal pass) |
| RED | quick swipe (<150 ms) | ignored (pedestrian filter) |
| RED | held still 3+ seconds | ignored (stationary filter) |
| RED | normal pass (200-1500 ms) | **VIOLATION**: red flicker, camera log, violation log |

## Project structure

```
.
├── Makefile                  build glue, paths, rules
├── tm4c123gh6pm.ld           linker script (256 KB flash + 32 KB SRAM)
├── startup_gcc.c             vector table, FreeRTOS-patched
├── FreeRTOSConfig.h          80 MHz, 1 ms tick, heap_4 (24 KB)
├── inc/
│   ├── board.h               pin assignments — ONE source of truth
│   ├── config.h              tunables: timings, thresholds, priorities
│   └── events.h              inter-task data types (sensor_event_t, violation_record_t)
└── src/
    ├── main.c                hardware init, queue creation, task startup
    ├── light_task.c          GREEN→YELLOW→RED cycle, owns g_current_light
    ├── sensor_task.c         HC-SR04 reader + pedestrian dwell filter
    ├── violation_task.c      pure event dispatcher (sensor → 3 queues)
    ├── alert_task.c          visual/audible alert (flash + buzzer placeholder)
    ├── camera_task.c         ESP32-CAM capture command (UART placeholder)
    └── log_task.c            timestamped UART output
```

## Tuning

Edit `inc/config.h`:

| Constant | Default | Effect |
|---|---|---|
| `LIGHT_GREEN_MS` | 6000 | green phase duration |
| `LIGHT_YELLOW_MS` | 2000 | yellow phase duration |
| `LIGHT_RED_MS` | 5000 | red phase duration |
| `VEHICLE_DISTANCE_CM_MAX` | 30 | "object present" threshold |
| `VEHICLE_DWELL_MS_MIN` | 150 | shortest valid pass (pedestrian filter) |
| `VEHICLE_DWELL_MS_MAX` | 2000 | longest valid pass (stationary filter) |
| `SENSOR_POLL_HZ` | 50 | sensor polling rate |
| `ALERT_BUZZER_MS` | 500 | total alert duration |
| `ALERT_FLASH_MS` | 20 | flash burst (camera sync, when wired) |

## Future work — the modularization payoff

Each of these is **a single-file change**. The contract between modules
(queues + `events.h` types) is stable, so downstream code never knows.

### Swap HC-SR04 for FSR pressure sensors

1. Wire two FSRs to ADC channels (PE2/PE3 are free)
2. In `src/sensor_task.c`, replace `HCSR04_ReadCm()` with `FSR_Read()`
3. Replace dwell-time filter with axle-to-axle gap detection (both FSRs trigger in sequence within 50–500 ms)
4. Update thresholds in `inc/config.h`: `FSR_ADC_THRESHOLD`, `FSR_AB_GAP_MS_MIN`, `FSR_AB_GAP_MS_MAX`

`violation_task.c`, `alert_task.c`, `camera_task.c`, `log_task.c`,
`light_task.c`: **zero changes**.

### Wire the flash LED

1. Solder/wire 1W LED + transistor to PE2 with appropriate current limit
2. Uncomment `Alert_Flash()` in `src/alert_task.c`
3. Done

### Wire the ESP32-CAM

1. Configure UART2 in `main.c::HardwareInit()` (PD6/PD7, 115200)
2. Replace text placeholder in `src/camera_task.c` with the binary protocol
3. Tune the camera firmware (separate ESP32 project) to listen on UART
4. Done

### Add an LCD display

1. Create `src/display_task.c`
2. Subscribe to `xLogQueue` (ViolationTask already fans out to it)
3. Format violation count + last timestamp on every received record
4. Add to `main.c` task creation list

The architecture is fan-out, so adding consumers is additive. ViolationTask
doesn't need editing.

### Add HM-10 Bluetooth wireless logging

1. Configure UART2 in `HardwareInit()` (or UART1 if camera takes UART2)
2. Modify `src/log_task.c` to also write each line to that UART
3. Pair HM-10 to a phone, watch logs over Bluetooth

## Troubleshooting

**Build error: `unknown type name 'bool'`**
TivaWare expects `<stdbool.h>` before driverlib. Make sure your `.c`
files include `<stdbool.h>` and `<stdint.h>` first.

**Linker error: section .ARM.exidx LMA overlaps section .data**
Pass `-fno-unwind-tables -fno-exceptions -fno-asynchronous-unwind-tables`
in CFLAGS, ensure the linker script has `*(.ARM.exidx*)` inside `.text`.

**Boot loops (banner prints repeatedly, LED frozen)**
Stack overflow somewhere. Bump the offending task's stack. Recent culprit:
SensorTask if you change the HC-SR04 reader.

**LED stuck on green, scheduler runs but LightTask never advances**
A higher-priority task is starving LightTask. SensorTask must yield via
`vTaskDelay` (not `vTaskDelayUntil` if reads can exceed the period).

**HC-SR04 always reads 0 cm or OUT_OF_RANGE**
- Confirm VCC on `+3.3V` (silkscreen)
- Confirm ECHO firmly seated in `PB1`, not a neighbor
- Double-check TRIG and ECHO aren't swapped

**`make flash` fails with "Permission denied"**
Not in the `dialout` group, or udev rules didn't apply. Re-run the udev
step above and log out/in.

**picocom shows scrambled output**
Multiple terminals attached, or another `screen`/`picocom` instance is
still running. Run `sudo pkill picocom; sudo pkill screen` first, then
reopen.

## Team

| Name | GitHub | Responsibilities |
|---|---|---|
| Yousef Sayed | @yousefsebaie | Hardware, FSR sensor interfacing, ADC calibration, violation logic |
| Marwan Abudaif | @MarwanMoh11 | ESP32-CAM, flash circuit, light cycle, integration, build system |
| Hadi Hesham | @— | UART logging, display task, documentation, demo presentation |

Course: Embedded Systems, Spring 2026
