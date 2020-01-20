
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "j2534_v0404.h"
#include "PassThruMisc.h"

const char* PassThru_Retval2Str(unsigned long RetVal) {
    switch (RetVal){
        case STATUS_NOERROR:			return "STATUS_NOERROR";				// Assigned in J2534-1
        case ERR_NOT_SUPPORTED:			return "ERR_NOT_SUPPORTED";
        case ERR_INVALID_CHANNEL_ID:	return "ERR_INVALID_CHANNEL_ID";
        case ERR_INVALID_PROTOCOL_ID:	return "ERR_INVALID_PROTOCOL_ID";
        case ERR_NULL_PARAMETER:		return "ERR_NULL_PARAMETER";
        case ERR_INVALID_IOCTL_VALUE:	return "ERR_INVALID_IOCTL_VALUE";
        case ERR_INVALID_FLAGS:			return "ERR_INVALID_FLAGS";
        case ERR_FAILED:				return "ERR_FAILED";
        case ERR_DEVICE_NOT_CONNECTED:	return "ERR_DEVICE_NOT_CONNECTED";
        case ERR_TIMEOUT:				return "ERR_TIMEOUT";
        case ERR_INVALID_MSG:			return "ERR_INVALID_MSG";
        case ERR_INVALID_TIME_INTERVAL:	return "ERR_INVALID_TIME_INTERVAL";
        case ERR_EXCEEDED_LIMIT:		return "ERR_EXCEEDED_LIMIT";
        case ERR_INVALID_MSG_ID:		return "ERR_INVALID_MSG_ID";
        case ERR_DEVICE_IN_USE:			return "ERR_DEVICE_IN_USE";
        case ERR_INVALID_IOCTL_ID:		return "ERR_INVALID_IOCTL_ID";
        case ERR_BUFFER_EMPTY:			return "ERR_BUFFER_EMPTY";
        case ERR_BUFFER_FULL:			return "ERR_BUFFER_FULL";
        case ERR_BUFFER_OVERFLOW:		return "ERR_BUFFER_OVERFLOW";
        case ERR_PIN_INVALID:			return "ERR_PIN_INVALID";
        case ERR_CHANNEL_IN_USE:		return "ERR_CHANNEL_IN_USE";
        case ERR_MSG_PROTOCOL_ID:		return "ERR_MSG_PROTOCOL_ID";
        case ERR_INVALID_FILTER_ID:		return "ERR_INVALID_FILTER_ID";
        case ERR_NO_FLOW_CONTROL:		return "ERR_NO_FLOW_CONTROL";
        case ERR_NOT_UNIQUE:			return "ERR_NOT_UNIQUE";
        case ERR_INVALID_BAUDRATE:		return "ERR_INVALID_BAUDRATE";
        case ERR_INVALID_DEVICE_ID:		return "ERR_INVALID_DEVICE_ID";
        default: break;
    }

    if (RetVal >= 0x00010000 && RetVal <= 0xFFFFFFFF) {
        return "?J2534-2?";
    } else if (RetVal >= 0x0000001B && RetVal <= 0x0000FFFF) {
        return "?J2534-1?";
    }
    return "?retval?";
}

int PassThru_loadDriver(PassThru_DriverArray &array)
{
	int dwCnt = 0, index = 0;
	HKEY hKey1,hKey2,hKey3;
	FILETIME FTime;
	long hKey2RetVal;
	DWORD VendorIndex;
	DWORD KeyType;
	DWORD KeySize;
	DWORD lMaxValueLen;
	LSTATUS retval;
	TCHAR * KeyValue;
	PassThru_Driver *driver;

	// Open HKLM/Software
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software"), 0, KEY_READ | KEY_WOW64_32KEY, &hKey1) != ERROR_SUCCESS)
	{//strcpy_s(J2534BoilerplateErrorResult, sizeof(J2534BoilerplateErrorResult), "Can't open HKEY_LOCAL_MACHINE->Software key.");
		return dwCnt;
	}
	// Open HKLM/Software/PassThruSupport.04.04
	if (RegOpenKeyEx(hKey1, _T("PassThruSupport.04.04"), 0, KEY_READ | KEY_WOW64_32KEY, &hKey2) != ERROR_SUCCESS)
	{//strcpy_s(J2534BoilerplateErrorResult, sizeof(J2534BoilerplateErrorResult), "Can't open HKEY_LOCAL_MACHINE->..->PassThruSupport.04.04 key");
		RegCloseKey(hKey1);
		return dwCnt;
	}
	RegCloseKey(hKey1);

	// Determine the maximum subkey length for HKLM/Software/PassThruSupport.04.04/*
	DWORD lMaxSubKeyLen;
	RegQueryInfoKey(hKey2, NULL, NULL, NULL, NULL, &lMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

	// Allocate a buffer large enough to hold that name
	KeyValue = new TCHAR[lMaxSubKeyLen+1];

	// Iterate through HKLM/Software/PassThruSupport.04.04/*
	VendorIndex = 0;
	do
	{
		// Get the name of HKLM/Software/PassThruSupport.04.04/VendorDevice[i]
		KeySize = lMaxSubKeyLen+1;
		hKey2RetVal = RegEnumKeyEx(hKey2, VendorIndex++, KeyValue, &KeySize, NULL, NULL, NULL, &FTime);
		if (hKey2RetVal != ERROR_SUCCESS) continue;

		// Open HKLM/Software/PassThruSupport.04.04/Vendor[i]
		if (RegOpenKeyEx(hKey2, KeyValue, 0, KEY_READ | KEY_WOW64_64KEY, &hKey3) == ERROR_SUCCESS) dwCnt++;
	}while(hKey2RetVal == ERROR_SUCCESS);

	if(dwCnt <= 0)
	{
		RegCloseKey(hKey2);
		goto RETURN;
	}

	array.count = dwCnt;
	array.librarys	= new PassThru_Driver[dwCnt];
	VendorIndex = 0;
	do
	{
		driver = array.librarys + VendorIndex;
		// Get the name of HKLM/Software/PassThruSupport.04.04/VendorDevice[i]
		KeySize = lMaxSubKeyLen+1;
		hKey2RetVal = RegEnumKeyEx(hKey2, VendorIndex++, KeyValue, &KeySize, NULL, NULL, NULL, &FTime);
		if (hKey2RetVal != ERROR_SUCCESS)  continue;

		// Open HKLM/Software/PassThruSupport.04.04/Vendor[i]
		if (RegOpenKeyEx(hKey2, KeyValue, 0, KEY_READ, &hKey3) != ERROR_SUCCESS) continue;
		

		// Determine the maximum value length for HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/*
		retval = RegQueryInfoKey(hKey3, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &lMaxValueLen, NULL, NULL);

		// Read HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/Vendor
		driver->szVendor = new char[lMaxValueLen+1];
		KeySize = lMaxValueLen+1;
		retval = RegQueryValueEx(hKey3, _T("Vendor"), 0, &KeyType, (LPBYTE) driver->szVendor, &KeySize);

		// Read HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/Name
		driver->szName = new char[lMaxValueLen+1];
		KeySize = lMaxValueLen+1;
		retval = RegQueryValueEx(hKey3, _T("Name"), 0, &KeyType, (LPBYTE) driver->szName, &KeySize);

		// Read HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/FunctionLibrary
		driver->szFunctionLibrary = new char[lMaxValueLen+1];
		KeySize = lMaxValueLen+1;
		retval = RegQueryValueEx(hKey3, _T("FunctionLibrary"), 0, &KeyType, (LPBYTE) driver->szFunctionLibrary, &KeySize);

		RegCloseKey(hKey3);
	}while(hKey2RetVal == ERROR_SUCCESS);
	RegCloseKey(hKey2);

RETURN:
	if(KeyValue) delete KeyValue;
	return  dwCnt;
}