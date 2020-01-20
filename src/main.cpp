#include <node_api.h>
#include <stdio.h>
#include "J2534.h"

napi_value Init(napi_env env, napi_value exports) {
  J2534::Init(&env, &exports);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)