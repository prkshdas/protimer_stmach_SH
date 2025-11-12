# Protimer State Machine - SH

This repository implements a productive timer using a state machine on STM32 microcontroller, featuring an OLED display and push buttons for user interaction.

## Features

- Countdown timer with productivity tracking.
- OLED display output for timer and status messages.
- Four physical buttons for timer control: Increase, Decrease, Start/Pause, Abort.
- Debouncing and button state management for reliable user input.

## Hardware Connections

### OLED Display

The display is handled by functions using the `SSD1306` controller. It is interacted with via I2C, typically connected to:

- **SCL (Clock):** Connect to STM32 I2C SCL pin.
- **SDA (Data):** Connect to STM32 I2C SDA pin.
- **VCC/GND:** Power the OLED with 3.3V (or 5V) and ground.

The firmware uses the following display functions:
- `SSD1306_GotoXY(x, y)`: Set cursor position.
- `SSD1306_Puts(str, font, color)`: Print text.
- `SSD1306_UpdateScreen()`: Refresh display.
- `SSD1306_Clear()`: Clear the display.

These are used in utility functions like `display_time()`, `display_message()`, and `display_clear()` for presenting timer values and status info.

### Buttons

There are four distinct buttons connected to the MCU GPIOs with the following purposes (as referenced in `main.h`):

| Button         | Purpose        | Code (macro)   |
| -------------- | -------------- | -------------- |
| Increase Time  | Add 1 minute   | BTN_VAL_INC (3)|
| Decrease Time  | Subtract 1 min | BTN_VAL_DEC (5)|
| Start/Pause    | Start/Pause    | BTN_VAL_SP  (6)|
| Abort          | Cancel timer   | BTN_VAL_ABRT(1)|

The buttons are scanned and debounced in the main loop (see [`process_btn_val()`](https://github.com/prkshdas/protimer_stmach_SH/blob/main/Core/Src/main.cpp)). Button reads trigger corresponding user events which the state machine reacts to.

### Example Button Wiring

GPIO pins for each button should be specified in your hardware setup. Typically, each button is connected between a designated STM32 pin and ground, with an optional pull-up resistor (internal/external).

## Circuit Diagram

Refer to the assets folder for the complete hardware schematic:

![Circuit Diagram](assets/circuit_image.png)

## Usage

1. Power up the hardware and connect the OLED display as per the circuit diagram.
2. Operate the timer using the four buttons. The OLED will provide real-time feedback.
3. The timer supports increment/decrement, start/pause, abort, and displays productivity stats on completion.

## Repository Structure

- `Core/Src/`: Main sources for the state machine, button processing, display logic.
- `Core/Inc/`: Header files and macro definitions.
- `assets/`: Contains circuit diagram and other supporting documents.

For more details on the source code:
- [OLED display handling and timer state logic](https://github.com/prkshdas/protimer_stmach_SH/blob/main/Core/Src/protimer_state_mach.cpp)
- [Button handling in main loop](https://github.com/prkshdas/protimer_stmach_SH/blob/main/Core/Src/main.cpp)
