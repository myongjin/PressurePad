// Multi-TeeTester_API.cpp : 定义 DLL 应用程序的导出函数。
//
#include "pch.h"
#include "MultiTeeTester.h"

#include <process.h> 
#include <initguid.h>		
#include <setupapi.h>
#include <stdlib.h> 
#pragma comment (lib, "setupapi.lib")


DEFINE_GUID(GUID_INTERFACE_SILABS_BULK,
	0x37538c66, 0x9584, 0x42d3, 0x96, 0x32, 0xeb, 0xad, 0xa, 0x23, 0xd, 0x13);

#define SILABS_BULK_WRITEPIPE	TEXT("PIPE01")
#define SILABS_BULK_READPIPE	TEXT("PIPE00")

#define USB_DEIVCE 0x00
#define USB_DEIVCE_WRITE 0x01
#define USB_DEIVCE_READ 0x02

HANDLE hTeeTester[8], hTeeTesterWrite[8], hTeeTesterRead[8], hCommunicationEvent[8];
TCHAR DevicePath[8][256];
TCHAR HardwareID[8][256], SerialNumber[8][256];//HardwareID: VID PID BCDDEVICE
BYTE DeviceNumber, DeviceSortedIndex[8], Button1, Button2;

struct TeeComunication
{
	BYTE DeviceIndex;
	LPVOID lpBufferToWrite;
	DWORD dwBytesToWrite;
	LPVOID lpBufferToRead;
	DWORD dwBytesToRead;
};


USB_API
DWORD _stdcall GetDevicesNum()
{
	GUID m_GUID = GUID_INTERFACE_SILABS_BULK;
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&m_GUID, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
	SP_DEVINFO_DATA DeviceInfoData;

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//MessageBox(L"Please plug device first!", L"ERROR!!!", MB_ICONERROR);
		DeviceNumber = 0;
		return 0;
	}
	//enumerate Teemo
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (DeviceNumber = 0; SetupDiEnumDeviceInfo(hDevInfo, DeviceNumber, &DeviceInfoData) && (DeviceNumber < 9); DeviceNumber++)
	{
		SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)HardwareID, sizeof(HardwareID), NULL);
		//Serial number
		SP_DEVICE_INTERFACE_DATA deviceInfoData;
		PSP_DEVICE_INTERFACE_DETAIL_DATA  functionClassDeviceData = NULL;
		ULONG  predictedLength, requiredLength;

		ZeroMemory(&deviceInfoData, sizeof(deviceInfoData));
		deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		SetupDiEnumDeviceInterfaces(hDevInfo, 0, &m_GUID, DeviceNumber, &deviceInfoData);

		predictedLength = requiredLength = 0;
		SetupDiGetDeviceInterfaceDetail(hDevInfo,
			&deviceInfoData,
			NULL,
			0,
			&requiredLength,
			NULL);
		predictedLength = requiredLength;
		functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);
		functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
		SP_DEVINFO_DATA did = { sizeof(SP_DEVINFO_DATA) };
		SetupDiGetDeviceInterfaceDetail(hDevInfo,
			&deviceInfoData,
			functionClassDeviceData,
			predictedLength,
			&requiredLength,
			&did);
		memcpy(DevicePath[DeviceNumber], functionClassDeviceData->DevicePath, lstrlen(functionClassDeviceData->DevicePath) * 2 + 2);
		DWORD dwSignCount = 0;
		DWORD dwStrIndex = 0;
		for (DWORD dwCount = 0; DevicePath[DeviceNumber][dwCount] != 0; dwCount++)
		{
			if (functionClassDeviceData->DevicePath[dwCount] == '#')
			{
				dwSignCount++;
			}
			if (dwSignCount == 2)
			{
				SerialNumber[DeviceNumber][dwStrIndex++] = functionClassDeviceData->DevicePath[dwCount + 1];
			}
			if (dwSignCount == 3)
			{
				SerialNumber[DeviceNumber][dwStrIndex - 1] = 0;
				break;
			}
		}
		free(functionClassDeviceData);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return DeviceNumber;
}

USB_API
void _stdcall GetSerialNumberSingle(DWORD dwDeviceIndex, LPWSTR lpSerialNumber)
{
	memcpy(lpSerialNumber, SerialNumber[dwDeviceIndex], 256);
}

BOOL OpenTeeTester(DWORD nIndex)
{
	TCHAR DeviceWritName[256];
	TCHAR DeviceReadName[256];
	hTeeTester[nIndex] = NULL;
	hTeeTesterRead[nIndex] = NULL;
	hTeeTesterWrite[nIndex] = NULL;
	hTeeTester[nIndex] = CreateFile(DevicePath[nIndex],
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	memset(DeviceWritName, 0, sizeof(DeviceWritName));
	memcpy(DeviceWritName, DevicePath[nIndex], lstrlen(DevicePath[nIndex]) * 2);
	wcscat_s(DeviceWritName, L"\\");
	wcscat_s(DeviceWritName, SILABS_BULK_WRITEPIPE);
	hTeeTesterWrite[nIndex] = CreateFile(DeviceWritName,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	memset(DeviceReadName, 0, sizeof(DeviceReadName));
	memcpy(DeviceReadName, DeviceWritName, lstrlen(DeviceWritName) * 2);
	wcscat_s(DeviceReadName, L"\\");
	wcscat_s(DeviceReadName, SILABS_BULK_READPIPE);
	hTeeTesterRead[nIndex] = CreateFile(DeviceReadName,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hTeeTester[nIndex] == NULL || hTeeTester[nIndex] == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	if (hTeeTesterRead[nIndex] == NULL || hTeeTesterRead[nIndex] == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	if (hTeeTesterWrite[nIndex] == NULL || hTeeTesterWrite[nIndex] == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	hCommunicationEvent[nIndex] = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // auto-reset event
		TRUE,              // initial state is signaled
		NULL
		);
	return TRUE;
}

void SortDeviceIndex(void)
{
	BYTE minimum = DeviceNumber;
	BYTE i, j;
	BOOL sorted[8] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };
	for (i = 0; i < DeviceNumber; i++)
	{
		for (j = 0; j < DeviceNumber; j++)
		{
			if (!sorted[j])
			{
				if (minimum == DeviceNumber)
				{
					minimum = j;
				}
				else
				{
					if (wcscmp(SerialNumber[j], SerialNumber[minimum]) < 0)
					{
						minimum = j;
					}
				}
			}
		}
		sorted[minimum] = TRUE;
		DeviceSortedIndex[i] = minimum;
		minimum = DeviceNumber;
	}
}

USB_API
BOOL _stdcall Open(DWORD dwDeviceIndex)
{
	//	if (dwDeviceIndex == -1)
	{
		for (DWORD i = 0; i < DeviceNumber; i++)
		{
			if (!OpenTeeTester(i))
			{
				return FALSE;
			}
		}
	}
	//	else
	//	{
	//		return FALSE;
	//	}
	SortDeviceIndex();
	return TRUE;
	/*	else if (dwDeviceIndex > (DeviceNumber + 1))
	{
	return FALSE;
	}
	else
	{
	OpenTeeTester(dwDeviceIndex);
	}*/
}

USB_API
void _stdcall Close()
{
	TCHAR index = 0;
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		CloseHandle(hTeeTester[index]);
		CloseHandle(hCommunicationEvent[index]);
	}
	DeviceNumber = 0;
}

BOOL TeeTesterRead(BYTE DeviceIndex, LPVOID lpBuffer, DWORD dwBytesToRead)
{
	DWORD BytesRead;
	if (ReadFile(hTeeTesterRead[DeviceIndex], lpBuffer, dwBytesToRead, &BytesRead, NULL))
	{
		if (dwBytesToRead != BytesRead)
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

BOOL TeeTesterWrite(BYTE DeviceIndex, LPVOID lpBuffer, DWORD dwBytesToWrite)
{
	DWORD BytesWritten;
	if (WriteFile(hTeeTesterWrite[DeviceIndex], lpBuffer, dwBytesToWrite, &BytesWritten, NULL))
	{
		if (dwBytesToWrite != BytesWritten)
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

unsigned int __stdcall TeeTesterCommunicate(LPVOID CommnunicateHandle)
{
	BOOL RetValue;
	struct TeeComunication *Communicate = (struct TeeComunication *)CommnunicateHandle;
	switch (WaitForSingleObject(hCommunicationEvent[Communicate->DeviceIndex], 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	};
	if (TeeTesterWrite(Communicate->DeviceIndex, Communicate->lpBufferToWrite, Communicate->dwBytesToWrite))
	{
		if (TeeTesterRead(Communicate->DeviceIndex, Communicate->lpBufferToRead, Communicate->dwBytesToRead))
		{
			RetValue = TRUE;
		}
		else
		{
			RetValue = FALSE;
		}
	}
	else
	{
		RetValue = FALSE;
	}
	SetEvent(hCommunicationEvent[Communicate->DeviceIndex]);
	return RetValue;
}

USB_API
BOOL _stdcall SetDAPower(BYTE bIndex, BYTE data)
{
	DWORD data_length = 0;
	BYTE data_send[7] = { 0 };
	BYTE data_rec[8][6] = { 0 };
	HANDLE hThread[8];
	struct TeeComunication TeeComnu[8];
	data_send[0] = 0x55;
	data_send[1] = 0x05;
	data_send[2] = 0x00;
	data_send[3] = 0x02;
	data_send[4] = bIndex;
	data_send[5] = data;
	data_send[6] = 0xAA;
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		TeeComnu[i].DeviceIndex = i;
		TeeComnu[i].lpBufferToWrite = data_send;
		TeeComnu[i].dwBytesToWrite = 7;
		TeeComnu[i].lpBufferToRead = data_rec[i];
		TeeComnu[i].dwBytesToRead = 6;
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TeeTesterCommunicate, (LPVOID)(TeeComnu + i), 0, NULL);
	}
	switch (WaitForMultipleObjects(DeviceNumber, hThread, TRUE, 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	}
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		if ((data_rec[i][0] == 0x55) && (data_rec[i][5] == 0xAA) && (data_rec[i][1] == 0x05))
		{
			data_length = ((DWORD)data_rec[i][2]) << 8;
			data_length += data_rec[i][3];
			if (data_length == 1)
			{
				if (data_rec[i][4] == 0x00)
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}

USB_API
BOOL _stdcall SetPowA(BYTE bPowA)
{
	return SetDAPower(0, bPowA);
}

BOOL SetClearButton(BYTE buttonIndex, BOOL Enable)
{
	DWORD data_length = 0;
	BYTE data_send[7] = { 0 };
	BYTE data_rec[8][6] = { 0 };
	HANDLE hThread[8];
	struct TeeComunication TeeComnu[8];

	data_send[0] = 0x55;
	data_send[1] = 0x81;
	data_send[2] = 0x00;
	data_send[3] = 0x02;
	data_send[4] = buttonIndex;
	data_send[5] = (BYTE)Enable;
	data_send[6] = 0xAA;

	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		TeeComnu[i].DeviceIndex = i;
		TeeComnu[i].lpBufferToWrite = data_send;
		TeeComnu[i].dwBytesToWrite = 7;
		TeeComnu[i].lpBufferToRead = data_rec[i];
		TeeComnu[i].dwBytesToRead = 6;
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TeeTesterCommunicate, (LPVOID)(TeeComnu + i), 0, NULL);
	}
	switch (WaitForMultipleObjects(DeviceNumber, hThread, TRUE, 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	}
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		if ((data_rec[i][0] == 0x55) && (data_rec[i][5] == 0xAA) && (data_rec[i][1] == 0x81))
		{
			data_length = ((DWORD)data_rec[i][2]) << 8;
			data_length += data_rec[i][3];
			if (data_length == 1)
			{
				if (data_rec[i][4] == 0x00)
				{
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	return TRUE;
}
USB_API
BOOL _stdcall SetButton1()
{
	Button1 = 0x00;
	return SetClearButton(0x01, TRUE);
}
USB_API
BOOL _stdcall SetButton2()
{
	Button2 = 0x00;
	return SetClearButton(0x02, TRUE);
}
USB_API
BOOL _stdcall ClearButton1Info()
{
	Button1 = 0xFF;
	return SetClearButton(0x01, FALSE);
}
USB_API
BOOL _stdcall ClearButton2Info()
{
	Button2 = 0xFF;
	return SetClearButton(0x02, FALSE);
}

BOOL GetButtonInfo(BYTE ButtonIndex, BYTE* lpbResult)
{
	DWORD data_length = 0;
	BYTE data_send[6] = { 0 };
	BYTE data_rec[8][7] = { 0 };
	HANDLE hThread[8];
	BOOL Changed = FALSE;
	struct TeeComunication TeeComnu[8];
	data_send[0] = 0x55;
	data_send[1] = 0x80;
	data_send[2] = 0x00;
	data_send[3] = 0x01;
	data_send[4] = ButtonIndex;
	data_send[5] = 0xAA;

	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		TeeComnu[i].DeviceIndex = i;
		TeeComnu[i].lpBufferToWrite = data_send;
		TeeComnu[i].dwBytesToWrite = 6;
		TeeComnu[i].lpBufferToRead = data_rec[i];
		TeeComnu[i].dwBytesToRead = 7;
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TeeTesterCommunicate, (LPVOID)(TeeComnu + i), 0, NULL);
	}
	switch (WaitForMultipleObjects(DeviceNumber, hThread, TRUE, 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	}
	for (BYTE i = 0; i < DeviceNumber; i++)
	{

		if ((data_rec[i][0] == 0x55) && (data_rec[i][6] == 0xAA) && (data_rec[i][1] == 0x80))
		{
			data_length = ((DWORD)data_rec[i][2]) << 8;
			data_length += data_rec[i][3];
			if (data_length == 2)
			{
				if (data_rec[i][5] == 0x00)
				{
					if ((data_rec[i][4] == 0x00) | (data_rec[i][4] == 0xff))
					{
						if (ButtonIndex == 1)
						{
							if (data_rec[i][4] != Button1)
							{
								Changed = TRUE;
							}
						}
						else
						{
							if (data_rec[i][4] != Button2)
							{
								Changed = TRUE;
							}
						}
					}
					else
					{
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	if (Changed)
	{
		if (ButtonIndex == 1)
		{
			Button1 = ~Button1;
			*lpbResult = Button1;
		}
		else
		{
			Button2 = ~Button2;
			*lpbResult = Button2;
		}
	}
	else
	{
		if (ButtonIndex == 1)
		{
			*lpbResult = Button1;
		}
		else
		{
			*lpbResult = Button2;
		}
	}
	return TRUE;
}
USB_API
BOOL _stdcall GetButton1Info(BYTE* lpbResult)
{
	return GetButtonInfo(0x01, lpbResult);
}
USB_API
BOOL _stdcall GetButton2Info(BYTE* lpbResult)
{
	return GetButtonInfo(0x02, lpbResult);
}

USB_API
BOOL _stdcall CollectFrame(BYTE* lpbResult)
{
	DWORD data_length = 0;
	BYTE data_send[9] = { 0 };
	BYTE data_rec[8][2294] = { 0 };
	HANDLE hThread[8];
	struct TeeComunication TeeComnu[8];

	data_send[0] = 0x55;
	data_send[1] = 0x11;
	data_send[2] = 0x00;
	data_send[3] = 0x00;
	data_send[4] = 0xAA;

	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		TeeComnu[i].DeviceIndex = DeviceSortedIndex[i];
		TeeComnu[i].lpBufferToWrite = data_send;
		TeeComnu[i].dwBytesToWrite = 5;
		TeeComnu[i].lpBufferToRead = data_rec[i];
		TeeComnu[i].dwBytesToRead = 2294;
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TeeTesterCommunicate, (LPVOID)(TeeComnu + i), 0, NULL);
	}
	switch (WaitForMultipleObjects(DeviceNumber, hThread, TRUE, 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	}
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		if ((data_rec[i][0] == 0x55) && (data_rec[i][2293] == 0xAA) && (data_rec[i][1] == 0x11))
		{
			data_length = ((DWORD)data_rec[i][2]) << 8;
			data_length += data_rec[i][3];
			if (data_length == 2289)
			{
				if (data_rec[i][2292] != 0)
				{
					return data_rec[i][2292];
				}
				memcpy(lpbResult + 2288 * i, data_rec[i] + 4, 2288);
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	return TRUE;
}

USB_API
BOOL _stdcall SenseLink(BYTE* lpbResult)
{
	DWORD data_length = 0;
	BYTE data_send[5] = { 0 };
	BYTE data_rec[8][7] = { 0 };
	HANDLE hThread[8];
	BOOL NotReady = FALSE;
	struct TeeComunication TeeComnu[8];
	data_send[0] = 0x55;
	data_send[1] = 0x82;
	data_send[2] = 0x00;
	data_send[3] = 0x00;
	data_send[4] = 0xAA;
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		TeeComnu[i].DeviceIndex = i;
		TeeComnu[i].lpBufferToWrite = data_send;
		TeeComnu[i].dwBytesToWrite = 5;
		TeeComnu[i].lpBufferToRead = data_rec[i];
		TeeComnu[i].dwBytesToRead = 7;
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TeeTesterCommunicate, (LPVOID)(TeeComnu + i), 0, NULL);
	}
	switch (WaitForMultipleObjects(DeviceNumber, hThread, TRUE, 1000))
	{
		// Event object was signaled
	case WAIT_OBJECT_0:

		break;
		// An error occurred
	default:
		return FALSE;
	}
	for (BYTE i = 0; i < DeviceNumber; i++)
	{
		if ((data_rec[i][0] == 0x55) && (data_rec[i][6] == 0xAA) && (data_rec[i][1] == 0x82))
		{
			data_length = ((DWORD)data_rec[i][2]) << 8;
			data_length += data_rec[i][3];
			if (data_length == 2)
			{
				if (data_rec[i][5] == 0x00)
				{
					if (data_rec[i][4] == 0xFF)
					{
						NotReady = TRUE;
					}
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	if (NotReady)
	{
		*lpbResult = 0xFF;
	}
	else
	{
		*lpbResult = 0;
	}
	return TRUE;
}