/***************************************************************************
* File Name: PlayingWithFusion_MAX31856.cpp
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.8.3
*
* Designed for use with with Playing With Fusion MAX31856 thermocouple
* breakout boards: SEN-30005, SEN-30006 (any TC type)
*
* Copyright © 2015 Playing With Fusion, Inc.
* SOFTWARE LICENSE AGREEMENT: This code is released under the MIT License.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
* **************************************************************************
* REVISION HISTORY:
* Author			Date		Comments
* J. Steinlage		2015Aug10   First rev
* J. Steinlage      2017May08   Read TC ch, even if fault. Fix reg update fcn
* J. Steinlage		2018Jul20	Removed DR and FLT pins since nobody uses them
*
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source
* development by buying products from Playing With Fusion!
* **************************************************************************
* ADDITIONAL NOTES:
* This file contains functions to initialize and run an Arduino Uno R3 in
* order to communicate with a MAX31856 single channel thermocouple breakout
* board. Funcionality is as described below:
*	- Initialize TC channel
*	- Read MAX31856 registers from Playing With Fusion SEN-30005 (any type)
*	- Properly unpack data into internal temp, TC temp and status variables
***************************************************************************/
#include "PlayingWithFusion_MAX31856.h"

PWF_MAX31856::PWF_MAX31856(uint8_t CSx)
{
	// Function to initialize thermocouple channel, load private variables
	_cs = CSx;

	pinMode(_cs, OUTPUT);

	// immediately pull CS pin high to avoid conflicts on SPI bus
	digitalWrite(_cs, HIGH);
}

uint8_t PWF_MAX31856::_sing_reg_read(uint8_t RegAdd)
{
	digitalWrite(_cs, LOW); // set pin low to start talking to IC
	// next pack address byte
	// bits 7:4 are 0 for read, register is in bits 3:0... format 0Xh
	SPI.transfer((RegAdd & 0x0F)); // write address
	// then read register data
	uint8_t RegData = SPI.transfer(0x00); // read register data from IC
	digitalWrite(_cs, HIGH);			  // set pin high to end SPI session

	return RegData;
}

void PWF_MAX31856::_sing_reg_write(uint8_t RegAdd, uint8_t BitMask, uint8_t RegData)
{
	// start by reading original register data (we're only modifying what we need to)
	uint8_t OrigRegData = _sing_reg_read(RegAdd);

	// calculate new register data... 'delete' old targeted data, replace with new data
	// note: 'BitMask' must be bits targeted for replacement
	// add'l note: this function does NOT shift values into the proper place... they need to be there already
	uint8_t NewRegData = ((OrigRegData & ~BitMask) | (RegData & BitMask));

	// now configure and write the updated register value
	digitalWrite(_cs, LOW); // set pin low to start talking to IC
	// next pack address byte
	// bits 7:4 are 1000b for read, register is in bits 3:0... format 8Xh
	SPI.transfer((RegAdd & 0x0F) | 0x80); // simple write, nothing to read back
	SPI.transfer(NewRegData);			  // write register data to IC
	digitalWrite(_cs, HIGH);			  // set pin high to end SPI session
}

void PWF_MAX31856::MAX31856_config(uint8_t TC_TYPE, uint8_t FILT_FREQ, uint8_t AVG_MODE, uint8_t MEAS_MODE)
{
	// TC_TYPE: B_TYPE, E_TYPE, J_TYPE, K_TYPE, N_TYPE, R_TYPE, S_TYPE, T_TYPE
	// FILT_FREQ: CUTOFF_60HZ, CUTOFF_50HZ
	// AVG_MODE: AVG_SEL_1SAMP, AVG_SEL_2SAMP, AVG_SEL_4SAMP, AVG_SEL_8SAMP, AVG_SEL_16SAMP
	// MEAS_MODE: CMODE_OFF, CMODE_AUTO

	uint8_t regdat = 0; // set up paramater to compile register configs

	// set CR0 (REG_CR0)
	if (CMODE_AUTO == MEAS_MODE) // auto conversion mode selected (~100ms interval sampling)
	{
		regdat = (CMODE_AUTO | ONESHOT_OFF | OCFAULT_10MS | CJ_ENABLED | FAULT_AUTO | FAULT_CLR_DEF | FILT_FREQ);
	}
	else // else it's single-shot mode
	{
		regdat = (CMODE_OFF | ONESHOT_OFF | OCFAULT_10MS | CJ_ENABLED | FAULT_AUTO | FAULT_CLR_DEF | FILT_FREQ);
	}
	_sing_reg_write(REG_CR0, 0xFF, regdat); // write data to register
											/*	CRO, 00h/80h:[7] cmode (0=off (default), 1=auto conv mode)
		[6] 1shot (0=off, default)
		[5:4] OCFAULT (table 4 in datasheet)
		[3] CJ disable (0=cold junction enabled by default, 1=CJ disabled, used to write CJ temp)
		[2] FAULT mode (0=sets, clears automatically, 1=manually cleared, sets automatically)
		[1] FAULTCLR   (0 - default, 1=see datasheet)
		[0] 50/60Hz (0=60hz (default), 1=50Hz filtering) + harmonics */

	// set CR1 (REG_CR1)
	regdat = (AVG_MODE | TC_TYPE);
	_sing_reg_write(REG_CR1, 0xFF, regdat);
	/*	CR1, 01h/81h:[7] reserved
		[6:4] AVGSEL (0=1samp(default),1=2samp,2=4samp,3=8samp,0b1xx=16samp])
		[3:0] TC type (0=B, 1=E, 2=J, 3=K(default), 4=N, 5=R, 6=S, 7=T, others, see datasheet)*/

	// set MASK (REG_MASK) - PWF default masks all but OV/UV and OPEN from lighting LED
	regdat = (CJ_HIGH_MASK | CJ_LOW_MASK | TC_HIGH_MASK | TC_LOW_MASK);
	_sing_reg_write(REG_MASK, 0x3F, regdat);
	/*	MASK, 02h/82h: This register masks faults from causing the FAULT output from asserting,
				   but fault bits will still be set in the FSR (0x0F)
		           All faults are masked by default... must turn them on if desired
		[7:6] reserved
		[5] CJ high fault mask
		[4] CJ low fault mask
		[3] TC high fault mask
		[2] TC low fault mask
		[1] OV/UV fault mask
		[0] Open fault mask
		PWF example: 0x03 (OV/UV + open) */

	// LEAVE CJHFT/CJLFT AT DEFAULT VALUES FOR PWF EXAMPLE
	// note: these values would potentially be used to indicate material or component
	//       limits have been exceeded for your specific measurement configuration
	/*	CJHFT, 03h/83h: cold-jcn high fault threshold, default 0x7F (bit 7 is sign)
	CJLFT, 04h/84h: cold-jcn low fault threshold, default 0x00) */

	// LEAVE LTXFTX AT DEFAULT VALUES FOR PWF EXAMPLE
	// note: these values would potentially be used to indicate material limits
	//       have been exceeded for your specific thermocouple
	/*	LTHFTH, 05h/85h: Linearize temperature high fault thresh MSB (bit 7 is sign)
	LTHFTL, 06h/86h: Linearize temperature high fault thresh LSB
	LTLFTH, 07h/87h: Linearize temperature low fault thresh MSB (bit 7 is sign)
	LTLFTL, 08h/88h: Linearize temperature low fault thresh LSB */
}

void PWF_MAX31856::MAX31856_update(struct var_max31856 *tc_ptr)
{
	// Start by reading SR for any faults, exit if faults present, though some
	// faults could potentially be dealt with
	uint8_t fault_status = _sing_reg_read(REG_SR);
	tc_ptr->status = fault_status;

	// Read Cold Jcn temperature (2 registers)
	int16_t cj_temp = (_sing_reg_read(REG_CJTH) << 8); // MSB, left shift 8
	cj_temp |= _sing_reg_read(REG_CJTL);			   // LSB read
	// now save sign, shift right 2 to align to type (see datasheet, pg 24 for reg packing)
	if (cj_temp & 0x8000)
	{
		cj_temp = (0xE000 | ((cj_temp & 0x7FFF) >> 2));
	}
	else
	{
		cj_temp = (cj_temp & 0x7FFF) >> 2;
	}
	tc_ptr->ref_jcn_temp = cj_temp; // store result in struct

	// Read Linearized TC temperature (3 registers)
	int32_t tc_temp = (_sing_reg_read(REG_LTCBH) << 8); // HSB, left shift 8
	tc_temp |= _sing_reg_read(REG_LTCBM);				// MSB
	tc_temp <<= 8;										// left shift 8
	tc_temp |= _sing_reg_read(REG_LTCBL);				// LSB
	// now save sign, shift to align to type (see datasheet, pg 25 for reg packing)
	if (tc_temp & 0x800000)
	{
		tc_temp = 0xFFFC0000 | ((tc_temp & 0x7FFFFF) >> 5);
	}
	else
	{
		tc_temp = (tc_temp & 0x7FFFFF) >> 5;
	}
	tc_ptr->lin_tc_temp = tc_temp;
}

void PWF_MAX31856::clear_fault()
{
	uint8_t t = _sing_reg_read(REG_CR0);

	t |= REG_MASK; // enable the fault status clear bit

	// now configure and write the updated register value
	digitalWrite(_cs, LOW); // set pin low to start talking to IC
	// next pack address byte
	// bits 7:4 are 1000b for read, register is in bits 3:0... format 8Xh
	SPI.transfer((REG_CR0 & 0x0F) | 0x80); // simple write, nothing to read back
	SPI.transfer(t);					   // write register data to IC
	digitalWrite(_cs, HIGH);			   // set pin high to end SPI session
}

void PWF_MAX31856::MAX31856_1shot_start(void)
{

	_sing_reg_write(REG_CR0, ONESHOT_ON, ONESHOT_ON); // set the 1shot bit
													  // note, it takes approximately 143ms to perform the conversion in 60-Hz filter mode
													  // and 169ms in 50-Hz filter mode. Keep this in mind if not using DRDY output for interrupt.
													  // Call the MAX31856_update command after the conversion is complete to read the new value.
													  // Also, don't run simultaneous conversions on multiple MAX chips if they are electrically connected
													  // as the readings will likely be skewed.
}

void PWF_MAX31856::MAX31856_CJ_offset(int8_t offset_val) // offset is 2^-4 degC/bit
{
	/*	CJTO, 09h/89h: Cold Junction Temperature Offset (int8_t, default 0x00) */
	// This function could be used to add a temperature offset based on a known difference
	//      between the chip temp and the location of the TC metal to copper transition

	// might need to write special handling for the signedness of the offset_val...
	_sing_reg_write(REG_CJTO, 0xFF, (uint8_t)offset_val);
}
