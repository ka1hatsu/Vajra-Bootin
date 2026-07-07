#include <windows.h>

#include "app/Application.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    vajra::Application application;
    return application.run(instance, show_command);
}
