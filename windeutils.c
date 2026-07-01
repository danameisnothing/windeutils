// winvold and winbrghtd combined onto one process

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <stdlib.h>
#include <stdio.h>

#define WD_VOL_UP 0x11082022
#define WD_VOL_DOWN 0x15022024
#define WD_BRGHT_UP 0xD00D // TODO: working value
#define WD_BRGHT_DOWN 0xDEAF // TODO: working value
#define WD_BRGHT_STEP 4
#define WD_BRGHT_CAP_FULL_LOW 20 // low cap on the brightness value, when GdiIcmGammaRange is set. values lower are too dark

int main(void) {
    if (!RegisterHotKey(NULL, WD_VOL_UP, MOD_WIN, VK_F3)
        || !RegisterHotKey(NULL, WD_VOL_DOWN, MOD_WIN, VK_F2)
        || !RegisterHotKey(NULL, WD_BRGHT_UP, MOD_WIN, VK_F10)
        || !RegisterHotKey(NULL, WD_BRGHT_DOWN, MOD_WIN, VK_F9)) {
        fprintf(stderr, "RegisterHotKey failed with \"%lu\"\n", GetLastError());
        return EXIT_FAILURE;
    }

    // wow : https://stackoverflow.com/questions/54383927/why-is-setdevicegammaramp-failing-when-adjusting-values-below-50
    // https://gist.github.com/caiorss/0a994cb739994ade349d50de8db12d11
    HKEY key = NULL;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM", 0, KEY_READ, &key) != ERROR_SUCCESS) {
        MessageBoxA(NULL, "Failed to open ICM registry key", "Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    LSTATUS regqwr = RegQueryValueExA(key, "GdiIcmGammaRange", NULL, NULL, NULL, NULL);
    if (regqwr != ERROR_SUCCESS && regqwr != ERROR_FILE_NOT_FOUND) {
        MessageBoxA(NULL, "Failed to query ICM registry value", "Error", MB_OK | MB_ICONERROR);
        RegCloseKey(key);
        return EXIT_FAILURE;
    }
    RegCloseKey(key);

    // will be true if the user has this registry key set
    int has_unlocked_range = (regqwr != ERROR_FILE_NOT_FOUND);

    if (!has_unlocked_range) {
        MessageBoxA(NULL, "GdiIcmGammaRange is not set, brightness may be capped to 50% and higher.", "Warning", MB_OK | MB_ICONWARNING);
    }

    printf("windeutils daemon started\n");

    MSG msg = {0};
    while (GetMessageA(&msg, NULL, 0, 0) != 0) {
        if (msg.message != WM_HOTKEY) continue;

        if (msg.wParam == WD_VOL_UP || msg.wParam == WD_VOL_DOWN) {
            // https://batchloaf.wordpress.com/2012/10/18/simulating-a-ctrl-v-keystroke-in-win32-c-or-c-using-sendinput/
            INPUT inputs[2] = {0};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

            WORD inp = 0;
            switch (msg.wParam) {
                case WD_VOL_DOWN: {
                    inp = VK_VOLUME_DOWN;
                    break;
                }
                case WD_VOL_UP: {
                    inp = VK_VOLUME_UP;
                    break;
                }
            }

            inputs[0].ki.wVk = inp;
            inputs[1].ki.wVk = inp;

            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        } else if (msg.wParam == WD_BRGHT_UP || msg.wParam == WD_BRGHT_DOWN) {
            // https://github.com/KrasnovM/Gamma-Manager/blob/master/Gamma%20Manager/Gamma.cs
            // https://stackoverflow.com/a/33268698

            // heavily based off of upon https://www.nirsoft.net/vc/change_screen_brightness.html
            // https://stackoverflow.com/a/7219673
            HDC dc = GetDC(NULL);
            WORD ramp[3][256];
            if (!GetDeviceGammaRamp(dc, ramp)) {
                MessageBoxA(NULL, "GetDeviceGammaRamp() failed. That's all we know.", "Error", MB_OK | MB_ICONERROR);
                return EXIT_FAILURE;
            }

            int bottom = (has_unlocked_range) ? 0 : 128;
            int upper = (has_unlocked_range) ? 256 : 128;

            // sample one for brightness
            int brght = ramp[0][5] / 5 - bottom;

            switch (msg.wParam) {
                case WD_BRGHT_UP: {
                    brght += WD_BRGHT_STEP;
                    break;
                }
                case WD_BRGHT_DOWN: {
                    brght -= WD_BRGHT_STEP;
                    break;
                }
            }

            // rude clamp
            if (brght < (has_unlocked_range ? WD_BRGHT_CAP_FULL_LOW : 0)) brght = (has_unlocked_range ? WD_BRGHT_CAP_FULL_LOW : 0);
            if (brght > upper) brght = upper;

            for (unsigned int i = 0; i < 3; i++) {
                for (unsigned int j = 0; j < 256; j++) {
                    ramp[i][j] = j * (brght + bottom);
                }
            }

            if (!SetDeviceGammaRamp(dc, ramp)) {
                MessageBoxA(NULL, "SetDeviceGammaRamp() failed. That's all we know.", "Error", MB_OK | MB_ICONERROR);
                return EXIT_FAILURE;
            }
            ReleaseDC(NULL, dc);
        }
    }

    // we don't care anymore if this fails, it's impossible for the program to reach here anyway
    UnregisterHotKey(NULL, WD_VOL_UP);
    UnregisterHotKey(NULL, WD_VOL_DOWN);
    UnregisterHotKey(NULL, WD_BRGHT_UP);
    UnregisterHotKey(NULL, WD_BRGHT_DOWN);

    return EXIT_SUCCESS;
}