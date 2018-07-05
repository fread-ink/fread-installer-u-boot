ifeq ($(TYPE),prod)
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

TEXT_BASE = 0xF8007000
else
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/bist.lds

TEXT_BASE = 0x79800000
endif
