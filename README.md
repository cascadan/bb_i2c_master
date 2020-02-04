# Bit-Banged I2C Master for Microcontrollers

This project provides a *generic* code to allow an easy implementation of bit-banged I2C master over GPIOs. It is designed to be independent of the target hardware, which means it could fit in virtually any microcontroller. This code solve all the pin-shaking decisions of an I2C master.

## Features

__Multi-instance__

It allows the user to instantiate as many independent channels as needed.

__Hardware agnostic__

The user must provide the callbacks to properly shake the desired pins, according to the chosen hardware.

__Interrupt driven (or not!)__

The edge transitions are executed by calling `bb_i2c_master_edge_processor()` what is intended to occur inside a periodic ISR (timer interruption) for a more efficient performance. However, the user is free to use other approaches like periodic task or loop with delay loops (less efficient).

__Other caracteristics__

- 7-bit I2C address.
- Read and Write functions can run in blocking or non-blocking mode.
- No bus fault detection.
- Send STOP when  detect NACK from slave.
- The bus speed is dependent on the frequency the `bb_i2c_master_edge_processor()` is called (4 calls = 1bit).

## How to use

### GPIO callbacks

Since this project is "hardware agnostic", it knows nothing about the GPIOs. The user must to implement the five callbacks to control the lines (SDA and SCL). The GPIOs must behave as  **open collector**. To act as an open collector in the OFF (High-Z) state, the GPIO chosen for SDA should be set as INPUT, so that it stays in high impedance and can still be read.

The example below shows how to implement the callbacks for SDA. "12" is an hypothetical GPIO pin chosen to be the SDA of the channel instance "ch0". The implementation of SCL follows the same idea.



```c
void ch0_sda_drive_low( void )
{
	gpio_set_pin_direction( 12, OUTPUT );
	gpio_set_pin_level( 12, LOW );
}

void ch0_sda_high_z( void )
{
	gpio_set_pin_direction( 12, INPUT );
}

int ch0_sda_read( void )
{
	if( gpio_get_pin_level(12) == HIGH )
		return 1;
	else
		return 0;
}
```


### Initialization with ISR

The following example shows how to initialize an I2C master using timer interruption to clock the bit-banging.

**NOTE:** It is recommended to enable the interruption only after calling ```bb_i2c_master_init()```, otherwise ```bb_i2c_master_edge_processor()``` may not behave well.

```c
#include "bb_i2c_master.h"

bb_i2c_master_t i2c_channel0;

// GPIO callbacks
void ch0_sda_drive_low( void );
void ch0_sda_high_z( void );
void ch0_scl_drive_low( void );
void ch0_scl_high_z( void );
int ch0_sda_read( void );

int main( void )
{
	// Initialize the timer (keep interruption disabled)
	timer_init();
	timer_set_period(I2C_CLK_PERIOD/4);

	// Initialize the I2C driver
	bb_i2c_master_init( &i2c_channel0,
	                    ch0_sda_drive_low,
	                    ch0_sda_high_z,
	                    ch0_scl_drive_low,
	                    ch0_scl_high_z,
	                    ch0_sda_read      );
	                    
	// Enable timer interruption. The "edge processor" starts running in IDLE state...
	timer_interrupt_enable();
	
	/*********
	     etc.
	   *********/
}


// Periodic timer interruption routine.
// If called 4000 times per second, gives a 1kHz I2C bus
void timer_isr(void)
{
	bb_i2c_master_edge_processor( &i2c_channel0 );
}
```


### Reading and Writing

This is simple. In the normal mode (blocking) the function blocks until the transaction finishes. This is useful for multi-task systems.

```c
uint8_t my_buffer[BUFF_SIZE];

// Write 3 bytes from my_buffer into slave 0x51
bb_i2c_master_write( &i2c_channel0, 0x51, my_buffer, 3, 0 );

// Read 2 bytes from slave 0x51 to my_buffer
bb_i2c_master_read ( &i2c_channel0, 0x51, my_buffer, 2, 0 );
```

The non-blocking mode may be a good option in single-task systems. In this case, user need to check if the transaction finished by polling ```bb_i2c_master_is_busy()```


### Working without an ISR

Some bit-banged drivers just uses software delays to clock the bit-banging. In this case no timer or ISR is needed. This example shows how to do it with Read (the same can be done for Write):

```c
#include "bb_i2c_master.h"

bb_i2c_master_t i2c_channel0;

// GPIO callbacks
void ch0_sda_drive_low( void );
void ch0_sda_high_z( void );
void ch0_scl_drive_low( void );
void ch0_scl_high_z( void );
int ch0_sda_read( void );

int main( void )
{
	int i;
	uint8_t my_buffer[BUFF_SIZE];
	
	
	// Initialize the I2C driver
	bb_i2c_master_init( &i2c_channel0,
	                    ch0_sda_drive_low,
	                    ch0_sda_high_z,
	                    ch0_scl_drive_low,
	                    ch0_scl_high_z,
	                    ch0_sda_read      );
	                    
	
	// Read 2 bytes from slave 0x51 to my_buffer (Must be Non-blocking)
	bb_i2c_master_read ( &i2c_channel0, 0x51, my_buffer, 2, BB_I2C_MASTER_MODE_NON_BLOCKING );
	
	// Start shaking the pins until the reading finishes.
	while( bb_i2c_master_is_busy(&i2c_channel0) )
	{
		// Process one edge
		bb_i2c_master_edge_processor( &i2c_channel0 );
		
		// Delay of one quarter of the desired I2C clock period.
		for( i=0; i<DELAY_PERIOD; i++ )
		{
			__asm__("nop");
		}
	}
	
	//  Reading finished!
}
```
