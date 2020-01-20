#ifndef _PASSTHRUMISC_H_
#define _PASSTHRUMISC_H_

typedef struct {
	char	*szVendor;			// Vendor
	char    *szName;			// Device Name
	char    *szFunctionLibrary; // library Path
} PassThru_Driver;

typedef struct 
{
	PassThru_Driver*  librarys;
	unsigned int     count;
} PassThru_DriverArray;

const char* PassThru_Retval2Str(unsigned long RetVal);
int PassThru_loadDriver(PassThru_DriverArray &array);

#endif