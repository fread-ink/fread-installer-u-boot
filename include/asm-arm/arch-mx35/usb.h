#include <asm/arch/mx35.h>
#include <asm/arch/iomux.h>

static inline u32 fsl_readl(u32 *r)
{
	u32 v = __REG(r);
	printf("RD [%08x] => %08x\n", r, v);
	return v;
}

static inline void fsl_writel(u32 v, u32 *r)
{
	printf("WR [%08x] <= %08x\n", r, v);
	__REG(r) = v;
}

#define USB_BASE		0x53FF4000

#define fsl_stop_usb()		/* dummy for now */

#define cpu_to_le16(x) 		(x)
#define cpu_to_le32(x)		(x)
#define le32_to_cpu(x)		(x)
#define le16_to_cpu(x)		(x)

static inline void fsl_start_usb(void ) 
{
#if 0
        mxc_request_iomux(MX31_PIN_CSPI1_MOSI,      /* USBH1_RXDM */
                              OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1); 
        mxc_request_iomux(MX31_PIN_CSPI1_MISO,      /* USBH1_RXDP */
                              OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
        mxc_request_iomux(MX31_PIN_CSPI1_SS0,       /* USBH1_TXDM */
                              OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
        mxc_request_iomux(MX31_PIN_CSPI1_SS1,       /* USBH1_TXDP */
                            OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
        mxc_request_iomux(MX31_PIN_CSPI1_SS2,       /* USBH1_RCV  */
                           OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
        mxc_request_iomux(MX31_PIN_CSPI1_SCLK,      /* USBH1_OEB (_TXOE) */
                              OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);
        mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY,   /* USBH1_FS   */
                              OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1);

        mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI,          /* USBH1_RXDM */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO,          /* USBH1_RXDP */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0,           /* USBH1_TXDM */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1,           /* USBH1_TXDP */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2,           /* USBH1_RCV  */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK,          /* USBH1_OEB (_TXOE) */
                          (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

        mxc_request_iomux(MX31_PIN_GPIO1_6,
                                OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
        mxc_iomux_set_pad(MX31_PIN_GPIO1_6,
                                PAD_CTL_DRV_NORMAL | PAD_CTL_ODE_OpenDrain |
                                PAD_CTL_HYS_CMOS | PAD_CTL_22K_PU |
                                PAD_CTL_SRE_SLOW);
        mxc_set_gpio_direction(MX31_PIN_GPIO1_6, 0); // output
        mxc_set_gpio_dataout(MX31_PIN_GPIO1_6, 0);
#endif
}

