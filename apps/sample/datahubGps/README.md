DataHub Sample Applications
==================

Consists of Sensor App and Monitor App.
Sensor App reads the GPS coordinates, introduces some additional oscillations, and feeds the data to the DataHub.
Monitor App creates the "Dead Band" filter to receive notifications only when the coordinates go beyond the pre-defined limits.

Building
--------

The system (that would include default Legato system, plus DataHub, plus sample apps)
can be built/installed using the following commands:

    cd legato
    mksys -t <target> apps/sample/datahubGps/datahubGps.sdef
    update datahubGps.<target>.update

Testing
-------

On the device, to view and control the Data Hub inputs and outputs:

    alias dhub='/legato/systems/current/appsWriteable/dataHub/bin/dhub'
    dhub list

For more information:

    dhub --help
