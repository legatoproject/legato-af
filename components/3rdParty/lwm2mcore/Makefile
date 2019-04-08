LWM2MCORE_SRC=\
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/sessionManager/dtlsConnection.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/sessionManager/lwm2mcoreSession.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/aclConfiguration.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/bootstrapConfiguration.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/objects.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/objectsTable.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/handlers.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/utils.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/packageDownloader/lwm2mcorePackageDownloader.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/packageDownloader/update.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/packageDownloader/workspace.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/objectManager/lwm2mcoreCoapHandlers.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/security/lwm2mcoreSecurity.c

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
    -std=gnu99 \
    -DWITH_SHA256 \
    -DLEGACY_FW_STATUS \
    -DLWM2M_DEREGISTER \
    -DAV_SYSTEM_CONFIGURATION

ifeq ($(DEBUG),yes)
    CFLAGS += -g
endif

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

LWM2MCORE_OBJ=$(patsubst ${LEGATO_ROOT}/3rdParty/Lwm2mCore/%.c,${OBJDIR}/%.o,${LWM2MCORE_SRC})

lwm2mcore: liblwm2mcore.so liblwm2mcore.a

$(OBJDIR)/%.o: ${LEGATO_ROOT}/3rdParty/Lwm2mCore/%.c
	${LOG_DEBUG}mkdir -p $(dir $@)
	${LOG_DEBUG}${CC} ${DEPFLAGS} ${CFLAGS} ${INCLUDES} -c -o $@ $<

liblwm2mcore.so: ${LWM2MCORE_OBJ}
	${LOG_DEBUG}${CC} -shared ${DEPFLAGS} ${LDFLAGS} -Wl,-soname,$@ -o $@ ${LWM2MCORE_OBJ}

liblwm2mcore.a: ${LWM2MCORE_OBJ}
	${LOG_DEBUG}${AR} rcs $@ ${LWM2MCORE_OBJ}
