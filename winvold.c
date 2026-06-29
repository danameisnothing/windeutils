#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    if (!RegisterHotKey(NULL, 1, MOD_WIN, VK_F2)
        || !RegisterHotKey(NULL, 2, MOD_WIN, VK_F3)) {
        fprintf(stderr, "RegisterHotKey failed with \"%lu\"\n", GetLastError());
        return EXIT_FAILURE;
    }

    MSG msg = {0};
    while (GetMessageA(&msg, NULL, 0, 0) != 0) {
        if (msg.message != WM_HOTKEY) continue;

        // https://batchloaf.wordpress.com/2012/10/18/simulating-a-ctrl-v-keystroke-in-win32-c-or-c-using-sendinput/
        INPUT inputs[2] = {0};

        inputs[0].type = INPUT_KEYBOARD;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        WORD inp = 0;

        switch (msg.wParam) {
            case 1: {
                inp = VK_VOLUME_DOWN;
                break;
            }
            case 2: {
                inp = VK_VOLUME_UP;
                break;
            }
        }

        inputs[0].ki.wVk = inp;
        inputs[1].ki.wVk = inp;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    }

    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);

    return EXIT_SUCCESS;
}