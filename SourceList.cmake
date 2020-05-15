set(SOURCE_IOS
    src/AirplayRoutePicker.mm
)

set(HEADERS_IOS
    src/AirplayRoutePicker.h
)

set(SOURCE_TIZEN-NACL
    src/YiTizenNaClRemoteLoggerSink.cpp
    src/YiVideojsVideoPlayer.cpp
    src/YiVideojsVideoSurface.cpp
)

set(HEADERS_TIZEN-NACL
    src/YiTizenNaClRemoteLoggerSink.h
    src/YiVideojsVideoPlayer.h
    src/YiVideojsVideoPlayerPriv.h
    src/YiVideojsVideoSurface.h
)

set (YI_PROJECT_SOURCE
    src/AutomatedPlayerTesterApp.cpp
    src/IStreamPlanetFairPlayHandler.cpp
    src/PlayerTesterApp.cpp
    src/PlayerTesterAppFactory.cpp
    ${SOURCE_${YI_PLATFORM_UPPER}}
)

set (YI_PROJECT_HEADERS
    src/AutomatedPlayerTesterApp.h
    src/IStreamPlanetFairPlayHandler.h
    src/PlayerTesterApp.h
    ${HEADERS_${YI_PLATFORM_UPPER}}
)
