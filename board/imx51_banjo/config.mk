ifeq ($(O),prod)
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

TEXT_BASE = 0x1ffe5000
else
LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/bist.lds

TEXT_BASE = 0x97800000
endif
