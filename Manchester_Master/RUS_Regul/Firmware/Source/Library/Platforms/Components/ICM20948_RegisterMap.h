// ICM20948_RegisterMap.h
// Сборка ICM-20948 (3xAccelerometer, 3xGyroscope and AK09916 3xMagnetometer)
// Описание регистров
//
// Основано на icm20948.h и ICM20948_mag.h
// https://raw.githubusercontent.com/PX4/Firmware/master/src/drivers/imu/icm20948/icm20948.h
// https://raw.githubusercontent.com/PX4/Firmware/master/src/drivers/imu/icm20948/ICM20948_mag.h
#ifndef	ICM20948_REGISTER_MAP_H
#define	ICM20948_REGISTER_MAP_H
/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/


#define ICM_WHOAMI_20948            0xEA

// ICM20948 registers and data

/*
 * ICM20948 I2C address LSB can be switched by the chip's AD0 pin, thus is device dependent.
 * Noting this down for now. Here GPS uses 0x69. To support a device implementing the second
 * address, probably an additional MPU_DEVICE_TYPE is the way to go.
 */
#define PX4_I2C_EXT_ICM20948_0			0x68
#define PX4_I2C_EXT_ICM20948_1			0x69

/*
 * ICM20948 uses register banks. Register 127 (0x7F) is used to switch between 4 banks.
 * There's room in the upper address byte below the port speed setting to code in the
 * used bank. This is a bit more efficient, already in use for the speed setting and more
 * in one place than a solution with a lookup table for address/bank pairs.
 */

#define BANK0	0x0000
#define BANK1	0x0100
#define BANK2	0x0200
#define BANK3	0x0300

#define BANK_REG_MASK	0x0300
#define REG_BANK(r) 			(((r) & BANK_REG_MASK)>>4)
#define REG_ADDRESS(r)			((r) & ~BANK_REG_MASK)

#define ICMREG_20948_BANK_SEL 0x7F

#define	ICMREG_20948_WHOAMI					(0x00 | BANK0)
#define ICMREG_20948_USER_CTRL				(0x03 | BANK0)
#define ICMREG_20948_LP_CONFIG				(0x05 | BANK0)
#define ICMREG_20948_PWR_MGMT_1				(0x06 | BANK0)
#define ICMREG_20948_PWR_MGMT_2				(0x07 | BANK0)
#define ICMREG_20948_INT_PIN_CFG			(0x0F | BANK0)
#define ICMREG_20948_INT_ENABLE				(0x10 | BANK0)
#define ICMREG_20948_INT_ENABLE_1			(0x11 | BANK0)
#define ICMREG_20948_ACCEL_XOUT_H			(0x2D | BANK0)
#define ICMREG_20948_INT_ENABLE_2			(0x12 | BANK0)
#define ICMREG_20948_INT_ENABLE_3			(0x13 | BANK0)
#define ICMREG_20948_EXT_SLV_SENS_DATA_00	(0x3B | BANK0)
#define ICMREG_20948_GYRO_SMPLRT_DIV		(0x00 | BANK2)
#define ICMREG_20948_GYRO_CONFIG_1			(0x01 | BANK2)
#define ICMREG_20948_GYRO_CONFIG_2			(0x02 | BANK2)
#define ICMREG_20948_ACCEL_SMPLRT_DIV_1		(0x10 | BANK2)
#define ICMREG_20948_ACCEL_SMPLRT_DIV_2		(0x11 | BANK2)
#define ICMREG_20948_ACCEL_CONFIG			(0x14 | BANK2)
#define ICMREG_20948_ACCEL_CONFIG_2			(0x15 | BANK2)
#define ICMREG_20948_I2C_MST_CTRL			(0x01 | BANK3)
#define ICMREG_20948_I2C_SLV0_ADDR			(0x03 | BANK3)
#define ICMREG_20948_I2C_SLV0_REG			(0x04 | BANK3)
#define ICMREG_20948_I2C_SLV0_CTRL			(0x05 | BANK3)
#define ICMREG_20948_I2C_SLV0_DO			(0x06 | BANK3)



/*
* ICM20948 register bits
* Most of the regiser set values from ICM20948 have the same
* meaning on ICM20948. The exceptions and values not already
* defined for ICM20948 are defined below
*/
#define ICM_BIT_PWR_MGMT_1_ENABLE       	0x00
#define ICM_BIT_PWR_MGMT_1_RESET      	 	0x80
#define ICM_BIT_PWR_MGMT_1_SLEEP      	 	0x40
#define ICM_BIT_PWR_MGMT_1_LP_EN      	 	0x20
#define ICM_BIT_PWR_MGMT_1_CLKSEL_MASK 	 	0x07
#define ICM_BIT_USER_CTRL_I2C_MST_DISABLE   0x00
#define ICM_BIT_USER_CTRL_I2C_MST_ENABLE    0x20
#define ICM_BIT_INT_PIN_CFG_BYPASS_EN		0x02

#define ICM_BITS_GYRO_DLPF_CFG_197HZ		0x01
#define ICM_BITS_GYRO_DLPF_CFG_151HZ		0x09
#define ICM_BITS_GYRO_DLPF_CFG_119HZ		0x11
#define ICM_BITS_GYRO_DLPF_CFG_51HZ			0x19
#define ICM_BITS_GYRO_DLPF_CFG_23HZ			0x21
#define ICM_BITS_GYRO_DLPF_CFG_11HZ			0x29
#define ICM_BITS_GYRO_DLPF_CFG_5HZ			0x31
#define ICM_BITS_GYRO_DLPF_CFG_361HZ		0x39
#define ICM_BITS_GYRO_DLPF_CFG_MASK			0x39

#define ICM_BITS_GYRO_FS_SEL_250DPS			0x00
#define ICM_BITS_GYRO_FS_SEL_500DPS			0x02
#define ICM_BITS_GYRO_FS_SEL_1000DPS		0x04
#define ICM_BITS_GYRO_FS_SEL_2000DPS		0x06
#define ICM_BITS_GYRO_FS_SEL_MASK			0x06
#define ICM_BITS_GYRO_CHOICE				0x01

#define ICM_BITS_ACCEL_DLPF_CFG_246HZ_1125	0x01
#define ICM_BITS_ACCEL_DLPF_CFG_246HZ		0x09
#define ICM_BITS_ACCEL_DLPF_CFG_111HZ		0x11
#define ICM_BITS_ACCEL_DLPF_CFG_50HZ		0x19
#define ICM_BITS_ACCEL_DLPF_CFG_23HZ		0x21
#define ICM_BITS_ACCEL_DLPF_CFG_11HZ		0x29
#define ICM_BITS_ACCEL_DLPF_CFG_5HZ			0x31
#define ICM_BITS_ACCEL_DLPF_CFG_473HZ		0x39
#define ICM_BITS_ACCEL_DLPF_CFG_MASK		0x39

#define ICM_BITS_ACCEL_FS_SEL_2G			0x00
#define ICM_BITS_ACCEL_FS_SEL_4G			0x02
#define ICM_BITS_ACCEL_FS_SEL_8G			0x04
#define ICM_BITS_ACCEL_FS_SEL_16G			0x06
#define ICM_BITS_ACCEL_FS_SEL_MASK			0x06
#define ICM_BITS_ACCEL_CHOICE				0x01

#define ICM_BITS_DEC3_CFG_4					0x00
#define ICM_BITS_DEC3_CFG_8					0x01
#define ICM_BITS_DEC3_CFG_16				0x10
#define ICM_BITS_DEC3_CFG_32				0x11
#define ICM_BITS_DEC3_CFG_MASK				0x11

#define ICM_BITS_I2C_MST_CLOCK_370KHZ    	0x00
#define ICM_BITS_I2C_MST_CLOCK_400HZ    	0x07	// recommended by datasheet for 400kHz target clock



#define AK09916_I2C_ADDR         0x0C
//#define AK09916_DEVICE_ID        0x48

#define AK09916REG_WIA1          0x00
#define AK09916REG_WIA2          0x01

/* ak09916 deviating register addresses and bit definitions */

#define AK09916_DEVICE_ID_A		0x48	// same as AK09916
#define AK09916_DEVICE_ID_B		0x09	// additional ID byte ("INFO" on AK9063 without content specification.)

#define AK09916REG_ST1        0x10
#define AK09916REG_HXL        0x11
#define AK09916REG_HXH        0x12
#define AK09916REG_HYL        0x13
#define AK09916REG_HYH        0x14
#define AK09916REG_HZL        0x15
#define AK09916REG_HZH        0x16
#define AK09916REG_ST2        0x18
#define AK09916REG_CNTL1      0x30
#define AK09916REG_CNTL2      0x31
#define AK09916REG_CNTL3      0x32


#define AK09916_CNTL2_POWERDOWN_MODE            0x00
#define AK09916_CNTL2_SINGLE_MODE               0x01 /* default */
#define AK09916_CNTL2_CONTINOUS_MODE_10HZ       0x02
#define AK09916_CNTL2_CONTINOUS_MODE_20HZ       0x04
#define AK09916_CNTL2_CONTINOUS_MODE_50HZ       0x06
#define AK09916_CNTL2_CONTINOUS_MODE_100HZ      0x08
#define AK09916_CNTL2_SELFTEST_MODE             0x10
#define AK09916_CNTL3_SRST                      0x01
#define AK09916_ST1_DRDY                        0x01
#define AK09916_ST1_DOR                         0x02


#endif	// ICM20948_REGISTER_MAP_H
