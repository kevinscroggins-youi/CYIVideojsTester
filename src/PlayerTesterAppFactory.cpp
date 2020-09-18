
#include "AppFactory.h"
#include "PlayerTesterApp.h"

#define APP_NAME "PlayerTester"

#if defined(YI_PS4)
#    define APP_WIDTH (1920)
#    define APP_HEIGHT (1080)
#else
#    define APP_WIDTH (640)
#    define APP_HEIGHT (480)
#endif

std::unique_ptr<CYIApp> AppFactory::Create()
{
    return std::make_unique<PlayerTesterApp>();
}

int AppFactory::GetWindowWidth()
{
    return APP_WIDTH;
}

int AppFactory::GetWindowHeight()
{
    return APP_HEIGHT;
}

const char *AppFactory::GetWindowName()
{
    return APP_NAME;
}
