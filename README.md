# C Systems Programming & Bare-Metal AVR Firmware

Implementations in C covering bare-metal ATmega microcontroller firmware (for the FAU SPiCboard) and POSIX/Linux process and filesystem utilities. Only my own `.c` source files are included; university-provided hardware abstraction headers (`led.h`, `7seg.h`, `adc.h`, `timer.h`, `button.h`) are omitted.

```text
├── avr-bare-metal/
│   ├── led.c
│   ├── test-led.c
│   ├── snake.c
│   ├── game.c
│   └── solar.c
└── posix-linux/
    ├── mish.c
    └── printdir.c
```

## `avr-bare-metal/`

* **`led.c` & `test-led.c`**: Register-level GPIO driver configuring `DDRx` and `PORTx` across Ports B, C, and D with lazy initialization, bounds checking, bitmask output (`sb_led_setMask`), and bar-graph level scaling (`sb_led_showLevel`), paired with an on-board test routine using the 7-segment display.
* **`snake.c`**: Polls a photoresistor and potentiometer via ADC to scale animation speed and snake length (1–5 LEDs) dynamically, with edge-detected button presses toggling display inversion.
* **`game.c`**: LED reaction game using falling-edge external interrupts (`INT0`) and hardware timer delays to lock a moving cursor LED across increasing speed levels.
* **`solar.c`**: 6-state light-meter finite state machine (`OFF`, `BOOT`, `ON_7SEG`, `TRANS_7SEG`, `ON_LED`, `TRANS_LED`) driven by `INT0`/`INT1` button interrupts and `TIMER0` overflow interrupts (~61 Hz). Uses `avr/sleep` with `cli()`/`sei()` guards to avoid lost-wakeup race conditions between 1-second ADC photoresistor readings.

### Demo (`game.c`)
> https://github.com/user-attachments/assets/8930eb89-8bbc-42f0-a2b8-68d2d14903db

## `posix-linux/`

* **`mish.c`**: Minimal POSIX shell supporting foreground and background (`&`) execution via `fork()` and `execvp()`. Reaps child processes asynchronously with a `SIGCHLD` handler and `waitpid(..., WNOHANG)`, using `sigprocmask()` and `sigsuspend()` to wait on foreground jobs without signal race conditions.
* **`printdir.c`**: Directory listing tool using `opendir()`, `readdir()`, and `lstat()` to filter regular files (`S_ISREG`) and sum their sizes with path-length bounds checking.

### Build
```bash
gcc -std=c11 -D_XOPEN_SOURCE=700 -O3 -Wall -Werror -pedantic mish.c -o mish
gcc -std=c11 -D_XOPEN_SOURCE=700 -O3 -Wall -Werror -pedantic printdir.c -o printdir
```
