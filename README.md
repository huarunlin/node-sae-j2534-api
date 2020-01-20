# Describe

SAE-J2534 with nodejs

# Install

```shell
npm install sae_j2534_api
```

# Method description

##  Attribute: drivers(array)

Host driver array, driver contents include:<br>
name: device/driver name.<br>
vendor: device vendor.<br>
library: driver library path.<br>

## Method: open(library)

This function is used to establish a connection and intialize the Pass-Thru Device.<br>
library: String, device library path.<br>
return: int, 0 is success.<br>

## Method: close()

This function is used to close the connection to a Pass-Thru Device.<br>
return: int, 0 is success.<br>

## Method: connect(protocol, baudrate, flags)

This function is used to establish a logical connection with a protocol channel on the specified SAE J2534 device.<br>
protocol: channel protocol, reference to the definition of saej2534.<br>
baudrate: bus baudrate.<br>
flags: connect flags, reference to the definition of saej2534.<br>
return: int, 0 is success.<br>

## Method: disconnect()

This function is used to terminate a logical connection with a protocol channel.<br>
return: int, 0 is success.<br>

## Method: recv(timeout)

This function reads messages and indications from the receive buffer.<br>
timeout: recv timeout, unit:  millisecond.<br>
return: object, contents include protocol, id, flags, payload.<br>

## Method: send(id, payload, timeout)

This function is used to send messages.<br>
id: send msg can id.<br>
payload: send msg data.<br>
timeout: send timeout, unit:  millisecond.<br>
return: int, 0 is success.<br>

## Method: startPeriodicMsg(id, payload, interval)

This function will immediately queue the specified message for transmission, and repeat at the specified interval.<br>
id: periodic msg can id.<br>
payload: periodic msg data.<br>
timeout: periodic interval, unit:  millisecond.<br>
return: object, contents include err, data. When err equals 0, the operation is successful and data is message id.<br>

## Method: stopPeriodicMsg(msgId)

This function stops the specified periodic message.<br>
msgId: specified message id.<br>
return: int, 0 is success.<br>

## Method: startMsgFilter(filterType, mask, pattern, flowControl)

This function starts filtering of incoming messages.<br>
filterType: filter type, PASS_FILTER/BLOCK_FILTER/FLOW_CONTROL_FILTER, reference to the definition of saej2534.<br>
mask: msg id mask.<br>
pattern: pattern msg id.<br>
flowControl: flow control msg id.<br>
return: object, contents include err, data. When err equals 0, the operation is successful and data is filter id.<br>

## Method: stopPeriodicMsg(filterId)

This function removes the specified filter.<br>
filterId: specified filter id.<br>
return: int, 0 is success.<br>

# Usage Example

```javascript
/**************************/
/* ProtocolID definitions */
/**************************/
const J1850VPW = 1;
const J1850PWM = 2;
const ISO9141  = 3;
const ISO14230 = 4;
const CAN	   = 5;
const ISO15765 = 6;
const SCI_A_ENGINE = 7;
const SCI_A_TRANS  = 8;
const SCI_B_ENGINE = 9;
const SCI_B_TRANS  = 10;

/*******************************/
/* PassThruConnect definitions */
/*******************************/
// 0 = Receive standard CAN ID (11 bit)
// 1 = Receive extended CAN ID (29 bit)
const CAN_29BIT_ID = 0x00000100;
// 0 = The interface will generate and append the checksum as defined in ISO 9141-2 and ISO 14230-2 for
// transmitted messages, and verify the checksum for received messages.
// 1 = The interface will not generate and verify the checksum-the entire message will be treated as
// data by the interface
const ISO9141_NO_CHECKSUM = 0x00000200;
// 0 = either standard or extended CAN ID types used ?CAN ID type defined by bit 8
// 1 = both standard and extended CAN ID types used ?if the CAN controller allows prioritizing either standard
// (11 bit) or extended (29 bit) CAN ID's then bit 8 will determine the higher priority ID type
const CAN_ID_BOTH = 0x00000800;
// 0 = use L-line and K-line for initialization address
// 1 = use K-line only line for initialization address
const ISO9141_K_LINE_ONLY = 0x00001000;

/************************/
/* RxStatus definitions */
/************************/
// 0 = received i.e. this message was transmitted on the bus by another node
// 1 = transmitted i.e. this is the echo of the message transmitted by the PassThru device
const TX_MSG_TYPE = 0x00000001;
// 0 = Not a start of message indication
// 1 = First byte or frame received
const START_OF_MESSAGE     = 0x00000002;
const ISO15765_FIRST_FRAME = 0x00000002;	/*v2 compat from v0202*/
const ISO15765_EXT_ADDR     = 0x00000080;	/*DT Accidentally refered to in spec*/
// 0 = No break received
// 1 = Break received
const RX_BREAK		= 0x00000004;
// 0 = No TxDone
// 1 = TxDone
const TX_INDICATION	= 0x00000008;	// Preferred name
const TX_DONE		= 0x00000008;
// 0 = No Error
// 1 = Padding Error
const ISO15765_PADDING_ERROR = 0x00000010;
// 0 = no extended address,
// 1 = extended address is first byte after the CAN ID
const ISO15765_ADDR_TYPE = 0x00000080;
//CAN_29BIT_ID							0x00000100  defined above
const SW_CAN_NS_RX = 0x00040000;	/*-2*/
const SW_CAN_HS_RX = 0x00020000;	/*-2*/
const SW_CAN_HV_RX = 0x00010000;	/*-2*/

/***********************/
/* TxFlags definitions */
/***********************/
// 0 = no padding
// 1 = pad all flow controlled messages to a full CAN frame using zeroes
const ISO15765_FRAME_PAD = 0x00000040;
//ISO15765_ADDR_TYPE					0x00000080  defined above
//CAN_29BIT_ID							0x00000100  defined above
// 0 = Interface message timing as specified in ISO 14230
// 1 = After a response is received for a physical request, the wait time shall be reduced to P3_MIN
// Does not affect timing on responses to functional requests
const WAIT_P3_MIN_ONLY = 0x00000200;
const SW_CAN_HV_TX = 0x00000400;	/*-2*/
// 0 = Transmit using SCI Full duplex mode
// 1 = Transmit using SCI Half duplex mode
const SCI_MODE = 0x00400000;
// 0 = no voltage after message transmit
// 1 = apply 20V after message transmit
const SCI_TX_VOLTAGE = 0x00800000;
const DT_PERIODIC_UPDATE = 0x10000000;	/*DT*/

/**********************/
/* Filter definitions */
/**********************/
// Allows matching messages into the receive queue. This filter type is only valid on non-ISO 15765 channels
const PASS_FILTER = 0x00000001;

// Keeps matching messages out of the receive queue. This filter type is only valid on non-ISO 15765 channels
const BLOCK_FILTER = 0x00000002;
// Allows matching messages into the receive queue and defines an outgoing flow control message to support
// the ISO 15765-2 flow control mechanism. This filter type is only valid on ISO 15765 channels.
const FLOW_CONTROL_FILTER =	0x00000003;

const binding = require('..');
const device = new binding.J2534();

let CANID_HOST = 0x74D;
let CANID_ECU  = 0x7CD;
let arr = device.drivers;
for(let i = 0; i < arr.length; i++){
    console.log("Discover Driver: ", arr[i].name);
}

console.log("open ", arr[2].name, " device."); 
if (0 != device.open(arr[2].library)) {
    console.log("device open failure."); 
    process.exit();
}

console.log("connect ISO15765."); 
if (0 != device.connect(ISO15765, 500000, CAN_ID_BOTH)) {
    console.log("device connect failure."); 
    process.exit();
}

var filter = device.startMsgFilter(FLOW_CONTROL_FILTER, 0x7FF, CANID_ECU, CANID_HOST);
if (0 != filter.err) {
    console.log("start msg filter failure."); 
    device.disconnect();
    device.close();
    process.exit();
}

var periodPayload = Buffer.from([0x3E]);
var periodMsg = device.startPeriodicMsg(CANID_HOST, periodPayload, 1000);
if (0 != periodMsg.err) {
    console.log("start period msg failure."); 
    device.disconnect();
    device.close();
    process.exit();
}

while(1) {
    var msg = device.recv(1000);
    if (0 != msg.err) {
        continue;
    }
    
    if ((TX_MSG_TYPE | START_OF_MESSAGE) & msg.flags) {
        continue;
    }
    console.log("Recv Msg");
    console.log("Protocol:", "0x" + msg.protocol.toString(16));
    console.log("Id:",       "0x" + msg.id.toString(16));
    console.log("Flags:",    "0x" + msg.flags.toString(16));
    console.log("Payload:",  msg.payload.toString('hex'));
    device.send(CANID_HOST, msg.payload, 1000);
}

if (0 != device.stopPeriodicMsg(periodMsg.id)) {
    console.log("stop msg period failure."); 
}

if (0 != device.stopMsgFilter(filter.id)) {
    console.log("stop msg filter failure."); 
}

if (0 != device.disconnect()) {
    console.log("device disconnect failure."); 
}

if (0 != device.close()) {
    console.log("device close failure.");
}
process.exit()
```