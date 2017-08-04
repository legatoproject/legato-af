include common.mk

SUBDIRS := $(patsubst %/Makefile,%,$(wildcard */Makefile))

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $*
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all $(SUBDIRS) clean
