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
#define WD_BRGHT_STEP 2

int main(void) {
    if (!RegisterHotKey(NULL, WD_VOL_UP, MOD_WIN, VK_F3)
        || !RegisterHotKey(NULL, WD_VOL_DOWN, MOD_WIN, VK_F2)
        || !RegisterHotKey(NULL, WD_BRGHT_UP, MOD_WIN, VK_F10)
        || !RegisterHotKey(NULL, WD_BRGHT_DOWN, MOD_WIN, VK_F9)) {
        fprintf(stderr, "RegisterHotKey failed with \"%lu\"\n", GetLastError());
        return EXIT_FAILURE;
    }

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
            GetDeviceGammaRamp(dc, ramp);
            // sample one for brightness
            unsigned int brght = ramp[0][5] / 5 - 128;

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
            if (brght < 0) brght = 0;
            if (brght > 128) brght = 128;

            for (unsigned int i = 0; i < 3; i++) {
                for (unsigned int j = 0; j < 256; j++) {
                    ramp[i][j] = j * (brght + 128);
                }
            }

            SetDeviceGammaRamp(dc, ramp);
            ReleaseDC(NULL, dc);
        }
    }

    UnregisterHotKey(NULL, WD_VOL_UP);
    UnregisterHotKey(NULL, WD_VOL_DOWN);
    UnregisterHotKey(NULL, WD_BRGHT_UP);
    UnregisterHotKey(NULL, WD_BRGHT_DOWN);

    return EXIT_SUCCESS;
}