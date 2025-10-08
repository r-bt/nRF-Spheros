# nRF-Spheros

This is firmware for the nRF5380 DK dev board to control up to 15 Sphero BOLTs

## Installation

1. Clone the repo
2. Follow the installation guide for [nRF Connect for VSCode](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-VS-Code)

## Setup

In `prj.conf` you need to specify how many Spheros you're connecting to on two lines:
- `CONFIG_BT_SCAN_NAME_CNT`
- `CONFIG_BT_MAX_CONN`

## Building and flashing

Using the **nRF Connect for VSCode plugin** you should be able to build the firmware and flash it to the dev board

Additioanlly, the side panel for the nRF Connect plugin should show the port name of the board that will be used for sending the UART commands.