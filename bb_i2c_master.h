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


#ifndef BB_I2C_MASTER 
#define BB_I2C_MASTER

#include <stdint.h>



/*
 * This option alows read and write funtions return imediately.
 * In this case bb_i2c_master_is_busy() must be polled to check when the
 * transaction is concluded.
 */
#define BB_I2C_MASTER_MODE_NON_BLOCKING 0x01


typedef unsigned int bb_i2c_master_mode;

/*
 * Type to hold the instance. This driver allows instantiate several I2C channel.
 */
typedef struct
{
	
	// SDA/SCL functions
	void (*sda_drive_low)(void);
	void (*sda_high_z)   (void);
	void (*scl_drive_low)(void);
	void (*scl_high_z)   (void);
	 int (*sda_read)     (void);
	
	// Flow control variables
	int addr_rw; //slave address + read/write bit
	uint8_t *data;
	int write_len;
	int read_len;
	int start_flag;
	int busy_flag;
	
	enum { IDLE = 0, START,  WR_WORD,
	       RD_WORD,  RD_ACK, WR_ACK,
	       WR_NACK,  STOP            } state;
	
	int phase;
	int current_byte;
	int byte_cntr;
	int bit_cntr;
	

}bb_i2c_master_t;


/*
 * Initialization function.
 * 
 * instance:         Pointer to the channel instance.
 * 
 * cb_sda_drive_low: Callback to drive LOW in the SDA line.
 *                   
 *                   This function must set the desired pin as OUTPUT and set to LOW.
 *                   It is used by the driver to send ZEROs or ACKs.
 * 
 * cb_sda_high_z:    Callback to set HIGH-Z/INPUT in the SDA line.
 *                   
 *                   This function must set the desired pin as INPUT. 
 *                   It acts as an open colector in HIGH state.
 *                   It is used by the driver to send ONEs, or NACKs or listen the line.
 * 
 * cb_scl_drive_low: Callback to drive LOW in the SCL line.
 * 
 *                   Works like cb_sda_drive_low()
 * 
 * cb_sda_high_z:    Callback to set HIGH-Z/INPUT in the SCL line.
 * 
 *                   Works like cb_sda_high_z()
 * 
 * NOTE: It is recommended to initialize the interface **before** start the periodic 
 *       calling of bb_i2c_master_edge_processor() (i.e. before enable the timer 
 *       interruption).
 */
void bb_i2c_master_init( bb_i2c_master_t* instance,
                                    void (*cb_sda_drive_low)(void),
                                    void (*cb_sda_high_z)   (void),
                                    void (*cb_scl_drive_low)(void),
                                    void (*cb_scl_high_z)   (void),
                                     int (*cb_sda_read)     (void) );


/*
 * Master Write function.
 * 
 * instance: Pointer to the channel instance.
 * 
 * addr:     7-bit I2C slave address.
 * 
 * data:     Array containing the data to be sent into slave.
 * 
 * len:      The number of data bytes to be written into slave.
 * 
 * mode:     BB_I2C_MASTER_MODE_NON_BLOCKING for non blocking. 0 otherwise.
 * 
 */
void bb_i2c_master_write( bb_i2c_master_t* instance, uint8_t addr, uint8_t data[], int len, bb_i2c_master_mode mode );


/*
 * Master Read function.
 * 
 * instance: Pointer to the channel instance.
 * 
 * addr:     7-bit I2C slave address.
 * 
 * data:     Array get the received data.
 * 
 * len:      The number of data bytes to be read from slave.
 * 
 * mode:     BB_I2C_MASTER_MODE_NON_BLOCKING for non blocking. 0 otherwise.
 * 
 */
void bb_i2c_master_read ( bb_i2c_master_t* instance, uint8_t addr, uint8_t data[], int len, bb_i2c_master_mode mode );


/*
 * Check if the a given channel instance is busy (transmitting or receiving).
 * This is useful when using BB_I2C_MASTER_MODE_NON_BLOCKING option.
 */
int bb_i2c_master_is_busy( bb_i2c_master_t* instance );


/*
 * Edge processor function.
 * 
 * This is the function which makes the transaction happen. It must be called periodically 
 * every quarter of bit-rate (eg: for 100KHz I2C, it must be called at 400KHz).
 * 
 * It was designed to be called inside a periodic timer interruption. However, it could
 * be called in other ways, like a periodic task, or a loop with a delay.
 * 
 * When the channel is not being used (idle), the user is allowed suspend the periodic 
 * calling to improve CPU performance.
 */
void bb_i2c_master_edge_processor( bb_i2c_master_t* instance );




#endif // BB_I2C_MASTER
