#include <string.h>
//#include <Winbase.h>
#include "PassThruDevice.h"
#include "PassThruMisc.h"
#include "Common.h"

#define PASSTHRU_GET_LASTERROR(_ret_)                                     \
    {                                                                     \
        if (STATUS_NOERROR != _ret_)                                      \
        {                                                                 \
            tr_err("%s: %s\r\n", __FUNCTION__, PassThru_Retval2Str(_ret_)); \
        }                                                                 \
    }

PassThruDevice::PassThruDevice()
{
    _mutex = CreateMutex(NULL,FALSE,NULL);

    _isOpen = FALSE;
    _isConnect = FALSE;

    memset(&_api, 0x00, sizeof(PassThru_Api));
    _device = 0;
    _channel = 0;
}

PassThruDevice::~PassThruDevice()
{
    close();
}

void PassThruDevice::lock()
{
    WaitForSingleObject(_mutex,INFINITE); 
}

void PassThruDevice::unlock()
{
    ReleaseMutex(_mutex);
}

bool PassThruDevice::loadApi(const char *path)
{
    if (!path) {
        tr_err("[%s]: Device library path is NULL.\r\n", __FUNCTION__);
        return FALSE;
    }

    _api._dll = LoadLibrary(path);
    if (_api._dll == NULL) {
        tr_err("[%s]: Failed to open device library '%s'\r\n", __FUNCTION__, path);
        return FALSE;
    }
    _api._open = (PTOPEN)GetProcAddress(_api._dll, "PassThruOpen");
    _api._close = (PTCLOSE)GetProcAddress(_api._dll, "PassThruClose");
    _api._connect = (PTCONNECT)GetProcAddress(_api._dll, "PassThruConnect");
    _api._disconnect = (PTDISCONNECT)GetProcAddress(_api._dll, "PassThruDisconnect");
    _api._readMsgs = (PTREADMSGS)GetProcAddress(_api._dll, "PassThruReadMsgs");
    _api._writeMsgs = (PTWRITEMSGS)GetProcAddress(_api._dll, "PassThruWriteMsgs");
    _api._startPeriodicMsg = (PTSTARTPERIODICMSG)GetProcAddress(_api._dll, "PassThruStartPeriodicMsg");
    _api._stopPeriodicMsg = (PTSTOPPERIODICMSG)GetProcAddress(_api._dll, "PassThruStopPeriodicMsg");
    _api._startMsgFilter = (PTSTARTMSGFILTER)GetProcAddress(_api._dll, "PassThruStartMsgFilter");
    _api._stopMsgFilter = (PTSTOPMSGFILTER)GetProcAddress(_api._dll, "PassThruStopMsgFilter");
    _api._setProgrammingVoltage = (PTSETPROGRAMMINGVOLTAGE)GetProcAddress(_api._dll, "PassThruSetProgrammingVoltage");
    _api._readVersion = (PTREADVERSION)GetProcAddress(_api._dll, "PassThruReadVersion");
    _api._getLastError = (PTGETLASTERROR)GetProcAddress(_api._dll, "PassThruGetLastError");
    _api._ioctl = (PTIOCTL)GetProcAddress(_api._dll, "PassThruIoctl");
    return TRUE;
}

bool PassThruDevice::unloadApi()
{
    if (_api._dll) {
        FreeLibrary(_api._dll);
        memset(&_api, 0x00, sizeof(PassThru_Api));
    }
    return TRUE;
}

bool PassThruDevice::open(const char *path)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;

    lock();
    if (_isOpen) {
        goto RETURN;
    }

    if (!loadApi(path)) {
        goto RETURN;
    }

    PassThruRet = _api._open(NULL, &_device);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(ret);
    } else {
        _isOpen = TRUE;
        ret = TRUE;
    }
RETURN:
    if (!ret) {
        memset(&_api, 0x00, sizeof(PassThru_Api));
        unloadApi();
    }
    unlock();
    return ret;
}

bool PassThruDevice::close()
{
    lock();
    if (_isConnect) {
        unlock();
        disconnect();
        lock();
    }
    if (_isOpen) {
        _api._close(_device);
        _isOpen = false;
        unloadApi();
    }
    unlock();
    return TRUE;
}

bool PassThruDevice::connect(unsigned long protocol,
                             unsigned long baudrate,
                             unsigned long flags,
                             bool echo)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;
    SCONFIG CfgItem;
    SCONFIG_LIST Input;

    lock();
    if (!_isOpen) {
        goto RETURN;
    }

    if (_isConnect) {
        goto RETURN;
    }

    PassThruRet = _api._connect(_device, protocol, flags, baudrate, &_channel);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
        goto RETURN;
    }
    _protocol = protocol;
    _isConnect = TRUE;
    ret = TRUE;
    /* Config Echo */
    CfgItem.Parameter = LOOPBACK;
    CfgItem.Value = 0x01;
    Input.NumOfParams = echo ? 1 : 0;
    Input.ConfigPtr = &CfgItem;
    _api._ioctl(_channel, SET_CONFIG, (void *)&Input, NULL);
RETURN:
    unlock();
    return ret;
}

bool PassThruDevice::disconnect()
{
    lock();
    if (_isConnect) {
        _api._disconnect(_channel);
        _isConnect = FALSE;
    }
    unlock();
    return TRUE;
}

bool PassThruDevice::startMsgFilter(unsigned long filterType,
                                    unsigned int mask,
                                    unsigned int pattern,
                                    unsigned int flowControl,
                                    unsigned long *filterId)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }

    memset(&_maskMsg, 0x00, sizeof(PASSTHRU_MSG));
    _maskMsg.ProtocolID = _protocol;
    _maskMsg.Data[0] = (unsigned char)((mask >> 24) & 0xFF);
    _maskMsg.Data[1] = (unsigned char)((mask >> 16) & 0xFF);
    _maskMsg.Data[2] = (unsigned char)((mask >> 8) & 0xFF);
    _maskMsg.Data[3] = (unsigned char)((mask >> 0) & 0xFF);
    _maskMsg.DataSize = 4;
    if (FLOW_CONTROL_FILTER == filterType) {
        _maskMsg.TxFlags |= ISO15765_FRAME_PAD;
    }
    if (IS_29BIT_ID(mask)) {
        _maskMsg.TxFlags |= CAN_29BIT_ID;
    }

    memset(&_patternMsg, 0x00, sizeof(PASSTHRU_MSG));
    _patternMsg.ProtocolID = _protocol;
    _patternMsg.Data[0] = (unsigned char)((pattern >> 24) & 0xFF);
    _patternMsg.Data[1] = (unsigned char)((pattern >> 16) & 0xFF);
    _patternMsg.Data[2] = (unsigned char)((pattern >> 8) & 0xFF);
    _patternMsg.Data[3] = (unsigned char)((pattern >> 0) & 0xFF);
    _patternMsg.DataSize = 4;
    if (FLOW_CONTROL_FILTER == filterType) {
        _patternMsg.TxFlags |= ISO15765_FRAME_PAD;
    }
    if (IS_29BIT_ID(pattern)) {
        _patternMsg.TxFlags |= CAN_29BIT_ID;
    }

    memset(&_flowControlMsg, 0x00, sizeof(PASSTHRU_MSG));
    _flowControlMsg.ProtocolID = _protocol;
    _flowControlMsg.Data[0] = (unsigned char)((flowControl >> 24) & 0xFF);
    _flowControlMsg.Data[1] = (unsigned char)((flowControl >> 16) & 0xFF);
    _flowControlMsg.Data[2] = (unsigned char)((flowControl >> 8) & 0xFF);
    _flowControlMsg.Data[3] = (unsigned char)((flowControl >> 0) & 0xFF);
    _flowControlMsg.DataSize = 4;
    if (FLOW_CONTROL_FILTER == filterType) {
        _flowControlMsg.TxFlags |= ISO15765_FRAME_PAD;
    }
    if (IS_29BIT_ID(flowControl)) {
        _flowControlMsg.TxFlags |= CAN_29BIT_ID;
    }

    PassThruRet = _api._startMsgFilter(_channel, filterType, &_maskMsg, &_patternMsg, &_flowControlMsg, filterId);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
    } else {
        ret = TRUE;
    }
RETURN:    
    unlock();
    return ret;
}

bool PassThruDevice::stopMsgFilter(unsigned long filterId)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }
    PassThruRet = _api._stopMsgFilter(_channel, filterId);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
    } else {
        ret = TRUE;
    }
RETURN:    
    unlock();
    return ret;
}

bool PassThruDevice::startPeriodicMsg(unsigned int canId,
                                      const unsigned char* data, 
                                      unsigned long len,
                                      unsigned long interval,
                                      unsigned long *msgId)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }

    memset(&_writeMsg, 0x00, sizeof(PASSTHRU_MSG));
    _writeMsg.ProtocolID = _protocol;
    _writeMsg.Data[0] = (unsigned char)((canId >> 24) & 0xFF);
    _writeMsg.Data[1] = (unsigned char)((canId >> 16) & 0xFF);
    _writeMsg.Data[2] = (unsigned char)((canId >> 8) & 0xFF);
    _writeMsg.Data[3] = (unsigned char)((canId >> 0) & 0xFF);
    memcpy(&_writeMsg.Data[4], data, len);
    _writeMsg.DataSize = 4 + len;
    if ( ISO15765 == _protocol) {
        _writeMsg.TxFlags |= ISO15765_FRAME_PAD;
    }
    if (IS_29BIT_ID(canId)) {
        _writeMsg.TxFlags |= CAN_29BIT_ID;
    }

    PassThruRet = _api._startPeriodicMsg(_channel, &_writeMsg, msgId, interval);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
    } else {
        ret = TRUE;
    }
RETURN:    
    unlock();
    return ret;
}

bool PassThruDevice::stopPeriodicMsg(unsigned long msgId)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }
    PassThruRet = _api._stopPeriodicMsg(_channel, msgId);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
    } else {
        ret = TRUE;
    }
RETURN:    
    unlock();
    return ret;   
}

bool PassThruDevice::send(unsigned int canId,
                          const unsigned char* data, 
                          unsigned long len,
                          unsigned long timeout)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;
    unsigned long numMsg = 1;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }

    memset(&_writeMsg, 0x00, sizeof(PASSTHRU_MSG));
    _writeMsg.ProtocolID = _protocol;
    _writeMsg.Data[0] = (unsigned char)((canId >> 24) & 0xFF);
    _writeMsg.Data[1] = (unsigned char)((canId >> 16) & 0xFF);
    _writeMsg.Data[2] = (unsigned char)((canId >> 8) & 0xFF);
    _writeMsg.Data[3] = (unsigned char)((canId >> 0) & 0xFF);
    memcpy(&_writeMsg.Data[4], data, len);
    _writeMsg.DataSize = 4 + len;
    if ( ISO15765 == _protocol) {
        _writeMsg.TxFlags |= ISO15765_FRAME_PAD;
    }
    if (IS_29BIT_ID(canId)) {
        _writeMsg.TxFlags |= CAN_29BIT_ID;
    }

    PassThruRet = _api._writeMsgs(_channel, &_writeMsg, &numMsg, timeout);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
    } else {
        ret = TRUE;
    }
RETURN:    
    unlock();
    return ret;    
}

bool PassThruDevice::recv(unsigned int *canId,
                          unsigned long *protocol,
                          unsigned char **data, 
                          unsigned long *len,
                          unsigned long *flags,
                          unsigned long timeout)
{
    bool ret = FALSE;
    long PassThruRet = STATUS_NOERROR;
    unsigned long numMsg = 1;

    lock();
    if (!_isConnect) {
        tr_err("[%s]: Device not connect.\r\n", __FUNCTION__);
        goto RETURN;
    }

    PassThruRet = _api._readMsgs(_channel, &_readMsg, &numMsg, timeout);
    if (STATUS_NOERROR != PassThruRet) {
        PASSTHRU_GET_LASTERROR(PassThruRet);
        goto RETURN;
    }

    *canId = _readMsg.Data[0] << 24;
    *canId |= _readMsg.Data[1] << 16;
    *canId |= _readMsg.Data[2] << 8;
    *canId |= _readMsg.Data[3] << 0;
    *data = &_readMsg.Data[4];
    *len = _readMsg.DataSize - 4;
    *protocol = _readMsg.ProtocolID;
    *flags = _readMsg.RxStatus;
    ret = TRUE;
RETURN:    
    unlock();
    return ret;     
}