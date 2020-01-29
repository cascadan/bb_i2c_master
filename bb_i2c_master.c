/******************************************************************************

MIT License

Copyright (c) 2020 André Muller Cascadan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

******************************************************************************/



#include "bb_i2c_master.h"


void bb_i2c_master_init( bb_i2c_master_t* instance,
                                    void (*cb_sda_drive_low)(void),
                                    void (*cb_sda_high_z)   (void),
                                    void (*cb_scl_drive_low)(void),
                                    void (*cb_scl_high_z)   (void),
                                     int (*cb_sda_read)     (void) )
{

	// Initialize callbacks
	instance->sda_drive_low =  cb_sda_drive_low;
	instance->sda_high_z    =  cb_sda_high_z   ;
	instance->scl_drive_low =  cb_scl_drive_low;
	instance->scl_high_z    =  cb_scl_high_z   ;
	instance->sda_read      =  cb_sda_read     ;
	
	// Initialize IDLE state
	instance->start_flag = 0;
	instance->busy_flag  = 0;
	instance->state      = IDLE;
	
	// Initialize Bus in IDLE levels
	instance->scl_high_z();
	instance->sda_high_z();
}


void bb_i2c_master_write( bb_i2c_master_t* instance, uint8_t addr, uint8_t data[], int len, bb_i2c_master_mode mode )
{
	
	// Slave Address + R/W bit (0 = write)
	instance->addr_rw = (addr<<1) & 0xFE;
	
	// Data to be transmited
	instance->data = data;
	instance->write_len = len;
	instance->read_len = 0;

	// Start transaction
	instance->start_flag = 1;
	instance->busy_flag = 1;
	
	// Wait until finish the transaction
	if( (mode & BB_I2C_MASTER_MODE_NON_BLOCKING) == 0 )
		while( instance->busy_flag != 0 )
		{
			__asm__("nop");
		}
}


void bb_i2c_master_read( bb_i2c_master_t* instance, uint8_t addr, uint8_t data[], int len, bb_i2c_master_mode mode )
{
	
	// Slave Address + R/W bit (1 = read)
	instance->addr_rw = ( (addr<<1) & 0xFE ) | 0x01;
	
	// Data to be read
	instance->data = data;
	instance->write_len = 0;
	instance->read_len = len;

	// Start transaction
	instance->start_flag = 1;
	instance->busy_flag = 1;
	
	// Wait until finish the transaction
	if( (mode & BB_I2C_MASTER_MODE_NON_BLOCKING) == 0 )
		while( instance->busy_flag != 0 )
		{
			__asm__("nop");
		}
}


int bb_i2c_master_is_busy( bb_i2c_master_t* instance )
{
	return instance->busy_flag;
}


void bb_i2c_master_edge_processor( bb_i2c_master_t* instance )
{
	
	int byte;
	
	switch (instance->state)
	{
		case IDLE:
			if (instance->start_flag != 0 )
			{
				instance->start_flag = 0;
				instance->byte_cntr = 0;
				instance->phase = 3; // Phase adjust to start as 0 in next call.
				instance->state = START;
			}
			break;
		
		case START:
			switch (instance->phase)
			{
				case 0:
					break;
				case 1:
					break;
				case 2:
					instance->sda_drive_low();
					break;
				case 3:
					instance->scl_drive_low();
					instance->bit_cntr = 8;
					instance->current_byte = instance->addr_rw;
					instance->state = WR_WORD;
					break;
			}
			break;
		
		case WR_WORD:
			switch (instance->phase)
			{
				case 0:
					if( (instance->current_byte & 0x80)!=0 ) // insert bit on line 
						instance->sda_high_z();
					else
						instance->sda_drive_low();
					instance->current_byte = instance->current_byte << 1;
					instance->bit_cntr --;
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					break;
				case 3:
					instance->scl_drive_low();
					if (instance->bit_cntr == 0)
						instance->state = RD_ACK;
					break;
			}
			break;
			
		case RD_ACK:
			switch (instance->phase)
			{
				case 0:
					instance->sda_high_z(); // Release the bus
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					break;
				case 3:
					if( instance->sda_read() != 0) //If NACK
					{
						instance->state = STOP;
					}
					else
					{
						if( (instance->byte_cntr) < (instance->write_len) )
						{
							instance->current_byte = instance->data[instance->byte_cntr];
							instance->byte_cntr ++;
							instance->bit_cntr = 8;
							instance->state = WR_WORD;
						}
						else
						{
							if( (instance->byte_cntr) < (instance->read_len) )
							{
								instance->current_byte = 0;
								instance->bit_cntr = 8;
								instance->state = RD_WORD;
							}
							else 
								instance->state = STOP;
						}
					}
					instance->scl_drive_low();
					break;
			}
			break;
		
		case RD_WORD:
			switch (instance->phase)
			{
				case 0:
					instance->sda_high_z(); // Release the bus
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					break;
				case 3:
					byte = instance->current_byte << 1;
					if( instance->sda_read() != 0)
						instance->current_byte = byte | 0x01;
					else
						instance->current_byte = byte;
					instance->scl_drive_low();
					instance->bit_cntr --;
					if( instance->bit_cntr == 0 )
					{
						instance->data[instance->byte_cntr] = instance->current_byte;
						instance->byte_cntr ++;
						if( (instance->byte_cntr) < (instance->read_len) )
							instance->state = WR_ACK;
						else
							instance->state = WR_NACK;
					}
					break;
			}
			break;
		
		case WR_ACK:
			switch (instance->phase)
			{
				case 0:
					instance->sda_drive_low();
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					break;
				case 3:
					instance->scl_drive_low();
					instance->bit_cntr = 8;
					instance->state = RD_WORD;
					break;
			}
			break;
			
		case WR_NACK:
			switch (instance->phase)
			{
				case 0:
					instance->sda_high_z();
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					break;
				case 3:
					instance->scl_drive_low();
					instance->state = STOP;
					break;
			}
			break;
			
		case STOP:
			switch (instance->phase)
			{
				case 0:
					instance->sda_drive_low();
					break;
				case 1:
					instance->scl_high_z();
					break;
				case 2:
					instance->sda_high_z();
					break;
				case 3:
					instance->busy_flag = 0;
					instance->state = IDLE;
					break;
			}
			break;
			
	}
	
	// Phase increment: 0 -> 1 -> 2 -> 3 -> 0 -> 1 ... etc.
	instance->phase ++;
	instance->phase &= 0x03;
	
}
	
	
	
	
	
	
	
	
