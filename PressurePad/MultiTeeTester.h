#ifndef _MULTITEETESTER_H_
#define _MULTITEETESTER_H_

#include <windows.h>


#ifndef USB_API
#define USB_API  _declspec(dllexport)
#else
#endif

#ifdef __cplusplus
extern "C"{
#endif
	unsigned int __stdcall TeeTesterCommunicate(LPVOID CommnunicateHandle);

	USB_API
		DWORD _stdcall GetDevicesNum();

	USB_API
		void _stdcall GetSerialNumberSingle(DWORD dwDeviceIndex, LPWSTR lpSerialNumber);

	USB_API
		BOOL _stdcall Open(DWORD dwDeviceIndex);

	USB_API
		void _stdcall Close();

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C"{
#endif
	USB_API
		BOOL _stdcall SetPowA(BYTE bPowA);
	USB_API
		BOOL _stdcall SetButton1();
	USB_API
		BOOL _stdcall SetButton2();
	USB_API
		BOOL _stdcall ClearButton1Info();
	USB_API
		BOOL _stdcall ClearButton2Info();
	USB_API
		BOOL _stdcall GetButton1Info(BYTE* lpbResult);
	USB_API
		BOOL _stdcall GetButton2Info(BYTE* lpbResult);
	USB_API
		BOOL _stdcall CollectFrame(BYTE* lpbResult);
	USB_API
		BOOL _stdcall SenseLink(BYTE* lpbResult);

#ifdef __cplusplus
}
#endif

#endif