
include ../common.mk

$(TARGET):
	mkapp hello.adef -b -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices
	mksys hello-system.sdef -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices
