// HapticsNotifications.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Haptics.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Haptics;

static SimpleHapticsController controller = NULL;
static SimpleHapticsControllerFeedback controllerFeedback = NULL;

NTSTATUS NTAPI WnfCallback(uint64_t, void*, void*, void*, void*, void*);

extern "C" {
	NTSTATUS NTAPI RtlSubscribeWnfStateChangeNotification(void*, uint64_t, uint32_t, decltype(WnfCallback), size_t, size_t, size_t, size_t);
	NTSTATUS NTAPI RtlUnsubscribeWnfStateChangeNotification(decltype(WnfCallback));
}

constexpr uint64_t WNF_SHEL_TOAST_PUBLISHED = 0xD83063EA3BD0035;

bool GetState()
{
	HKEY key;
	DWORD hr;
	bool enabled = TRUE;

	hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OEM\\Vibra", NULL, KEY_READ, &key);
	if (hr == ERROR_SUCCESS)
	{
		bool state = TRUE;
		unsigned long type = REG_DWORD, size = 8;
		hr = RegQueryValueEx(key, L"Enabled", NULL, &type, (LPBYTE)& state, &size);

		if (hr != ERROR_SUCCESS)
		{
			goto CLEANUP;
		}

		if (type == REG_DWORD)
		{
			enabled = state;
		}
	CLEANUP:
		RegCloseKey(key);
	}

	return enabled;
}

long GetTime()
{
	HKEY key;
	DWORD hr;
	unsigned long time = 5000000L;

	hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OEM\\Vibra", NULL, KEY_READ, &key);
	if (hr == ERROR_SUCCESS)
	{
		unsigned long tms = 5;
		unsigned long type = REG_DWORD, size = 8;
		hr = RegQueryValueEx(key, L"Duration", NULL, &type, (LPBYTE)& tms, &size);

		if (hr != ERROR_SUCCESS)
		{
			goto CLEANUP;
		}

		if (type == REG_DWORD)
		{
			if (tms > 10)
				tms = 10;
			else if (tms < 1)
				tms = 1;

			time = tms * 1000000L;
		}
	CLEANUP:
		RegCloseKey(key);
	}

	return time;
}

int GetIntensity()
{
	HKEY key;
	DWORD hr;
	unsigned int intensity = 100;

	hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OEM\\Vibra", NULL, KEY_READ, &key);
	if (hr == ERROR_SUCCESS)
	{
		unsigned int ints = 100;
		unsigned long type = REG_DWORD, size = 8;
		hr = RegQueryValueEx(key, L"Intensity", NULL, &type, (LPBYTE)& ints, &size);

		if (hr != ERROR_SUCCESS)
		{
			goto CLEANUP;
		}

		if (type == REG_DWORD)
		{
			if (ints > 100)
				ints = 100;
			else if (ints < 20)
				ints = 20;

			intensity = ints;
		}
	CLEANUP:
		RegCloseKey(key);
	}

	return intensity;
}

NTSTATUS NTAPI WnfCallback(uint64_t p1, void* p2, void* p3, void* p4, void* p5, void* p6)
{
	if (GetState())
	{
		unsigned int intensity = GetIntensity();
		unsigned long time = GetTime();

		if (controller != NULL)
		{
			TimeSpan span(time);
			controller.SendHapticFeedbackForDuration(controllerFeedback, ((double)intensity / (double)100), span);
		}
	}
	return 0;
}

int main()
{
	// Inits WinRT
	init_apartment();

	// Aquires access to haptics hardware
	VibrationAccessStatus auth{ VibrationDevice::RequestAccessAsync().get() };

	if (auth == VibrationAccessStatus::Allowed)
	{
		VibrationDevice device = VibrationDevice::GetDefaultAsync().get();
		if (device != NULL)
		{
			controller = device.SimpleHapticsController();
			controllerFeedback = controller.SupportedFeedback().GetAt(0);

			uint32_t buf1{};
			size_t buf2{};
			NTSTATUS result = RtlSubscribeWnfStateChangeNotification(&buf2, WNF_SHEL_TOAST_PUBLISHED, buf1, WnfCallback, 0, 0, 0, 1);

			if (result == ERROR_SUCCESS)
			{
				// Signal subscribing succeeded for debugging purposes
				//TimeSpan span(10000000L);
				//controller.SendHapticFeedbackForDuration(controllerFeedback, 1, span);
			}

			// Wait indefinetly
			while (true)
			{
				Sleep(3000);
			}

			RtlUnsubscribeWnfStateChangeNotification(WnfCallback);
		}
	}

	return 0;
}
