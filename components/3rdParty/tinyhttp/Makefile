TINYHTTP_SOURCES=\
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinyhttp/chunk.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinyhttp/header.c \
    ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinyhttp/http.c

INCLUDES=\
    -I${LEGATO_ROOT}/framework/include \
    -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/3rdParty/tinyhttp/include

CFLAGS+=\
    -w \
    -std=gnu99

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

TINYHTTP_OBJ=$(patsubst ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinyhttp/%.c,${OBJDIR}/%.o,${TINYHTTP_SOURCES})

tinyhttp: libtinyhttp.so libtinyhttp.a

$(OBJDIR)/%.o: ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinyhttp/%.c
	${LOG_DEBUG}mkdir -p $(dir $@)
	${LOG_DEBUG}${CC} ${DEPFLAGS} ${CFLAGS} ${INCLUDES} -c -o $@ $<

libtinyhttp.so: ${TINYHTTP_OBJ}
	${LOG_DEBUG}${CC} -shared ${DEPFLAGS} ${LDFLAGS} -Wl,-soname,$@ -o $@ ${TINYHTTP_OBJ}

libtinyhttp.a: ${TINYHTTP_OBJ}
	${LOG_DEBUG}${AR} rcs $@ $(TINYHTTP_OBJ)
