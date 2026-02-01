#include "app.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    FreeConsole();
    MainWindow app;
    if (!app.Create()) return 1;
    return app.Run();
}
