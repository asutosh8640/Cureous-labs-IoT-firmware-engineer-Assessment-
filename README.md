# IoT Firmware Engineer Technical Assessment - Cureous Labs

This repository contains the firmware implementation and simulation model for the Cureous Labs IoT Technical Assessment.

## Live Simulation
- Wokwi Simulation Link: [Yahan Apna Wokwi Share Link Paste Karein]

## Architecture Overview

### Task 1: ESP32 Wi-Fi & Dynamic Telemetry
- FreeRTOS periodic task publishing MQTT heartbeat JSON payloads every 5 seconds.
- Non-blocking telemetry output including uptime_ms, status, and free_heap.
- Dynamic network reconnection logic.

### Task 2: High-Frequency ADC Sensor Sampling & Filter
- Continuous sampling at 10 Hz (100 ms interval).
- Implemented a 10-sample Moving Average Filter to smooth analog sensor input noise.
- Transmits raw vs filtered analog values over Serial output.

### Task 3: FreeRTOS Task Synchronization (Producer-Consumer)
- GPIO EXTI Interrupts on Switch 1 & Switch 2 acting as event producers.
- Thread-safe FreeRTOS Queue for pushing button events with system timestamps.
- Blocking UART consumer task to safely process and log incoming events.

## Repository Structure
- sketch.ino - Main firmware application code containing tasks and interrupt handlers.
- diagram.json - Wokwi circuit configuration and pin mapping.
- README.md - Documentation and submission summary.
