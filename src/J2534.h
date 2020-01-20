#ifndef _J2534_H_
#define _J2534_H_

#include <node_api.h>
#include "PassThruDevice.h"
#include "PassThruMisc.h"

class J2534 
{
public:
    J2534();
    virtual ~J2534();

    static void J2534::Init(napi_env *env, napi_value *exports);
    static napi_value structure(napi_env env, napi_callback_info info);
    static void destructor(napi_env env, void* nativeObject, void* finalize_hint);

    static napi_value drivers(napi_env env, napi_callback_info info);
    static napi_value open(napi_env env, napi_callback_info info);
    static napi_value close(napi_env env, napi_callback_info info);
    static napi_value connect(napi_env env, napi_callback_info info);
    static napi_value disconnect(napi_env env, napi_callback_info info);
    static napi_value send(napi_env env, napi_callback_info info);
    static napi_value recv(napi_env env, napi_callback_info info);
    static napi_value startMsgFilter(napi_env env, napi_callback_info info);
    static napi_value stopMsgFilter(napi_env env, napi_callback_info info);
    static napi_value startPeriodicMsg(napi_env env, napi_callback_info info);
    static napi_value stopPeriodicMsg(napi_env env, napi_callback_info info);
private:
    static bool  getParm(napi_env &env, napi_callback_info &info, J2534 **obj, napi_value *args, size_t *argc, void **data);

    static napi_ref   _constructor;
    static PassThru_DriverArray _driverArray;
    napi_env          _env;
    napi_ref          _wrapper;
    PassThruDevice    _device;
};

#endif