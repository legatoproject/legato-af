include common.mk

SUBDIRS := $(wildcard */.)

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $*
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all $(SUBDIRS) clean
