# RC Car — ESP32 + nRF24L01 (PA+LNA) — README

## Project Overview

This project documents the RC car I built using two ESP32 boards and two NRF modules (nRF24L01 PA+LNA type), two Arduino joystick modules, an SG90 servo, an L298N motor driver, jumpers and breadboards. The controller ESP32 (transmitter) reads the joystick and sends control packets over the nRF radio. The car ESP32 (receiver) drives the L298N motors and an SG90 steering servo.

---

## Features

* Wireless control via nRF24L01+ PA+LNA (extended range module)
* Forward / backward / left / right and steering servo control
* Uses ESP32 for both transmitter and receiver
* Simple packet protocol (bytes for throttle + steering)

---

## Bill of Materials (BOM)

* 2 × ESP32 development boards
* 2 × nRF24L01+ PA+LNA wireless modules (3.3V devices)
* 2 × Arduino joystick module (2-axis + push)
* 1 × SG90 micro servo (steering)
* 1 × L298N motor driver (2-channel H-bridge)
* Jumper wires
* 2 × small breadboards (or one larger)
* Power source for motors (battery pack) and 5V/3.3V regulator as needed

---

## Important Notes

* **Power the nRF24L01 modules with a stable 3.3V supply.** Many PA+LNA modules draw higher current when transmitting and can cause brownouts if fed from a weak 3.3V regulator. Consider using a separate 3.3V regulator or add decoupling capacitors.
* Do not power the nRF module directly from an unregulated 5V line. It needs 3.3V.
* The PA+LNA versions increase range but require good power and proper antenna placement.

---

## Pin Connections (Suggested)

> Notes: pin names used are generic. Adapt pins to your specific ESP32 board and chosen SPI.

### nRF24L01 ↔ ESP32 (both boards)

* **VCC** -> 3.3V (do NOT use 5V)
* **GND** -> GND
* **CE** -> any GPIO (e.g., 5)
* **CSN (CS)** -> any GPIO (e.g., 15)
* **SCK** -> SCK pin (e.g., 18)
* **MOSI** -> MOSI pin (e.g., 23)
* **MISO** -> MISO pin (e.g., 19)
* **IRQ** -> optional (not required for basic use)

> Example (ESP32 default HSPI pins):
>
> * SCK = 18, MISO = 19, MOSI = 23, CSN = 5 (or 15), CE = 17

### Transmitter (Controller) — ESP32 + Joystick

* Joystick VRx -> analog pin (e.g., 35)
* Joystick VRy -> analog pin (e.g., 34)
* Joystick SW  -> digital pin (e.g., 0)
* nRF24L01 SPI pins as above

### Receiver (Car) — ESP32 + L298N + SG90

* SG90 signal -> PWM-capable GPIO (e.g., 21)
* L298N IN1/IN2 -> motor A control GPIOs (e.g., 14, 27)
* L298N IN3/IN4 -> motor B control GPIOs (e.g., 26, 25)
* L298N ENA/ENB -> PWM enable pins or tied high (for PWM speed control use EN pins connected to PWM pins via transistor or direct if board supports)
* Motors -> motor power battery (separate from ESP32 3.3V). Connect grounds together (common GND).
* nRF24L01 SPI pins as above

---

## Wiring Tips

* Use a separate power supply for motors. The ESP32 and nRF modules should be powered by a **stable 3.3V/5V regulated** supply with common ground to the motor battery.
* Add a 10–100 µF decoupling capacitor near the nRF power pins for PA+LNA modules to handle TX current spikes.
* If the nRF module has an on-board AMS1117 3.3V regulator (some breakout boards do), be mindful that it might overheat if you drive PA at high power; better to supply a dedicated 3.3V regulator.

---

## Software / Firmware

* Use the Arduino core for ESP32 (install via Board Manager) or PlatformIO.
* Recommended nRF24 library: `RF24` (or `RF24Network` for more complex setups).

### Simple packet protocol (example)

* Transmit two bytes:

  * Byte 0: throttle (-127 to +127) mapped to 0..255
  * Byte 1: steering (-127 to +127) mapped to 0..255

### Transmitter responsibilities

* Read joystick axes, map to throttle and steering values.
* Send packet over nRF radio at ~10–50 Hz.

### Receiver responsibilities

* Receive packet, decode throttle and steering.
* Drive L298N motor direction pins and PWM speed accordingly.
* Move SG90 servo to steering angle derived from steering byte.

---

## Example Arduino-style pseudo-code

(Include this in your repo as `controller.ino` and `receiver.ino` — below is a high-level outline.)

### Controller (transmitter)

```cpp
// read analog joystick
int vx = analogRead(VRX_PIN);
int vy = analogRead(VRY_PIN);
int throttle = map(vy, 0, 4095, -127, 127);
int steer = map(vx, 0, 4095, -127, 127);

uint8_t packet[2];
packet[0] = throttle + 128; // map to unsigned
packet[1] = steer + 128;

radio.write(&packet, sizeof(packet));
```

### Receiver (car)

```cpp
uint8_t packet[2];
if (radio.available()) {
  radio.read(&packet, sizeof(packet));
  int throttle = (int)packet[0] - 128;
  int steer = (int)packet[1] - 128;

  // Convert throttle to PWM magnitude and direction pins
  // Convert steer to servo angle
}
```

---

## Troubleshooting

* **nRF modules not communicating**: check power (3.3V), wiring (CE/CSN swapped), and SPI pins. Use simple RX/TX test sketches first.
* **Range is poor**: ensure PA+LNA antenna is connected and nRF has stable power. Reduce data rate (250 kbps gives best range).
* **ESP32 resets when radio transmits**: likely brownout — add capacitor or use stronger 3.3V regulator.
* **Motors cause noise**: add decoupling capacitors and ferrite beads; keep motor wires short.

---

## RC Car Demo Video

Here’s a demo of the RC car in action:

[![Watch the video](https://img.shields.io/badge/Watch%20Video-Click%20Here-red?style=for-the-badge)](https://github.com/1-abesh-1/spike-the-dog-rc-car/blob/main/WhatsApp%20Video%202025-09-27%20at%206.53.03%20PM.mp4)


---

## Note

I burned some components during development:

One ESP32 was damaged due to a short circuit caused by a coverless 18650 LiPo battery.
The motor driver (L298N) requires a stable 12V supply to work reliably.
Battery protection boards were added later for both the transmitter and receiver to avoid similar issues in the future.
The other two components were burned in different projects. I plan to create a repository to share the causes of burned components and the precautions to avoid them.
![burned](https://github.com/1-abesh-1/spike-the-dog-rc-car/blob/main/WhatsApp%20Image%202025-09-27%20at%206.51.00%20PM.jpeg?raw=true)

---


---

## Contribution

Feel free to open issues or PRs if you want to improve the firmware, wiring guides, or add features (e.g., failsafe, telemetry, battery voltage reporting).

---

