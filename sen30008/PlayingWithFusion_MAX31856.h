/***************************************************************************
* File Name: PlayingWithFusion_MAX31856.h
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
* J. Steinlage		2018Jul10	Removed DR and FLT pins since nobody uses them
*
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source
* development by buying products from Playing With Fusion!
*
* **************************************************************************/

#ifndef PWF_MAX31856_h
#define PWF_MAX31856_h

#include "Arduino.h"			// use "WProgram.h" for IDE <1.0
#include "stdlib.h"
#include "PlayingWithFusion_MAX31856_STRUCT.h"
#include "SPI.h"

// Registers
#define REG_CR0 	0x00	// Config Reg 0 - See Datasheet, pg 19
#define REG_CR1 	0x01	// Config Reg 1 - averaging and TC type
#define REG_MASK	0x02	// Fault mask register (for fault pin)
#define REG_CJHF	0x03	// Cold Jcn high fault threshold, 1 degC/bit
#define REG_CJLF	0x04	// Cold Jcn low fault threshold, 1 degC/bit
#define REG_LTHFTH	0x05	// TC temp high fault threshold, MSB, 0.0625 degC/bit
#define REG_LTHFTL	0x06	// TC temp high fault threshold, LSB
#define REG_LTLFTH	0x07	// TC temp low fault threshold, MSB, 0.0625 degC/bit
#define REG_LTLFTL	0x08	// TC temp low fault threshold, LSB
#define REG_CJTO	0x09	// Cold Jcn Temp Offset Reg, 0.0625 degC/bit
#define REG_CJTH	0x0A	// Cold Jcn Temp Reg, MSB, 0.015625 deg C/bit (2^-6)
#define REG_CJTL	0x0B	// Cold Jcn Temp Reg, LSB
#define REG_LTCBH	0x0C	// Linearized TC Temp, Byte 2, 0.0078125 decC/bit
#define REG_LTCBM	0x0D	// Linearized TC Temp, Byte 1
#define REG_LTCBL	0x0E	// Linearized TC Temp, Byte 0
#define REG_SR		0x0F	// Status Register

// CR0 Configs
#define CMODE_OFF		0x00
#define	CMODE_AUTO		0x80
#define ONESHOT_OFF		0x00
#define ONESHOT_ON		0x40
#define OCFAULT_OFF		0x00
#define OCFAULT_10MS	0x10
#define OCFAULT_32MS	0x20
#define OCFAULT_100MS	0x30
#define CJ_ENABLED		0x00
#define CJ_DISABLED		0x08
#define FAULT_AUTO		0x00
#define FAULT_MANUAL	0x04
#define FAULT_CLR_DEF	0x00
#define FAULT_CLR_ALT	0x02
#define CUTOFF_60HZ		0x00
#define CUTOFF_50HZ		0x01

// CR1 Configs
#define AVG_SEL_1SAMP	0x00
#define AVG_SEL_2SAMP	0x10
#define AVG_SEL_4SAMP	0x20
#define AVG_SEL_8SAMP	0x30
#define AVG_SEL_16SAMP	0x40
#define B_TYPE			0x00
#define E_TYPE			0x01
#define J_TYPE			0x02
#define K_TYPE			0x03
#define N_TYPE			0x04
#define R_TYPE			0x05
#define S_TYPE			0x06
#define T_TYPE			0x07

// MASK Configs
#define CJ_HIGH_MASK	0x20
#define CJ_LOW_MASK		0x10
#define TC_HIGH_MASK	0x08
#define TC_LOW_MASK		0x04
#define OV_UV_MASK		0x02
#define OPEN_FAULT_MASK	0x01

class PWF_MAX31856
{
 public:
  PWF_MAX31856(uint8_t CSx);
  void MAX31856_config(uint8_t TC_TYPE, uint8_t FILT_FREQ, uint8_t AVG_MODE, uint8_t MEAS_MODE);
  void MAX31856_CJ_offset(int8_t offset_val);	// offset is 2^-4 degC/bit
  void SPIbus_Init(void);
  void MAX31856_update(struct var_max31856 *tc_ptr);
  void MAX31856_1shot_start(void);
  void clear_fault();

 private:
  uint8_t _cs, _fault, _drdy;
  uint8_t _sing_reg_read(uint8_t RegAdd);
  void _sing_reg_write(uint8_t RegAdd, uint8_t BitMask, uint8_t RegData);
};

#endif
