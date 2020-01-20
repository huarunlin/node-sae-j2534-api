#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>

#define  tr_info(_format_, ...)       printf("[INFO] " _format_, ##__VA_ARGS__)
#define  tr_warn(_format_, ...)       printf("[WARN] " _format_, ##__VA_ARGS__)
#define  tr_err(_format_, ...)        printf("[ERR] " _format_, ##__VA_ARGS__)

#endif