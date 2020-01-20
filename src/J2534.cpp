#include <string.h>
#include <stdio.h>
#include "J2534.h"
#include "Common.h"

PassThru_DriverArray J2534::_driverArray = { 0, 0 };
napi_ref J2534::_constructor;

J2534::J2534() 
{

}

J2534::~J2534() 
{

}

void J2534::Init(napi_env *env, napi_value *exports) 
{
    PassThru_loadDriver(_driverArray);
    napi_property_descriptor properties[] = {
        { "drivers",    NULL,   NULL,         drivers,    NULL,       0,  napi_default,   NULL },
        { "open",       NULL,   open,         NULL,       NULL,       0,  napi_default,   NULL },
        { "close",      NULL,   close,        NULL,       NULL,       0,  napi_default,   NULL },
        { "connect",    NULL,   connect,      NULL,       NULL,       0,  napi_default,   NULL },
        { "disconnect", NULL,   disconnect,   NULL,       NULL,       0,  napi_default,   NULL },
        { "send",       NULL,   send,         NULL,       NULL,       0,  napi_default,   NULL },
        { "recv",       NULL,   recv,         NULL,       NULL,       0,  napi_default,   NULL },
        { "startMsgFilter",    NULL,   startMsgFilter,     NULL,       NULL,       0,  napi_default,   NULL },
        { "stopMsgFilter",     NULL,   stopMsgFilter,      NULL,       NULL,       0,  napi_default,   NULL },
        { "startPeriodicMsg",  NULL,   startPeriodicMsg,   NULL,       NULL,       0,  napi_default,   NULL },
        { "stopPeriodicMsg",   NULL,   stopPeriodicMsg,    NULL,       NULL,       0,  napi_default,   NULL },
    };

    napi_value cons;
    napi_define_class(*env, "J2534", NAPI_AUTO_LENGTH, structure, NULL, sizeof(properties) / sizeof(napi_property_descriptor), properties, &cons);
    napi_create_reference(*env, cons, 1, &_constructor);
    napi_set_named_property(*env, *exports, "J2534", cons);
}

napi_value J2534::structure(napi_env env, napi_callback_info info) 
{
    napi_status status;
    napi_value new_target;
    napi_value _this;

    status = napi_get_cb_info(env, info, NULL, NULL, &_this, NULL);
    if (status != napi_ok) {
        tr_err("structure: napi_get_cb_info error.\r\n");
        return NULL;
    }

    napi_get_new_target(env, info, &new_target);
    if (new_target != NULL) {
        // Invoked as constructor: `new J2534(...)`
        J2534* obj = new J2534();
        obj->_env = env;
        napi_wrap(env, _this, obj, J2534::destructor, NULL, &obj->_wrapper);
        return _this;
    } else {
        // Invoked as plain function_wrap `J2534(...)`, turn into construct call.
        napi_value cons;
        napi_value instance;
        status = napi_get_reference_value(env, _constructor, &cons);
        if (status != napi_ok) {
            tr_err("structure: napi_get_reference_value error.\r\n");
            return NULL;
        }

        status = napi_new_instance(env, cons, NULL, NULL, &instance);
        if (status != napi_ok) {
            tr_err("structure: napi_new_instance error.\r\n");
            return NULL;
        }
        return instance;
    }
}

void J2534::destructor(napi_env env, void* nativeObject, void*) 
{
    J2534* obj = static_cast<J2534*>(nativeObject);
    if (obj) delete obj;
}

bool J2534::getParm(napi_env &env, napi_callback_info &info, J2534 **obj, napi_value *args, size_t *argc, void **data) 
{
    napi_status status;
    napi_value  thisValue;

    if (data) {
        status = napi_get_cb_info(env, info, argc, args, &thisValue,  (void**)(data));
    } else {
        status = napi_get_cb_info(env, info, argc, args, &thisValue,  NULL);
    }
    if (status != napi_ok) {
        tr_err("napi_get_cb_info error.\r\n");
        return false;
    }

    napi_unwrap(env, thisValue, reinterpret_cast<void**>(obj));
    if (status != napi_ok) {
        tr_err("napi_get_cb_info error.\r\n");
        return false;
    }
    return true;
}

napi_value J2534::drivers(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value  ret;
    J2534      *obj;


    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        return NULL;
    }

    status = napi_create_array_with_length(env, _driverArray.count, &ret);
    if (status != napi_ok) {
        tr_err("J2534::drivers: napi_create_array_with_length error.\r\n");
        return NULL;
    }

    for (unsigned int i = 0; i < _driverArray.count; i++) {
        napi_value driver, name, vendor, library;

        napi_create_object(env, &driver);
        
        napi_create_string_utf8(env, _driverArray.librarys[i].szName, NAPI_AUTO_LENGTH, &name);
        napi_set_named_property(env, driver, "name", name);
        
        napi_create_string_utf8(env, _driverArray.librarys[i].szVendor, NAPI_AUTO_LENGTH, &vendor);
        napi_set_named_property(env, driver, "vendor", vendor);
        
        napi_create_string_utf8(env, _driverArray.librarys[i].szFunctionLibrary, NAPI_AUTO_LENGTH, &library);
        napi_set_named_property(env, driver, "library", library);
        
        napi_set_element(env, ret, i, driver);
    }
    return ret;
}

napi_value J2534::open(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value  ret;
    int32_t     errCode = -1;
    size_t      argc = 1;
    napi_value  args[1];
    J2534      *obj;
    size_t      size;
    char        path[256];

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_string_utf8(env, args[0], path, 256, &size);
    if (status != napi_ok || size <= 0) {
        tr_err("CMbus::connect: Invaild driver library path.\r\n");
        goto exit;
    }

    if (obj->_device.open(path)) {
        errCode = 0;
    }
    
exit:
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value J2534::close(napi_env env, napi_callback_info info)
{
    napi_value  ret;
    int32_t     errCode = -1;
    J2534 *obj;

    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        goto exit;
    } 

    if (obj->_device.close()) {
        errCode = 0;
    }
exit:
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value J2534::connect(napi_env env, napi_callback_info info) 
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 3;
    napi_value  args[3];
    J2534    *obj;
    unsigned long protocol;
    unsigned long flags;
    unsigned long baudrate;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&protocol);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild protocol.\r\n");
        goto exit;
    }
    status = napi_get_value_uint32(env, args[1], (uint32_t*)&baudrate);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild baudrate.\r\n");
        goto exit;
    }
    status = napi_get_value_uint32(env, args[2], (uint32_t*)&flags);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild flags.\r\n");
        goto exit;
    }
    if (obj->_device.connect(protocol, baudrate, flags)) {
        errCode = 0;
    }
exit:
    napi_value  ret;
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value J2534::disconnect(napi_env env, napi_callback_info info) {
    napi_value  ret;
    int32_t     errCode = -1;
    J2534 *obj;

    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        goto exit;
    } 

    if (obj->_device.disconnect()) {
        errCode = 0;
    }
exit:
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value J2534::send(napi_env env, napi_callback_info info)
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 3;
    napi_value  args[3];
    J2534    *obj;
    unsigned int canId, timeout;
    size_t      payloadLen;
    unsigned char *payload;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&canId);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild filterType.\r\n");
        goto exit;
    }
    status = napi_get_buffer_info(env, args[1], (void**)&payload, &payloadLen);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild payload.\r\n");
        errCode = -2;
        goto exit; 
    }
    status = napi_get_value_uint32(env, args[2], (uint32_t*)&timeout);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild timeout.\r\n");
        goto exit;
    }
    if (obj->_device.send(canId, payload, payloadLen, timeout)) {
        errCode = 0;
    }
exit:
    napi_value  ret;
    napi_create_int32(env, errCode, &ret);
    return ret;    
}

napi_value J2534::recv(napi_env env, napi_callback_info info)
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 3;
    napi_value  args[3];
    J2534    *obj;
    unsigned int canId;
    unsigned long protocol = 0, flags = 0, timeout;
    unsigned long payloadLen = 0;
    unsigned char *payload;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&timeout);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild timeout.\r\n");
        goto exit;
    }

    if (obj->_device.recv(&canId, &protocol, &payload, &payloadLen, &flags, timeout)) {
        errCode = 0;
    }

exit:
    napi_value ret, err, napi_protocol, napi_canId, napi_payload, napi_flags;
    napi_create_object(env, &ret);
    napi_create_int32(env, errCode, &err);
    napi_create_uint32(env, protocol, &napi_protocol);
    napi_create_uint32(env, canId, &napi_canId);
    napi_create_uint32(env, flags, &napi_flags);
    napi_create_buffer_copy(env, payloadLen, payload, NULL, &napi_payload);
    napi_set_named_property(env, ret, "err", err);
    napi_set_named_property(env, ret, "protocol", napi_protocol);
    napi_set_named_property(env, ret, "id", napi_canId);
    napi_set_named_property(env, ret, "flags", napi_flags);
    napi_set_named_property(env, ret, "payload", napi_payload);
    return ret;    
}

napi_value J2534::startMsgFilter(napi_env env, napi_callback_info info) 
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 4;
    napi_value  args[4];
    J2534    *obj;
    unsigned long filterType, filterId;
    unsigned int mask, pattern, flowControl;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&filterType);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild filterType.\r\n");
        goto exit;
    }
    status = napi_get_value_uint32(env, args[1], (uint32_t*)&mask);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild mask.\r\n");
        goto exit;
    }
    status = napi_get_value_uint32(env, args[2], (uint32_t*)&pattern);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild pattern.\r\n");
        goto exit;
    }
    status = napi_get_value_uint32(env, args[3], (uint32_t*)&flowControl);
    if (status != napi_ok) {
        tr_err("J2534::connect: Invaild flowControl.\r\n");
        goto exit;
    }
    if (obj->_device.startMsgFilter(filterType, mask, pattern, flowControl, &filterId)) {
        errCode = 0;
    }
exit:
    napi_value ret, err, napi_id;
    napi_create_object(env, &ret);
    napi_create_int32(env, errCode, &err);
    napi_create_uint32(env, filterId, &napi_id);
    napi_set_named_property(env, ret, "err", err);
    napi_set_named_property(env, ret, "id", napi_id);

    return ret;
}

napi_value J2534::stopMsgFilter(napi_env env, napi_callback_info info) 
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 1;
    napi_value  args[1];
    J2534    *obj;
    unsigned long filterId;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&filterId);
    if (status != napi_ok) {
        tr_err("J2534::stopMsgFilter: Invaild filterId.\r\n");
        goto exit;
    }
    if (obj->_device.stopMsgFilter(filterId)) {
        errCode = 0;
    }
exit:
    napi_value ret;
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value J2534::startPeriodicMsg(napi_env env, napi_callback_info info)
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 3;
    napi_value  args[3];
    J2534    *obj;
    unsigned int canId, interval;
    unsigned long msgId;
    size_t      payloadLen;
    unsigned char *payload;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&canId);
    if (status != napi_ok) {
        tr_err("J2534::startPeriodicMsg: Invaild filterType.\r\n");
        goto exit;
    }
    status = napi_get_buffer_info(env, args[1], (void**)&payload, &payloadLen);
    if (status != napi_ok) {
        tr_err("J2534::startPeriodicMsg: Invaild payload.\r\n");
        errCode = -2;
        goto exit; 
    }
    status = napi_get_value_uint32(env, args[2], (uint32_t*)&interval);
    if (status != napi_ok) {
        tr_err("J2534::startPeriodicMsg: Invaild interval.\r\n");
        goto exit;
    }
    if (obj->_device.startPeriodicMsg(canId, payload, payloadLen, interval, &msgId)) {
        errCode = 0;
    }
exit:
    napi_value ret, err, napi_id;
    napi_create_object(env, &ret);
    napi_create_int32(env, errCode, &err);
    napi_create_uint32(env, msgId, &napi_id);
    napi_set_named_property(env, ret, "err", err);
    napi_set_named_property(env, ret, "id", napi_id);
    return ret;
}

napi_value J2534::stopPeriodicMsg(napi_env env, napi_callback_info info)
{
    napi_status status;
    int32_t     errCode = -1;
    size_t      argc = 1;
    napi_value  args[1];
    J2534    *obj;
    unsigned long msgId;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        goto exit;
    } 

    status = napi_get_value_uint32(env, args[0], (uint32_t*)&msgId);
    if (status != napi_ok) {
        tr_err("J2534::stopPeriodicMsg: Invaild msgId.\r\n");
        goto exit;
    }
    if (obj->_device.stopPeriodicMsg(msgId)) {
        errCode = 0;
    }
exit:
    napi_value  ret;
    napi_create_int32(env, errCode, &ret);
    return ret;
}