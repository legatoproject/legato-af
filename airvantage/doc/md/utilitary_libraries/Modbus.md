Modbus
======

Protocol overview
=================

Modbus Protocol is a messaging structure developed by Modicon in 1979.

- Master-slave/client-server communication between intelligent devices
- De facto standard
- Open
- Most widely used network protocol in the industrial manufacturing
  environment

Modbus is a "request response" protocol type

#### MODBUS Application Protocol


Key features:

- Application layer messaging protocol (OSI 7)
- Client/Server oriented
    - Reserved port TCP/IP 502
    - Request/reply protocol offering services specified by function codes
- MODBUS PDU (max 253 bytes) = [FuntionCode][Data]
- ADU = PDU + additionnal fields related to specific buses or networks = [Additional adress][FuntionCode][Data][ErrorCheck]
    - RS232/RS485 ADU max size = 256 bytes
    - TCP/IP ADU max size = 260 bytes
- 3 types of PDU
    - request
    - response
    - exception response
- big-Endian representation (to send value 0x1234, the first byte sent
  is 0x12 then 0x34)

#### MODBUS over serial line protocol

Key features:

- Data Link layer protocol (OSI 2)
- Master/Slave type (Master=Client, Slave=Server)
- Request/response oriented
    - Slave cannot respond without a request from the master
    - Slave cannot communicate with each other
- The master initialises only one transaction at a time
    - unicast mode, the master adresses a specific slave
    - broadcast mode, the master adresses all slaves but no reponse is returned
- MODBUS PDU (max 253 bytes) = [FuntionCode][Data]
- MODBUS serial line PDU = (max 256 bytes) = [Additional field][FuntionCode][Data][CRC (or LRC)]
- 2 transmission modes
    - RTU [SlaveAdress 1 byte][FunctionCode 1 byte][Data max 252 bytes][CRC 2 bytes]
    - ASCII [Start 1 char :][Adress 2 chars][Function 2 chars][Data max 2 * 252 chars][LRC 2 chars][End 2 chars CR,LF]

#### MODBUS data model

The MODBUS data model is based on a series of tables having
distinguishing characteristics.

The four primary tables are:

--------------------------------------------------------------------------------------------
Primary tables        Object type    Type of        Comments
------------------    -----------    -----------    ----------------------------------------
Discrete Input        Single bit     Read-Only      can be provided by an I/O system

Coils                 Single bit     Read-Write     can be altered by an apllication program

Input Registers       16-bit word    Read-Only      can be provided by an I/O system

Holding Registers     16-bit word    Read-Write     can be altered by an application program
--------------------------------------------------------------------------------------------


The distinction between inputs and outputs, and between bit-addressable
and word-adressable data items is conceptual. The tables may be
overlaying one another.\
 For each of the primary tables, the protocol allow individual selection
of 65536 data items.\
 All data handled via modbus (bits or registers) must be allocated in
device application memory but physical adress in memory should not be
confused with data reference.\
 Modbus logical reference are unsigned integer starting at 0.

Protocol implementations
========================

Supported code functions:

- READ_COILS = (0x01),
- READ_DISCRETE_INPUTS = (0x02),
- READ_HOLDING_REGISTERS = (0x03),
- READ_INPUT_REGISTERS = (0x04),
- WRITE_SINGLE_COIL = (0x05),
- WRITE_SINGLE_REGISTER = (0x06),
- READ_EXCEPTION_STATUS = (0x07),
- DIAGNOSTICS = (0x08),
- GET_COMM_EVENT_COUNTER = (0x0B),
- GET_COMM_EVENT_LOG = (0x0C),
- WRITE_MULTIPLE_COILS = (0x0F),
- WRITE_MULTIPLE_REGISTERS = (0x10),
- REPORT_SLAVE_ID = (0x11),
- READ_FILE_RECORD = (0x14),
- WRITE_FILE_RECORD = (0x15),
- MASK_WRITE_REGISTER = (0x16),
- READWRITE_MULTIPLE_REGISTERS = (0x17),
- READ_FIFO_QUEUE = (0x18),
- ENCAPSULATED_INTERFACE_TRANSPORT = (0x2B),

#### Lua implementation

This implementation is synchronous and thread safe.\
 It requires the 'pack' to manipulate string buffers.

##### Serial ASCII and RTU mode

The stack can be initialized with default parametres:

-   "m,err=modbus.new()" , partially
-   "m,err=modbus.new(nil, {baudrate=19200}) "

To start a GPIO to signal the transmission and reception phases use
"\_,err = m:enable\_hardware\_switch(12,'high')" and "\_,err =
m:disable\_hardware\_switch()".

Once initialized you have access to the 8 main MODBUS functions:

1.  <coilvalues\>,err=m:request('ReadCoils',<slaveid\>,<startingaddress\>,<numberofcoils\>)
2.  <inputvalues\>,err=m:request('ReadDiscreteInputs',<slaveid\>,<startingaddress\>,<numberofinputs\>)
3.  <registervalues\>,err=m:request('ReadHoldingRegisters',<slaveid\>,<startingaddress\>,<numberofregisters\>)
4.  <registervalues\>,err=m:request('ReadInputRegisters',<slaveid\>,<startingaddress\>,<numberofregisters\>)
5.  \_,err=m:request('WriteSingleCoil',<slaveid\>,<startingaddress\>,<coilstate\>)
6.  \_,err=m:request('WriteSingleRegister',<slaveid\>,<startingaddress\>,<registervalue\>)
7.  \_,err=m:request('WriteMultipleCoils',<slaveid\>,<startingaddress\>,<numberofcoils\>,<coilvalues\>)
8.  \_,err=m:request('WriteMultipleRegisters',<slaveid\>,<startingaddress\>,<registervalues\>)

coil and register values are specified as string buffers.

The stack is explicitely closed by calling "m:close()".

~~~~{.lua}
require 'modbus'
m,err=modbus.new('/dev/ttyS0',{baudrate=9600,parity='even',timeout=3},'RTU')
data,err=m:request('ReadHoldingRegisters',1,0,40)
if err then print(err) end
if data then
  res={string.unpack(data,"<H40")}
  table.remove(res,1)
  p(res)
end
~~~~

### TCP mode or ASCII and RTU over TCP mode

The stack can be initialized with default parametres:

-   "m,err=modbus.new()" , partially
-   "m,err=modbus.new(nil, 'TCP') "
-   or fully "m,err=modbus.new({timeout=5,maxsocket=2},'RTU')".

Once initialized you have access to the 8 main MODBUS functions:

1.  <coilvalues\>,err=m:request('ReadCoils',<host\>,<port\>,<slaveid\>,<startingaddress\>,<numberofcoils\>)
2.  <inputvalues\>,err=m:request('ReadDiscreteInputs',<host\>,<port\>,<slaveid\>,<startingaddress\>,<numberofinputs\>)
3.  <registervalues\>,err=m:request('ReadHoldingRegisters',<host\>,<port\>,<slaveid\>,<startingaddress\>,<numberofregisters\>)
4.  <registervalues\>,err=m:request('ReadInputRegisters',<host\>,<port\>,<slaveid\>,<startingaddress\>,<numberofregisters\>)
5.  _,err=m:request('WriteSingleCoil',<host\>,<port\>,<slaveid\>,<startingaddress\>,<coilstate\>)
6.  _,err=m:request('WriteSingleRegister',<host\>,<port\>,<slaveid\>,<startingaddress\>,<registervalue\>)
7.  _,err=m:request('WriteMultipleCoils',<host\>,<port\>,<slaveid\>,<startingaddress\>,<numberofcoils\>,<coilvalues\>)
8.  _,err=m:request('WriteMultipleRegisters',<host\>,<port\>,<slaveid\>,<startingaddress\>,<registervalues\>)

<host\> is either an ip address or a resolvable hostname.\
 <port\> can be set to nil, in this case the default value is 502.\
 coil and register values are specified as string buffers.

The stack is explicitely closed by calling "m:close()".

~~~~{.lua}
require 'modbustcp'
m,err=modbustcp.new('/dev/ttyS0', {timeout=1,maxsocket=2})

data,err=m:request('ReadHoldingRegisters','10.41.51.164',502,1,0,40)
if err then print(err) end
if data then
  res={string.unpack(data,"<H40")}
  table.remove(res,1)
  p(res)
end
~~~~

