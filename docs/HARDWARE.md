# Hardware Notes

This project was prepared for an ESP32 connected to a u-blox 7M GPS module and a 128x64 SH1106 OLED display.

## GPS Module

- Recommended module family: `u-blox 7M`
- Serial interface: UART
- Expected baud rate in firmware: `9600`
- Parser input format: standard NMEA sentences

Many low-cost breakout boards sold as `NEO-7M` or `u-blox 7M` are compatible with this sketch if they expose standard UART TX/RX pins.

## ESP32 Wiring

Current firmware pin mapping:

| Function | ESP32 Pin |
| --- | --- |
| GPS RX | `GPIO16` |
| GPS TX | `GPIO17` |
| OLED SDA | board-dependent |
| OLED SCL | board-dependent |

Typical OLED defaults on many ESP32 boards are:

- SDA: `GPIO21`
- SCL: `GPIO22`

## Power Notes

- Confirm the GPS module supply voltage supported by your specific board.
- Confirm the OLED supply voltage supported by your specific display module.
- Make sure ESP32, GPS, and OLED share a common ground.

## Practical Fix Notes

- First GPS fix can take several minutes, especially indoors.
- Initial tests are easier near a window or outdoors.
- If you do not receive valid coordinates, verify TX/RX crossing and baud rate first.
