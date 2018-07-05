#ifndef INC_MX51_I2C
#define INC_MX51_I2C

#include "i2c.h"

#define CONFIG_I2C_BUS1	1
#define CONFIG_I2C_BUS2	2

#define CONFIG_I2C_AUDIO_CODEC	1
#define CONFIG_I2C_SMART_BATTERY	1
#define CONFIG_I2C_ACCELEROMETER	1

enum i2c_bus_index {
	NOT_USED = 0,
#ifdef CONFIG_I2C_AUDIO_CODEC
	I2C_BUS_AUDIO_CODEC,
#endif
#ifdef CONFIG_I2C_SMART_BATTERY
	I2C_BUS_SMART_BATTERY,
#endif
#ifdef CONFIG_I2C_ACCELEROMETER
	I2C_BUS_ACCELEROMETER,
#endif
};

//#define I2C_TIMEOUT	(CONFIG_SYS_HZ / 4)

#define I2C_READ_BIT  1
#define I2C_WRITE_BIT 0

DECLARE_GLOBAL_DATA_PTR;

//
#define IOMUXC_SW_MUX_CTL_PAD_GPIO1_2  		(IOMUXC_BASE_ADDR + 0x3cc)
#define IOMUXC_SW_MUX_CTL_PAD_GPIO1_3  		(IOMUXC_BASE_ADDR + 0x3d0)
#define IOMUXC_I2C2_IPP_SCL_IN_SELECT_INPUT  	(IOMUXC_BASE_ADDR + 0x9b8)
#define IOMUXC_I2C2_IPP_SDA_IN_SELECT_INPUT  	(IOMUXC_BASE_ADDR + 0x9bc)
#define IOMUXC_SW_PAD_CTL_PAD_GPIO1_2  		(IOMUXC_BASE_ADDR + 0x7d4)
#define IOMUXC_SW_PAD_CTL_PAD_GPIO1_3  		(IOMUXC_BASE_ADDR + 0x7d8)

//
#define IOMUXC_SW_MUX_CTL_PAD_SD2_CMD  		(IOMUXC_BASE_ADDR + 0x3b4)
#define IOMUXC_SW_MUX_CTL_PAD_SD2_CLK  		(IOMUXC_BASE_ADDR + 0x3b8)
#define IOMUXC_SW_PAD_CTL_PAD_SD2_CMD  		(IOMUXC_BASE_ADDR + 0x7bc)
#define IOMUXC_SW_PAD_CTL_PAD_SD2_CLK  		(IOMUXC_BASE_ADDR + 0x7c0)
#define IOMUXC_I2C1_IPP_SCL_IN_SELECT_INPUT  	(IOMUXC_BASE_ADDR + 0x9b0)
#define IOMUXC_I2C1_IPP_SDA_IN_SELECT_INPUT  	(IOMUXC_BASE_ADDR + 0x9b4)

//
#define I2C_CLKDIV 		0x18			//MY-TODO: fine tune
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

typedef struct imx51_i2c_base {
	u32 base_clock;	//addr for clock control register
	u32 data_clock;	//data for clock control

	u32 base_iadr;	//addr for IADR
	u32 base_ifdr;	//addr for IFDR
	u32 base_i2cr;	//addr for I2CR
	u32 base_i2sr;	//addr for I2SR
	u32 base_i2dr;	//addr for I2DR
} imx51_i2c_base_t;

typedef struct imx51_i2c_dev {
	u8 chip;		//devie addr
	u8 alen;		//register addr length
	struct imx51_i2c_base *i2c_comm;	//share same physical attribute

	u32 base_switch;	//mutiplexer, GPIO control?
	u32 base_enable;	//mutiplexer, GPIO control?
	u8 channel;		//channelnumber
	u8 busid;		//bus number
	u8 found;		//on the bus
	
	u32 addr;		//register addr
	int dlen;		//data length
	u8 *name;
//	u8 *buf;
} imx51_i2c_dev_t;

#endif	//INC_MX51_I2C
