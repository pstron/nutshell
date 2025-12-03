# Nutshell

[![GitHub License](https://img.shields.io/github/license/pstron/nutshell)](LICENSE)

Firmware for a dual-inductor, single-motor electromagnetic line-following car (Class B).

## Overview

**Nutshell** is an open-source control program for an electromagnetic tracking vehicle.

This project grew out of a university on-campus competition, and you can read about the full story in my blog post:

[Building Nutshell: An Open-Source Line-Following Car - and Two Months of Teamwork and Memories](https://blog.feynmach.com/posts/building-nutshell)

The post also provides **detailed technical explanations of the code, module design, and development environment setup**, making it a helpful companion if you want to understand or modify the project in depth.

The project uses **STM32CubeMX**, **C++**, and a lightweight toolchain based on **gcc-arm-none-eabi**, **CMake**, **Ninja**, and **OpenOCD**.

## Features

* Dual inductor sensing
* Single-motor drive
* Electromagnetic line tracking
* Segmented PID control
* Real-time tuning with Vofa+
* STM32F103 + HAL (CubeMX generated)
* Fun add-on: Mario melody via passive buzzer

## Toolchain

* gcc-arm-none-eabi
* CMake + Ninja
* OpenOCD
* STM32CubeMX

## Build & Flash

```bash
./build.sh -S   # For more information, run ./build.sh -h or read the script
```

## Notes

* Designed for basic electromagnetic tracking research and education.
* Uses simple control logic; tuning should be adapted to specific hardware and track conditions.

## Limitations

Due to limited study of control theory and insufficient tuning time, the performance on the actual track is **average**.
