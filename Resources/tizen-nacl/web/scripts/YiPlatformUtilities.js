/**
 * @file YiPlatformUtilities.js
 * @brief JavaScript platform utility helper code.
 */

"use strict";

function CYIPlatformUtilities() { }

var drmKeySystems = {
    widevine: "com.widevine.alpha",
    playready: "com.microsoft.playready"
};

var drmKeySystemConfigurations = [
    {
        videoCapabilities: [
            {
                contentType: "video/mp4; codecs=\"avc1.42E01E\""
            }
        ]
    }
];

Object.defineProperty(CYIPlatformUtilities, "platformName", {
    value: "Unknown",
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "isEmbedded", {
    value: false,
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "isTizen", {
    value: typeof tizen !== "undefined" && typeof webapis !== "undefined",
    enumerable: true
});

Object.defineProperty(CYIPlatformUtilities, "isUWP", {
    value: window.external !== undefined && window.external.notify !== undefined,
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "isPS4", {
    value: false,
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "isOSX", {
    value: false,
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "tizenPlatformVersion", {
    value: null,
    enumerable: true,
    writable: true
});

Object.defineProperty(CYIPlatformUtilities, "drmSupport", {
    value: {
        playready: false,
        widevine: false
    },
    enumerable: true
});

if(CYIPlatformUtilities.isTizen && CYIUtilities.isInvalid(CYIPlatformUtilities.tizenPlatformVersion)) {
    CYIPlatformUtilities.platformName = "Tizen";

    try {
        CYIPlatformUtilities.tizenPlatformVersion = tizen.systeminfo.getCapability("http://tizen.org/feature/platform.native.api.version");
    }
    catch(error) {
        console.error("Failed to determine Tizen platform version: " + error.message);
    }
}
else if(CYIPlatformUtilities.isUWP) {
    CYIPlatformUtilities.platformName = "UWP";
}

CYIPlatformUtilities.isDRMTypeSupported = function isDRMTypeSupported(drmType) {
    drmType = CYIUtilities.trimString(drmType);

    if(CYIUtilities.isEmptyString(drmType)) {
        return false;
    }

    drmType = drmType.toLowerCase();

    return !!CYIPlatformUtilities.drmSupport[drmType];
};

CYIPlatformUtilities.checkDRMSupport = function checkDRMSupport() {
    var drmTypes = Object.keys(drmKeySystems);

    for(var i = 0; i < drmTypes.length; i++) {
        var drmType = drmTypes[i];
        var drmKeySystem = drmKeySystems[drmType];

        if(!CYIUtilities.isFunction(navigator.requestMediaKeySystemAccess)) {
            return;
        }

        (function(drmType, drmKeySystem) {
            navigator.requestMediaKeySystemAccess(drmKeySystem, drmKeySystemConfigurations).then(function(access) {
                if(!CYIUtilities.isFunction(access.createMediaKeys)) {
                    return;
                }

                access.createMediaKeys().then(function(keys) {
                    CYIPlatformUtilities.drmSupport[drmType] = true;
                }).catch(function(error) {
                    console.log(CYIPlatformUtilities.platformName + " does not support creating media keys for " + drmType + " DRM.");
                });
            }).catch(function(error) {
                console.log(CYIPlatformUtilities.platformName + " does not support " + drmType + " DRM.");
            });
        })(drmType, drmKeySystem);
    }
};

CYIPlatformUtilities.checkDRMSupport();
