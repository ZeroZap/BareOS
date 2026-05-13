# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BareOS is an IoT bare-metal (裸机) framework for MCUs, written in C. It targets embedded systems running without an OS, built around the combination of **protothreads + state machines + foreground/background architecture + custom clib**.

The project is currently in the design/planning phase. Source code has not yet been committed. The architecture design document (`架构设计.md`) is a placeholder for detailed specifications.

## Planned Module Structure

The framework is organized into these major subsystems:

- **MCU Standard Peripheral Drivers** — GPIO, Timer, UART, SPI, I2C, Flash, Misc
- **AT Command Communication Modules** — 4G, Ethernet, Bluetooth, Satellite, GNSS
- **Storage Module** — EEPROM, internal/external Flash, fixed-length log storage (supports concurrent error/warning/event/usage log instances)
- **System Module** — sleep handling, reset reason tracking, versioning, restart, mode definitions
- **State Machine Module**
- **tiny clib** — custom C library implementation including `snprintf`
- **tiny memory** — custom memory management

## Architecture

The core execution model is a **foreground/background system**:
- Background: main loop drives protothreads (cooperative, stackless coroutines)
- Foreground: ISRs post events to the background

State machines layer on top of protothreads to handle protocol logic and multi-step peripheral interactions.

## Build System

No build system has been set up yet. When adding one, the `.gitignore` is already configured for C/C++ embedded toolchain artifacts (`.elf`, `.o`, `.obj`, `.map`, linker outputs, debug symbols).

Expected toolchain: ARM-GCC or similar cross-compiler, with CMake or Make as the build system.
