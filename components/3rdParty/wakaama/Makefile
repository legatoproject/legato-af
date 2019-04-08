WAKAAMA_SOURCES=\
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/er-coap-13/er-coap-13.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/acl.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/bootstrap.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/block1.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/block1-stream.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/block2-stream.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/data.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/discover.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/json.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/liblwm2m.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/list.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/management.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/objects.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/observe.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/packet.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/registration.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/tlv.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/transaction.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/uri.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/utils.c

INCLUDES=\
    -I${LEGATO_ROOT}/framework/include \
    -I${LEGATO_ROOT}/apps/platformServices/airVantageConnector/avcClient \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/platform-specific/linux \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/sessionManager \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/packageDownloader \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/core/er-coap-13 \
    -I${LEGATO_BUILD}/3rdParty/tinydtls/include/tinydtls

CFLAGS+=\
    -DLWM2M_CLIENT_MODE \
    -DLWM2M_LITTLE_ENDIAN \
    -DLWM2M_BOOTSTRAP \
    -DLWM2M_OLD_CONTENT_FORMAT_SUPPORT \
    -DLEGATO_LWM2M_CLIENT \
    -DWITH_TINYDTLS \
    -DLWM2M_WITH_LOGS \
    -DWITH_LOGS \
    -DSIERRA \
    -w \
    -std=gnu99 \
    -DWITH_SHA256 \
    -DCOAP_BLOCK1_SIZE=4096 \
    -DLWM2M_DEREGISTER \
    -DAV_SYSTEM_CONFIGURATION

ifeq ($(TEST_COVERAGE),1)
    CFLAGS +=--coverage
    LDFLAGS+=--coverage
endif

LOG_DEBUG=@
ifeq ($(VERBOSE),1)
    LOG_DEBUG=
endif

DEPFLAGS=-fPIC

OBJDIR?=./obj

WAKAAMA_OBJ=$(patsubst ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/%.c,${OBJDIR}/%.o,${WAKAAMA_SOURCES})

wakaama: libwakaama.so libwakaama.a

$(OBJDIR)/%.o: ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/wakaama/%.c
	${LOG_DEBUG}mkdir -p $(dir $@)
	${LOG_DEBUG}${CC} ${DEPFLAGS} ${CFLAGS} ${INCLUDES} -c -o $@ $<

libwakaama.so: ${WAKAAMA_OBJ}
	${LOG_DEBUG}${CC} -shared ${DEPFLAGS} ${LDFLAGS} -Wl,-soname,$@ -o $@ ${WAKAAMA_OBJ}

libwakaama.a: ${WAKAAMA_OBJ}
	${LOG_DEBUG}${AR} rcs $@ $(WAKAAMA_OBJ)