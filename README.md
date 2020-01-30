# Bit-Banged I2C Master for Microcontrollers

This project provides a *generic* code to allow an easy impelentation of bit-banged I2C master over GPIOs. It is designed to be independent  of the target hardware, which means it could fit in virtualy any microcontroller. This code solve all the pin-shaking decisions of an I2C master.

## Features

__Multi-instance__

It allows the user to instantiate as many independent channels as needed.

__Hardware agnostic__

The user must provide the callbacks to properly shake the desired pins, according to the choosen hardware.

__Interrupt driven (or not!)__

The edge transitions is executed by calling `bb_i2c_master_edge_processor()` what is intended to occur inside a periodic ISR (Timer interruption) for a more eficient performance. However, the user is free to use other approaches like periodic task or loop with delay loops (less efficient).

__Other caracteristics__

- 7-bit I2C address.
- Read and Write functions can run in blocking or non-blocking mode.
- No bus fault detection.
- Send STOP when  detect NACK from slave.
- The bus speed is dependent on the frequence the `bb_i2c_master_edge_processor()` is called (4 calls = 1bit).
