MQTT Sample Applications
==================

Consists of publisher and subscriber modules.
Publisher periodically publishes the data on the MQTT server, using the MQTT client service.
Subscriber receives the published data.

Building
--------

The system (that would include default Legato system, plus MQTT client, plus MQTT sample apps)
can be built using the command:

    cd legato
    mksys -t <target> apps/sample/mqtt/mqttSample.sdef

Install the system:

    update mqttSample.<target>.update 192.168.2.2

Start the publisher and subscriber:

    app start mqttPublisher
    app start mqttSubscriber

MQTT broker
----------

Sample app assumes that Mosquitto MQTT server is running on the Linux workstation, and allows
unsecured connection using the URL

    tcp://192.168.2.3:1883

Testing
-------

1) Device log should contain the periodic messages from publisher and subscriber.

2) MQTT Mosquitto subscriber command-line client can run on different host, e.g.
on the Linux workstation, and be able to receive the published messages:

    $ mosquitto_sub -t <device_id>/messages/json
    {"value":3}
    {"value":4}
    {"value":5}
