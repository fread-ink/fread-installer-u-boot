#ifndef INC_MX50_I2C
#define INC_MX50_I2C

#include "i2c.h"

#define I2C_READ_BIT  1
#define I2C_WRITE_BIT 0

DECLARE_GLOBAL_DATA_PTR;

//
#define IOMUXC_SW_MUX_CTL_PAD_I2C1_SCL  	(IOMUXC_BASE_ADDR + 0x040)
#define IOMUXC_SW_MUX_CTL_PAD_I2C1_SDA		(IOMUXC_BASE_ADDR + 0x044)
#define IOMUXC_SW_PAD_CTL_PAD_I2C1_SCL  	(IOMUXC_BASE_ADDR + 0x2ec)
#define IOMUXC_SW_PAD_CTL_PAD_I2C1_SDA		(IOMUXC_BASE_ADDR + 0x2f0)

//
#define IOMUXC_SW_MUX_CTL_PAD_I2C2_SCL  	(IOMUXC_BASE_ADDR + 0x048)
#define IOMUXC_SW_MUX_CTL_PAD_I2C2_SDA		(IOMUXC_BASE_ADDR + 0x04c)
#define IOMUXC_SW_PAD_CTL_PAD_I2C2_SCL  	(IOMUXC_BASE_ADDR + 0x2f4)
#define IOMUXC_SW_PAD_CTL_PAD_I2C2_SDA		(IOMUXC_BASE_ADDR + 0x2f8)

//
#define I2C_CLKDIV 		0x18
#define I2C_WAIT_TIMEOUT	10000
#define I2C_READ_TIMEOUT	10000
#define I2C_WRITE_TIMEOUT	10000

#define IADR	0x00
#define IFDR	0x04
#define I2CR	0x08
#define I2SR	0x0c
#define I2DR	0x10

#define I2C1_IADR 	(I2C1_BASE_ADDR + IADR)
#define I2C1_IFDR	(I2C1_BASE_ADDR + IFDR)
#define I2C1_I2CR	(I2C1_BASE_ADDR + I2CR)
#define I2C1_I2SR	(I2C1_BASE_ADDR + I2SR)
#define I2C1_I2DR	(I2C1_BASE_ADDR + I2DR)

#define I2C2_IADR 	(I2C2_BASE_ADDR + IADR)
#define I2C2_IFDR	(I2C2_BASE_ADDR + IFDR)
#define I2C2_I2CR	(I2C2_BASE_ADDR + I2CR)
#define I2C2_I2SR	(I2C2_BASE_ADDR + I2SR)
#define I2C2_I2DR	(I2C2_BASE_ADDR + I2DR)

#define I2C3_IADR 	(I2C3_BASE_ADDR + IADR)
#define I2C3_IFDR	(I2C3_BASE_ADDR + IFDR)
#define I2C3_I2CR	(I2C3_BASE_ADDR + I2CR)
#define I2C3_I2SR	(I2C3_BASE_ADDR + I2SR)
#define I2C3_I2DR	(I2C3_BASE_ADDR + I2DR)

#define I2CR_IEN		(1 << 7)
#define I2CR_IIEN		(1 << 6)
#define I2CR_MSTA		(1 << 5)
#define I2CR_MTX		(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA		(1 << 2)

#define I2SR_ICF		(1 << 7)
#define I2SR_IBB		(1 << 5)
#define I2SR_IIF		(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

typedef struct imx50_i2c_base {
	u32 base_clock;	//addr for clock control register
	u32 data_clock;	//data for clock control

	u32 base_iadr;	//addr for IADR
	u32 base_ifdr;	//addr for IFDR
	u32 base_i2cr;	//addr for I2CR
	u32 base_i2sr;	//addr for I2SR
	u32 base_i2dr;	//addr for I2DR
} imx50_i2c_base_t;

typedef struct imx50_i2c_dev {
	u8 chip;		//devie addr
	u8 alen;		//register addr length
	struct imx50_i2c_base *i2c_comm;	//share same physical attribute

	u32 base_switch;	//mutiplexer, GPIO control?
	u32 base_enable;	//mutiplexer, GPIO control?
	u8 channel;		//channelnumber
	u8 busid;		//bus number
	u8 found;		//on the bus
	
	u32 addr;		//register addr
	int dlen;		//data length
	u8 *name;
//	u8 *buf;
} imx50_i2c_dev_t;

#endif	//INC_MX50_I2C
