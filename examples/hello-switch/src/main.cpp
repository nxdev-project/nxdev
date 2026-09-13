#include <switch.h>
#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize default console on Nintendo Switch screen
    consoleInit(NULL);

    // Configure standard Nintendo Switch controller input
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[14;20H=== NXDev Switch Homebrew ===");
    printf("\x1b[16;22HHello, Nintendo Switch!");
    printf("\x1b[18;18HPowered by devkitA64 & libnx");
    printf("\x1b[22;19HPress + (Plus) to exit.");

    // Main application event loop
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
