# EENG1030 Project 2 — Digital Stopwatch
**Module:** EENG1030 Embedded Systems  
**Platform:** STM32L432KC (ARM Cortex-M4 @ 80 MHz)  
**Framework:** STM32Cube HAL  
**IDE:** PlatformIO + Visual Studio Code  
**Student:** Ashish  
**Institution:** TU Dublin  

---

## Project Overview
A real-time digital stopwatch with millisecond precision built on the 
STM32L432KC Nucleo-32 microcontroller. Elapsed time is displayed on a 
0.96 inch ST7735S colour TFT LCD. Two push buttons control Start/Stop 
and Reset. Two LEDs indicate running state.

---

## Hardware
| Component | Details |
|-----------|---------|
| Microcontroller | STM32L432KC Nucleo-32 |
| Display | ST7735S 0.96" TFT LCD (SPI1, 80×160) |
| Timer | TIM6 — 1ms hardware interrupt |
| Button 1 | PB3 — Start/Stop (internal pull-up) |
| Button 2 | PB4 — Reset (internal pull-up) |
| Green LED | PA9 — ON when running |
| Red LED | PB1 — ON when stopped/paused |
| Resistors | 220Ω × 2 (current limiting) |

---

## Pin Mapping
| Pin | Function |
|-----|----------|
| PA4 | LCD CS |
| PA5 | LCD SCK (SPI1) |
| PA7 | LCD MOSI (SPI1) |
| PA8 | LCD DC |
| PA9 | Green LED |
| PB0 | LCD RST |
| PB1 | Red LED |
| PB3 | Button 1 — Start/Stop |
| PB4 | Button 2 — Reset |

---

## Software Architecture
- **main.c** — peripheral init, UI rendering, button polling, ISR callbacks
- **stopwatch.c** — state machine (SW_STOPPED/SW_RUNNING), 1ms counter
- **st7735.c** — custom SPI driver with embedded 5×7 font
- **stopwatch.h** — public API for state machine
- **st7735.h** — public API for display driver, RGB565 colour defines
- 
## How to Build and Flash
1. Install [PlatformIO](https://platformio.org/) in VS Code
2. Clone this repository:https://github.com/YOUR_USERNAME/EENG1030-Stopwatch.git
3. Open the folder in VS Code
4. Connect the Nucleo board via USB
5. Click **Upload** in PlatformIO sidebar

---

## Demonstration Video
[YouTube Link — https://youtu.be/m38J7Zu_JuM?is=Y9hb4_QMO80dizkN]

---

## References
- STMicroelectronics. STM32L432KC Datasheet DS11451 Rev 4. www.st.com  
- STMicroelectronics. RM0394 Reference Manual Rev 4. www.st.com  
- STMicroelectronics. AN4013 Timer Overview. www.st.com  
- Sitronix. ST7735S Datasheet v1.4  
- PlatformIO Documentation. docs.platformio.org
---

## How to Build and Flash
1. Install [PlatformIO](https://platformio.org/) in VS Code
2. Clone this repository:
