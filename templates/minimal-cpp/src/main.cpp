#include <switch.h>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    consoleInit(nullptr);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    std::cout << "\x1b[2;2H=== " << "{{PROJECT_NAME}}" << " ===\n";
    std::cout << "\x1b[4;2HBuilt with NXDev (Modern C++20 / libnx)\n";
    std::cout << "\x1b[6;2HPress PLUS (+) to exit.\n";

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            break;
        }

        consoleUpdate(nullptr);
    }

    consoleExit(nullptr);
    return 0;
}
