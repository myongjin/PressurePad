#include "pch.h"
#include "MultiTeeTester.h"

#ifdef PRESSUREPAD_EXPORTS
#define DLL_TEST_API	__declspec(dllexport)
#else
#define DLL_TEST_API	__declspec(dllimport)
#endif


extern "C"
{

	DLL_TEST_API int Test(int a, int b)
	{
		return a + b;
	}

	DLL_TEST_API int* TestArray(int a, int b)
	{
		int* array;
		array = new int[a];

		for (int i = 0; i < a; i++)
		{
			array[i] = b;
		}

		return array;
	}

	DLL_TEST_API void TestArrayV2(int* IntArray)
	{
		for (int i = 0; i < 10; i++)
		{
			IntArray[i] = 5;
		}
	}

	DLL_TEST_API void TestArrayV3(int* IntArray)
	{
		BYTE test[10];

		for (int i = 0; i < 10; i++)
		{
			test[i] = 5;
			IntArray[i] = test[i] << 24;;
		
		}
	}

	DLL_TEST_API void TestArrayV4(int* IntArray)
	{
		BYTE test[10];

		for (int i = 0; i < 10; i++)
		{
			test[i] = 5;
			IntArray[i] = static_cast<int>(test[i]);

		}
	}

	DLL_TEST_API bool InitDevice()
	{
		DWORD devNum = GetDevicesNum();
		if (devNum>0)
		{
			if (Open(devNum)) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	DLL_TEST_API void SetSensitivity(BYTE value)
	{
		SetPowA(value);
	}

	DLL_TEST_API bool GetPressureArray(int* array)
	{
		BYTE myByte[2288];
		bool getData = CollectFrame(myByte);

		if (getData)
		{
			for (int i = 0; i < 2288; i++)
			{
				array[i] = static_cast<int>(myByte[i]);
			}
		}
		
		return getData;
	}

	
	DLL_TEST_API void CloseDevice()
	{
		Close();
	}
}