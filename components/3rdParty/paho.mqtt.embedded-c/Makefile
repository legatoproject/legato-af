
PAHO_PACKET_PATH=${LEGATO_ROOT}/3rdParty/paho.mqtt.embedded-c/MQTTPacket/src

LIBDIR="${LEGATO_BUILD}/3rdParty/paho.mqtt.embedded-c/lib"

PAHO_SOURCES=\
    ${PAHO_PACKET_PATH}/MQTTConnectClient.c \
    ${PAHO_PACKET_PATH}/MQTTFormat.c \
    ${PAHO_PACKET_PATH}/MQTTUnsubscribeClient.c \
    ${PAHO_PACKET_PATH}/MQTTPacket.c \
    ${PAHO_PACKET_PATH}/MQTTDeserializePublish.c \
    ${PAHO_PACKET_PATH}/MQTTSubscribeClient.c \
    ${PAHO_PACKET_PATH}/MQTTSerializePublish.c

CFLAGS+=\
    -I${PAHO_PACKET_PATH} \
    -Wno-unused-parameter \
    -DREVERSED \
    -DMQTT_TASK

LOG_DEBUG=@
ifeq ($(VERBOSE),1)
    LOG_DEBUG=
endif

PAHO_OBJS=$(notdir $(patsubst %.c,%.o,${PAHO_SOURCES}))

mqttpacketclient: libmqttpacketclient.a

libmqttpacketclient.a: ${PAHO_OBJS}
	${LOG_DEBUG}${AR} rcs $@ $(PAHO_OBJS)

%.o: ${PAHO_PACKET_PATH}/%.c
	${LOG_DEBUG}${CC} ${CFLAGS} -c $< -o $@
