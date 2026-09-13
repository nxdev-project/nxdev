#include <switch.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize the default Switch text console
    consoleInit(NULL);

    // Configure the default gamepad input state
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    printf("\x1b[2;2H=== %s ===", "{{PROJECT_NAME}}");
    printf("\x1b[4;2HBuilt with NXDev (Raw C / libnx)");
    printf("\x1b[6;2HPress PLUS (+) to exit.");

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            break;
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
