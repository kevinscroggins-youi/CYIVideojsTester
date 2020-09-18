/**
 * @file YiVideojsVideoPlayer.js
 * @brief JavaScript wrapper class for Video.js video player instances.
 */

"use strict";

class CYIVideojsVideoPlayerVersion {
    constructor(version) {
        const self = this;

        const _properties = { };

        Object.defineProperty(self, "major", {
            enumerable: true,
            get() {
                return _properties.major;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(isNaN(newValue) || newValue < 0) {
                    throw CYIUtilities.createError("Invalid major version value: \"" + CYIUtilities.toString(value) + "\".");
                }

                _properties.major = newValue;
            }
        });

        Object.defineProperty(self, "minor", {
            enumerable: true,
            get() {
                return _properties.minor;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(isNaN(newValue) || newValue < 0) {
                    throw CYIUtilities.createError("Invalid minor version value: \"" + CYIUtilities.toString(value) + "\".");
                }

                _properties.minor = newValue;
            }
        });

        Object.defineProperty(self, "patch", {
            enumerable: true,
            get() {
                return _properties.patch;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(isNaN(newValue) || newValue < 0) {
                    throw CYIUtilities.createError("Invalid patch version value: \"" + CYIUtilities.toString(value) + "\".");
                }

                _properties.patch = newValue;
            }
        });

        Object.defineProperty(self, "version", {
            enumerable: true,
            configurable: true,
            get() {
                return _properties.major + "." + _properties.minor + "." + _properties.patch;
            },
            set(value) {
                const newValue = CYIUtilities.trimString(value);

                if(CYIUtilities.isEmptyString(newValue)) {
                    throw CYIUtilities.createError("Invalid version value: \"" + CYIUtilities.toString(value) + "\".");
                }

                const rawVersionData = newValue.match(CYIVideojsVideoPlayerVersion.PlayerVersionRegex);

                if(CYIUtilities.isInvalid(rawVersionData)) {
                    throw CYIUtilities.createError("Invalid version: \"" + newValue + "\".");
                }

                _properties.major = rawVersionData[2];
                _properties.minor = rawVersionData[3];
                _properties.patch = rawVersionData[4];
            }
        });

        Object.defineProperty(self, "full", {
            enumerable: true,
            configurable: true,
            get() {
                return self.version;
            }
        });

        self.version = version;
    }

    static isVideoPlayerVersion(value) {
        return value instanceof CYIVideojsVideoPlayerVersion;
    }
}

class CYIVideojsVideoPlayerState {
    constructor(id, attributeName, displayName) {
        const self = this;

        if(CYIUtilities.isObject(id)) {
            const data = id;
            id = data.id;
            attributeName = data.attributeName;
            displayName = data.displayName;
        }

        const _properties = {
            transitionStates: []
        };

        Object.defineProperty(self, "id", {
            enumerable: true,
            get() {
                return _properties.id;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(isNaN(newValue)) {
                    throw CYIUtilities.createError("Invalid id value, expected integer.");
                }

                _properties.id = newValue;
            }
        });

        Object.defineProperty(self, "attributeName", {
            enumerable: true,
            get() {
                return _properties.attributeName;
            },
            set(value) {
                const newValue = CYIUtilities.trimString(value);

                if(CYIUtilities.isEmptyString(newValue)) {
                    throw CYIUtilities.createError("Invalid attribute name, expected non-empty string.");
                }

                _properties.attributeName = newValue;
            }
        });

        Object.defineProperty(self, "displayName", {
            enumerable: true,
            get() {
                return _properties.displayName;
            },
            set(value) {
                const newValue = CYIUtilities.trimString(value);

                if(CYIUtilities.isEmptyString(newValue)) {
                    throw CYIUtilities.createError("Invalid display name, expected non-empty string.");
                }

                _properties.displayName = newValue;
            }
        });

        Object.defineProperty(self, "transitionStates", {
            enumerable: true,
            get() {
                return _properties.transitionStates;
            },
            set(value) {
                if(!Array.isArray(value)) {
                    throw CYIUtilities.createError("Invalid transition states collection!");
                }

                for(let i = 0; i < value.length; i++) {
                    const state = value[i];

                    if(!CYIVideojsVideoPlayerState.isVideojsVideoPlayerState(state) || state.id < 0) {
                        throw CYIUtilities.createError("Invalid transition state at index " + i + "!");
                    }

                    if(self.id === state.id) {
                        throw CYIUtilities.createError("Transition state at index " + i + " must have a different id from the current state.");
                    }

                    for(let j = i + 1; j < value.length; j++) {
                        if(value[i].id === value[j].id) {
                            throw CYIUtilities.createError("Duplicate transition state with id: " + value[i].id + " found at index " + j + ".");
                        }
                    }
                }

                _properties.transitionStates.length = 0;
                Array.prototype.push.apply(_properties.transitionStates, value);
            }
        });

        self.id = id;
        self.attributeName = attributeName;

        if(CYIUtilities.isEmptyString(displayName)) {
            self.displayName = self.attributeName;
        }
        else {
            self.displayName = displayName;
        }
    }

    numberOfTransitionStates() {
        const self = this;

        return self.transitionStates.length;
    }

    hasTransitionState(state) {
        const self = this;

        return self.indexOfTransitionState(state) !== -1;
    }

    indexOfTransitionState(state) {
        const self = this;

        if(!CYIVideojsVideoPlayerState.isVideojsVideoPlayerState(state)) {
            return -1;
        }

        for(let i = 0; i < self.transitionStates.length; i++) {
            if(self.transitionStates[i].id === state.id) {
                return i;
            }
        }

        return -1;
    }

    indexOfTransitionStateWithId(id) {
        const self = this;

        id = CYIUtilities.parseInteger(id);

        if(isNaN(id)) {
            return -1;
        }

        for(let i = 0; i < self.transitionStates.length; i++) {
            if(self.transitionStates[i].id === id) {
                return i;
            }
        }

        return -1;
    }

    canTransitionTo(state) {
        const self = this;

        if(!CYIVideojsVideoPlayerState.isVideojsVideoPlayerState(state) || !state.isValid() || self.id === state.id) {
            return false;
        }

        return self.hasTransitionState(state);
    }

    getTransitionStateWithId(id) {
        const self = this;

        const transitionStateIndex = self.indexOfTransitionStateWithId(id);

        if(transitionStateIndex === -1) {
            return null;
        }

        return self.transitionStates[transitionStateIndex];
    }

    getTransitionStateAtIndex(index) {
        const self = this;

        index = CYIUtilities.parseInteger(index);

        if(isNaN(index) || index < 0 || index >= self.transitionStates.length) {
            return null;
        }

        return self.transitionStates[index];
    }

    addTransitionState(state) {
        const self = this;

        if(!CYIVideojsVideoPlayerState.isValid(state) || self.id === state.id || self.hasTransitionState(state)) {
            return false;
        }

        self.transitionStates.push(state);

        return true;
    }

    removeTransitionState(state) {
        const self = this;

        const transitionStateIndex = self.indexOfTransitionState(state);

        if(transitionStateIndex === -1) {
            return false;
        }

        self.transitionStates.splice(transitionStateIndex, 1);

        return true;
    }

    removeTransitionStateWithId(id) {
        const self = this;

        const transitionStateIndex = self.indexOfTransitionStateWithId(id);

        if(transitionStateIndex === -1) {
            return false;
        }

        self.transitionStates.splice(transitionStateIndex, 1);

        return true;
    }

    removeTransitionStateAtIndex(index) {
        const self = this;

        index = CYIUtilities.parseInteger(index);

        if(isNaN(index) || index < 0 || index >= self.transitionStates.length) {
            return false;
        }

        self.transitionStates.splice(index, 1);

        return true
    }

    clearTransitionStates() {
        const self = this;

        self.transitionStates.length = 0;
    }

    equals(value) {
        const self = this;

        if(!CYIVideojsVideoPlayerState.isVideojsVideoPlayerState(value)) {
            return false;
        }

        return self.id === value.id;
    }

    toString() {
        const self = this;

        return self.displayName + " Video Player State";
    }

    static isVideojsVideoPlayerState(value) {
        return value instanceof CYIVideojsVideoPlayerState;
    }

    isValid() {
        const self = this;

        return self.id >= 0;
    }

    static isValid(value) {
        return (CYIVideojsVideoPlayerState.isVideojsVideoPlayerState(value)) &&
               value.isValid();
    }
}

class CYIVideojsVideoPlayerStreamFormat {
    constructor(streamFormat, drmTypes) {
        const self = this;

        const _properties = {
            format: null,
            drmTypes: []
        }

        Object.defineProperty(self, "format", {
            enumerable: true,
            get() {
                return _properties.format;
            },
            set(value) {
                _properties.format = CYIUtilities.trimString(value);
            }
        });

        Object.defineProperty(self, "drmTypes", {
            enumerable: true,
            get() {
                return _properties.drmTypes;
            },
            set(value) {
                _properties.drmTypes.length = 0;

                if(CYIUtilities.isNonEmptyArray(value)) {
                    for(let i = 0; i < value.length; i++) {
                        const formattedDRMType = CYIUtilities.trimString(value[i]);

                        if(CYIUtilities.isEmptyString(formattedDRMType)) {
                            continue;
                        }

                        for(let i = 0; i < _properties.drmTypes.length; i++) {
                            if(CYIUtilities.equalsIgnoreCase(_properties.drmTypes[i], formattedDRMType)) {
                                continue;
                            }
                        }

                        if(!CYIPlatformUtilities.isDRMTypeSupported(formattedDRMType)) {
                            continue;
                        }

                        _properties.drmTypes.push(formattedDRMType);
                    }
                }
            }
        });

        self.format = streamFormat;
        self.drmTypes = drmTypes;
    }

    numberOfDRMTypes() {
        const self = this;

        return self.drmTypes.length;
    }

    hasDRMType(drmType) {
        const self = this;

        return self.indexOfDRMType(drmType) !== -1;
    }

    indexOfDRMType(drmType) {
        const self = this;

        const formattedDRMType = CYIUtilities.trimString(drmType);

        if(CYIUtilities.isEmptyString(formattedDRMType)) {
            return -1;
        }

        for(let i = 0; i < self.drmTypes.length; i++) {
            if(CYIUtilities.equalsIgnoreCase(self.drmTypes[i], formattedDRMType)) {
                return i;
            }
        }

        return -1;
    }

    addDRMType(drmType) {
        const self = this;

        if(self.hasDRMType(drmType)) {
            return false;
        }

        const formattedDRMType = CYIUtilities.trimString(drmType);

        if(CYIUtilities.isEmptyString(formattedDRMType)) {
            return false;
        }

        if(!CYIPlatformUtilities.isDRMTypeSupported(formattedDRMType)) {
            return false;
        }

        self.drmTypes.push(formattedDRMType);

        return true;
    }

    addDRMTypes(drmTypes) {
        const self = this;

        if(CYIUtilities.isNonEmptyArray(drmTypes)) {
            for(let i = 0; i < drmTypes.length; i++) {
                self.addDRMType(drmTypes[i]);
            }
        }
    }

    removeDRMType(drmType) {
        const self = this;

        const drmTypeIndex = self.indexOfDRMType(drmType);

        if(drmTypeIndex === -1) {
            return false;
        }

        self.drmTypes.splice(drmTypeIndex, 1);

        return true;
    }

    clearDRMTypes() {
        const self = this;

        self.drmTypes.length = 0;
    }

    equals(value) {
        const self = this;

        if(!self.isValid() || !CYIVideojsVideoPlayerStreamFormat.isValid(value)) {
            return false;
        }

        return CYIUtilities.equalsIgnoreCase(self.format, value.format);
    }

    toString() {
        const self = this;

        return (CYIUtilities.isNonEmptyString(self.format) ? self.format : "Invalid") + (CYIUtilities.isNonEmptyArray(self.drmTypes) ? " (" + self.drmTypes.join(", ") + ")" : "");
    }

    static isStreamFormat(value) {
        return value instanceof CYIVideojsVideoPlayerStreamFormat;
    }

    isValid() {
        const self = this;

        return Array.isArray(self.drmTypes);
    }

    static isValid(value) {
        return CYIVideojsVideoPlayerStreamFormat.isStreamFormat(value) &&
               value.isValid();
    }
}

class CYIRectangle {
    constructor(x, y, width, height) {
        const self = this;

        const _properties = { };

        if(CYIUtilities.isObject(x)) {
            const data = x;
            x = data.x;
            y = data.y;
            width = data.width;
            height = data.height;
        }

        Object.defineProperty(self, "x", {
            enumerable: true,
            get() {
                return _properties.x;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(!Number.isInteger(newValue)) {
                    throw CYIUtilities.createError("Invalid rectangle x value: " + CYIUtilities.toString(value));
                }

                _properties.x = newValue;
            }
        });

        Object.defineProperty(self, "y", {
            enumerable: true,
            get() {
                return _properties.y;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(!Number.isInteger(newValue)) {
                    throw CYIUtilities.createError("Invalid rectangle y value: " + CYIUtilities.toString(value));
                }

                _properties.y = newValue;
            }
        });

        Object.defineProperty(self, "width", {
            enumerable: true,
            get() {
                return _properties.width;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(!Number.isInteger(newValue) || newValue < 0) {
                    throw CYIUtilities.createError("Invalid rectangle width: " + CYIUtilities.toString(value));
                }

                _properties.width = newValue;
            }
        });

        Object.defineProperty(self, "height", {
            enumerable: true,
            get() {
                return _properties.height;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(!Number.isInteger(newValue) || newValue < 0) {
                    throw CYIUtilities.createError("Invalid rectangle height: " + CYIUtilities.toString(value));
                }

                _properties.height = newValue;
            }
        });

        self.x = x;
        self.y = y;
        self.width = width;
        self.height = height;
    }

    toArray() {
        const self = this;

        return [self.x, self.y, self.width, self.height];
    }

    equals(value) {
        const self = this;

        if(!CYIRectangle.isRectangle(value)) {
            return false;
        }

        return self.x === value.x &&
               self.y === value.y &&
               self.width === value.width &&
               self.height === value.height;
    }

    toString() {
        const self = this;

        return "X: " + self.x + " Y: " + self.y + " W: " + self.width + " H: " + self.height;
    }

    static isRectangle(value) {
        return value instanceof CYIRectangle;
    }
}

class CYIVideojsVideoPlayerProperties {
    constructor() {
        const self = this;

        const _properties = {
            instance: null,
            script: null,
            dependenciesLoaded: false,
            playerVersion: null
        };

        Object.defineProperty(self, "instance", {
            enumerable: true,
            get() {
                return _properties.instance;
            },
            set(value) {
                if(CYIUtilities.isInvalid(value)) {
                    _properties.instance = null;

                    return;
                }

                if(!(value instanceof CYIVideojsVideoPlayer)) {
                    return;
                }

                _properties.instance = value;
            }
        });

        Object.defineProperty(self, "script", {
            enumerable: true,
            get() {
                return _properties.script;
            },
            set(value) {
                if(!(value instanceof HTMLScriptElement)) {
                    throw CYIUtilities.createError("Invalid HTML script element!");
                }

                _properties.script = value;
            }
        });

        Object.defineProperty(self, "dependenciesLoaded", {
            enumerable: true,
            get() {
                return _properties.dependenciesLoaded;
            },
            set(value) {
                const newValue = CYIUtilities.parseBoolean(value);

                if(newValue !== null) {
                    _properties.dependenciesLoaded = newValue;
                }
            }
        });

        Object.defineProperty(self, "playerVersion", {
            enumerable: true,
            get() {
                return _properties.playerVersion;
            },
            set(value) {
                if(typeof value === "string") {
                    _properties.playerVersion = new CYIVideojsVideoPlayerVersion(value);
                }
                else if(CYIVideojsVideoPlayerVersion.isVideoPlayerVersion(value)) {
                    _properties.playerVersion = value;
                }
                else {
                    throw CYIUtilities.createError("Invalid player version data!");
                }
            }
        });
    }
}

class CYIVideojsVideoPlayer {
    constructor(configuration) {
        const self = this;

        if(!CYIVideojsVideoPlayer.dependenciesLoaded) {
            throw CYIUtilities.createError(CYIVideojsVideoPlayer.getType() + " dependencies not loaded yet!");
        }

        // store private player properties internally and only expose them through getters and setters
        const _properties = {
            streamFormats: [],
            externalTextTrackIdCounter: 1,
            externalTextTrackQueue: []
        };

        Object.defineProperty(self, "state", {
            enumerable: true,
            get() {
                return _properties.state;
            },
            set(value) {
                if(!CYIVideojsVideoPlayerState.isValid(value)) {
                    throw CYIUtilities.createError("Invalid video player state!");
                }

                _properties.state = value;
            }
        });

        // create getters and setters for all player instance properties
        Object.defineProperty(self, "streamFormats", {
            enumerable: true,
            get() {
                return _properties.streamFormats;
            }
        });

        Object.defineProperty(self, "initialized", {
            enumerable: true,
            get() {
                return _properties.initialized;
            },
            set(value) {
                _properties.initialized = CYIUtilities.parseBoolean(value, false);
            }
        });

        Object.defineProperty(self, "loaded", {
            enumerable: true,
            get() {
                return _properties.loaded;
            },
            set(value) {
                _properties.loaded = CYIUtilities.parseBoolean(value, false);
            }
        });

        Object.defineProperty(self, "nickname", {
            enumerable: true,
            get() {
                return _properties.nickname;
            },
            set(value) {
                _properties.nickname = CYIUtilities.trimString(value);
            }
        });

        Object.defineProperty(self, "container", {
            enumerable: true,
            get() {
                return _properties.container;
            },
            set(value) {
                if(value instanceof HTMLElement) {
                    _properties.container = value;
                }
                else {
                    _properties.container = null;
                }
            }
        });

        Object.defineProperty(self, "video", {
            enumerable: true,
            get() {
                return _properties.video;
            },
            set(value) {
                if(value instanceof HTMLVideoElement) {
                    _properties.video = value;

                    if(CYIUtilities.isValid(_properties.requestedVideoRectangle)) {
                        self.setVideoRectangle(_properties.requestedVideoRectangle);
                    }
                }
                else {
                    _properties.video = null;
                }
            }
        });

        Object.defineProperty(self, "verbose", {
            enumerable: true,
            get() {
                return _properties.verbose;
            },
            set(value) {
                _properties.verbose = CYIUtilities.parseBoolean(value, false);
            }
        });

        Object.defineProperty(self, "verboseStateChanges", {
            enumerable: true,
            get() {
                return _properties.verboseStateChanges;
            },
            set(value) {
                _properties.verboseStateChanges = CYIUtilities.parseBoolean(value, false);
            }
        });

        Object.defineProperty(self, "streamFormat", {
            enumerable: true,
            get() {
                return _properties.streamFormat;
            },
            set(value) {
                if(CYIUtilities.isInvalid(value)) {
                    _properties.streamFormat = null;
                }
                else {
                    if(!CYIVideojsVideoPlayerStreamFormat.isValid(value)) {
                        throw CYIUtilities.createError("Invalid stream format: " + JSON.stringify(value));
                    }

                    _properties.streamFormat = value;
                }
            }
        });

        Object.defineProperty(self, "currentDurationSeconds", {
            enumerable: true,
            get() {
                return _properties.currentDurationSeconds;
            },
            set(value) {
                _properties.currentDurationSeconds = CYIUtilities.parseFloatingPointNumber(value, null);
            }
        });

        Object.defineProperty(self, "initialAudioBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.initialAudioBitrateKbps;
            },
            set(value) {
                _properties.initialAudioBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "currentAudioBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.currentAudioBitrateKbps;
            },
            set(value) {
                _properties.currentAudioBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "initialVideoBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.initialVideoBitrateKbps;
            },
            set(value) {
                _properties.initialVideoBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "currentVideoBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.currentVideoBitrateKbps;
            },
            set(value) {
                _properties.currentVideoBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "initialTotalBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.initialTotalBitrateKbps;
            },
            set(value) {
                _properties.initialTotalBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "currentTotalBitrateKbps", {
            enumerable: true,
            get() {
                return _properties.currentTotalBitrateKbps;
            },
            set(value) {
                _properties.currentTotalBitrateKbps = CYIUtilities.parseInteger(value, null);
            }
        });

        Object.defineProperty(self, "requestedVideoRectangle", {
            enumerable: true,
            get() {
                return _properties.requestedVideoRectangle;
            },
            set(value) {
                if(CYIUtilities.isInvalid(value)) {
                    _properties.requestedVideoRectangle = null;
                }
                else {
                    _properties.requestedVideoRectangle = new CYIRectangle(value);
                }
            }
        });

        Object.defineProperty(self, "shouldResumePlayback", {
            enumerable: true,
            get() {
                return _properties.shouldResumePlayback;
            },
            set(value) {
                _properties.shouldResumePlayback = CYIUtilities.parseBoolean(value, null);
            }
        });

        Object.defineProperty(self, "externalTextTrackQueue", {
            enumerable: true,
            get() {
                return _properties.externalTextTrackQueue;
            }
        });

        Object.defineProperty(self, "player", {
            enumerable: true,
            get() {
                return _properties.player;
            },
            set(value) {
                if(CYIUtilities.isObject(value) && value.constructor.name === "Player") {
                    _properties.player = value;
                }
                else {
                    _properties.player = null;
                }
            }
        });

        Object.defineProperty(self, "buffering", {
            enumerable: true,
            get() {
                return _properties.buffering;
            },
            set(value) {
                _properties.buffering = CYIUtilities.parseBoolean(value, false);
            }
        });

        Object.defineProperty(self, "requestedTextTrackId", {
            enumerable: true,
            get() {
                return _properties.requestedTextTrackId;
            },
            set(value) {
                let newValue = CYIUtilities.trimString(value);

                if(CYIUtilities.isEmptyString(newValue)) {
                    newValue = null;
                }

                _properties.requestedTextTrackId = newValue;
            }
        });

        Object.defineProperty(self, "requestedSeekTimeSeconds", {
            enumerable: true,
            get() {
                return _properties.requestedSeekTimeSeconds;
            },
            set(value) {
                let newValue = CYIUtilities.parseFloatingPointNumber(value);

                if(newValue < 0) {
                    newValue = NaN;
                }

                _properties.requestedSeekTimeSeconds = newValue;
            }
        });

        Object.defineProperty(self, "externalTextTrackIdCounter", {
            enumerable: true,
            get() {
                return _properties.externalTextTrackIdCounter;
            },
            set(value) {
                const newValue = CYIUtilities.parseInteger(value);

                if(!isNaN(newValue) && newValue > _properties.externalTextTrackIdCounter) {
                    _properties.externalTextTrackIdCounter = newValue;
                }
            }
        });

        // assign default values for all player properties
        self.initialized = false;
        self.state = CYIVideojsVideoPlayer.State.Uninitialized;
        self.loaded = false;
        self.nickname = null;
        self.container = null;
        self.video = null;
        self.verbose = CYIUtilities.isObjectStrict(configuration) ? CYIUtilities.parseBoolean(configuration.verbose, false) : false;
        self.verboseStateChanges = false;
        self.streamFormat = null;
        self.currentDurationSeconds = null;
        self.initialAudioBitrateKbps = null;
        self.currentAudioBitrateKbps = null;
        self.initialVideoBitrateKbps = null;
        self.currentVideoBitrateKbps = null;
        self.initialTotalBitrateKbps = null;
        self.currentTotalBitrateKbps = null;
        self.shouldResumePlayback = null;
        self.player = null;
        self.requestedTextTrackId = null;
        self.requestedSeekTimeSeconds = NaN;
        self.buffering = false;

        self.registerStreamFormat("DASH", ["PlayReady", "Widevine"]);
        self.registerStreamFormat("HLS", ["PlayReady", "Widevine"]);
        self.registerStreamFormat("MP4");

        self.resetExternalTextTrackIdCounter = function resetExternalTextTrackIdCounter() {
            _properties.externalTextTrackIdCounter = 1;
        };

        document.addEventListener("visibilitychange", function onVisibilityChangeEvent() {
            if(document.hidden) {
                self.suspend();
            }
            else {
                self.restore();
            }
        });

        if(self.verbose) {
            console.log("Created new " + CYIVideojsVideoPlayer.getType() + " " + CYIVideojsVideoPlayer.getVersion() + " player.");
        }
    }

    static createInstance(configuration) {
        return new Promise(function(resolve, reject) {
            if(CYIUtilities.isValid(CYIVideojsVideoPlayer.instance)) {
                throw CYIUtilities.createError("Cannot create more than one " + CYIVideojsVideoPlayer.getType() + " video player instance!");
            }

            return CYIVideojsVideoPlayer.loadDependencies(configuration, function(error) {
                if(error) {
                    return reject(error);
                }

                let videoPlayer = null;

                try {
                    videoPlayer = new CYIVideojsVideoPlayer(configuration);
                }
                catch(error) {
                    return reject(CYIUtilities.createError("Failed to create " + CYIVideojsVideoPlayer.name + " instance: " + error.message));
                }

                CYIVideojsVideoPlayer.instance = videoPlayer;

                return resolve();
            });
        });
    }

    static getInstance() {
        return CYIVideojsVideoPlayer.instance;
    }

    static loadDependencies(options, callback) {
        if(CYIUtilities.isFunction(options)) {
            callback = options;
            options = null;
        }

        if(!CYIUtilities.isFunction(callback)) {
            throw CYIUtilities.createError("Missing or invalid callback function!");
        }

        if(CYIVideojsVideoPlayer.dependenciesLoaded) {
            return callback();
        }

        if(!CYIUtilities.isObjectStrict(options)) {
            options = { };
        }

        const verbose = CYIUtilities.parseBoolean(options.verbose, false);

        let filePaths = CYIVideojsVideoPlayer.DefaultVideoPlayerFileNames;

        if(CYIUtilities.isNonEmptyString(options.filePaths)) {
            filePaths = [options.filePaths];
        }
        else if(CYIUtilities.isNonEmptyArray(options.filePaths)) {
            filePaths = options.filePaths;
        }

        if(CYIUtilities.isNonEmptyString(options.filePaths)) {
            filePaths = [options.filePaths];
        }

        for(let i = 0; i < filePaths.length; i++) {
            if(CYIUtilities.isEmptyString(filePaths[i])) {
                throw CYIUtilities.createError(CYIVideojsVideoPlayer.name + " subclass " + CYIVideojsVideoPlayer.name + " dependency loading called with an invalid video player file path at index " + i + ".");
            }
        }

        return CYIWebFileLoader.getInstance().loadFiles(
            filePaths,
            {
                verbose: verbose
            },
            function(error) {
                if(error) {
                    return callback(error);
                }

                CYIVideojsVideoPlayer.dependenciesLoaded = true;

                return callback();
            }
        );
    }

    static generateAudioTrackTitle(audioTrack, addToTrack) {
        if(!CYIUtilities.isObjectStrict(audioTrack)) {
            return null;
        }

        addToTrack = CYIUtilities.parseBoolean(addToTrack, true);

        let audioTrackTitle = audioTrack.label;

        if(CYIUtilities.isEmptyString(audioTrackTitle)) {
            audioTrackTitle = audioTrack.language;
        }

        if(CYIUtilities.isEmptyString(audioTrackTitle)) {
            audioTrackTitle = audioTrack.id.toString();
        }

        if(addToTrack) {
            audioTrack.title = audioTrackTitle;
        }

        return audioTrackTitle;
    }

    static generateTextTrackTitle(textTrack, addToTrack) {
        if(!CYIUtilities.isObjectStrict(textTrack)) {
            return null;
        }

        addToTrack = CYIUtilities.parseBoolean(addToTrack, true);

        let textTrackTitle = textTrack.label;

        if(CYIUtilities.isEmptyString(textTrackTitle)) {
            textTrackTitle = textTrack.language;
        }

        if(CYIUtilities.isEmptyString(textTrackTitle)) {
            textTrackTitle = textTrack.id.toString();
        }

        if(addToTrack) {
            textTrack.title = textTrackTitle;
        }

        return textTrackTitle;
    }

    static getType() {
        return "Video.js";
    }

    static getVersion() {
        if(typeof videojs === "undefined") {
            return "Unknown";
        }

        return videojs.VERSION;
    }

    static getVersionData() {
        if(typeof videojs === "undefined") {
            return "Unknown";
        }

        if(CYIUtilities.isInvalid(CYIVideojsVideoPlayer.playerVersion)) {
            CYIVideojsVideoPlayer.playerVersion = CYIVideojsVideoPlayer.getVersion();
        }

        return CYIVideojsVideoPlayer.playerVersion;
    }

    initialize(name) {
        const self = this;

        return new Promise(function(resolve, reject) {
            if(CYIPlatformUtilities.isEmbedded) {
                return reject(CYIUtilities.createError(CYIVideojsVideoPlayer.getType() + " is not supported on embedded platforms!"));
            }

            if(typeof videojs === "undefined") {
                return reject(CYIUtilities.createError(CYIVideojsVideoPlayer.getType() + " player is not loaded yet!"));
            }

            if(self.verbose) {
                console.log("Initializing " + CYIVideojsVideoPlayer.getType() + " player...");
            }

            if(self.player) {
                if(self.verbose) {
                    console.log(CYIVideojsVideoPlayer.getType() + " player already initialized!");
                }

                return resolve();
            }

            if(CYIUtilities.isValid(name)) {
                self.setNickname(name);
            }

            self.video = document.createElement("video");
            self.video.id = "videojs_player";

            if(!CYIPlatformUtilities.isEmbedded) {
                self.video.style.width = "100%";
                self.video.style.height = "100%";
            }

            document.body.appendChild(self.video);

            self.player = videojs(self.video.id, {
                controls: false,
                autoplay: false,
                loadingSpinner: false,
                html5: {
                    nativeTextTracks: false,
                    nativeAudioTracks: true,
                    nativeVideoTracks: false,
                    hls: { // applies to DASH as well
                        overrideNative: true
                    }
                }
            });

            // obtain the new parent container element
            self.container = self.video.parentElement;

            if(!CYIPlatformUtilities.isEmbedded) {
                self.container.style.visibility = "hidden";
                self.container.zIndex = 50;

                self.hideUI();
            }

            if(self.verbose) {
                self.player.log.level("debug");
            }

            self.player.on("ready", function onReadyEvent() {
                if(self.verbose) {
                    console.log(self.getDisplayName() + " is ready.");
                }

                self.video = self.player.el().getElementsByTagName("video")[0];
            });

            self.player.on("loadeddata", function onLoadedDataEvent() {
                if(self.verbose) {
                    console.log(self.getDisplayName() + " loaded initial data.");
                }

                self.notifyLiveStatus();

                if(Number.isInteger(self.requestedSeekTimeSeconds)) {
                    if(self.verbose) {
                        console.log(self.getDisplayName() + " processing delayed seek request.");
                    }

                    const requestedSeekTimeSeconds = self.requestedSeekTimeSeconds;

                    self.requestedSeekTimeSeconds = NaN;

                    self.seek(requestedSeekTimeSeconds);
                }
            });

            self.player.on("error", function onErrorEvent(event) {
                self.stop();

                const error = CYIUtilities.createError(self.player.error().message);
                error.code = self.player.error().code;
                error.originalMessage = error.message;
                error.message = self.getDisplayName() + " encountered an unexpected error: " + error.originalMessage;

                return self.sendErrorEvent("playerError", error);
            });

            self.player.on("waiting", function onBufferingStartedEvent(event) {
                if(!self.loaded || self.buffering) {
                    return;
                }

                self.buffering = true;

                self.notifyBufferingStateChanged(true);
            });

            self.player.on("canplay", function onBufferingStartedEvent(event) {
                if(!self.loaded || !self.buffering) {
                    return;
                }

                self.buffering = false;

                self.notifyBufferingStateChanged(false);
            });

            self.player.audioTracks().on("addtrack", function onAudioTrackAddedEvent(event) {
                self.notifyAudioTracksChanged();
            });

            self.player.audioTracks().on("removetrack", function onAudioTrackRemovedEvent(event) {
                self.notifyAudioTracksChanged();
            });

            self.player.audioTracks().on("change", function onAudioTrackChangedEvent(event) {
                self.notifyActiveAudioTrackChanged();
            });

            self.player.textTracks().on("addtrack", function onTextTrackAddedEvent(event) {
                self.notifyTextTracksChanged();
            });

            self.player.textTracks().on("removetrack", function onTextTrackRemovedEvent(event) {
                self.notifyTextTracksChanged();
            });

            self.player.textTracks().on("change", function onTextTrackChangedEvent(event) {
                self.notifyActiveTextTrackChanged();
            });

            self.player.textTracks().on("addtrack", function(event) {
                const textTracks = self.player.textTracks();
                const metadataTextTrack = textTracks[textTracks.length - 1];

                if(CYIUtilities.isInvalid(metadataTextTrack) || metadataTextTrack.kind !== "metadata") {
                    return;
                }

                if(self.verbose) {
                    console.log("Metadata text track detected, adding cue change listener to monitor for timed metadata events.");
                }

                metadataTextTrack.on("cuechange", function(event) {
                    if(metadataTextTrack.activeCues.length === 0) {
                        return;
                    }

                    const activeCue = metadataTextTrack.activeCues[metadataTextTrack.activeCues.length - 1];

                    if(CYIUtilities.isEmptyString(activeCue.value.key) || activeCue.value.key === "PRIV") {
                        return;
                    }

                    self.notifyMetadataAvailable({
                        identifier: activeCue.value.key,
                        value: activeCue.value.data,
                        timestamp: new Date(),
                        durationMs: (activeCue.endTime - activeCue.startTime) * 1000
                    });
                });
            });

            self.player.on("play", function onPlayEvent(event) {
                if(self.state === CYIVideojsVideoPlayer.State.Playing) {
                    return;
                }

                self.updateState(CYIVideojsVideoPlayer.State.Playing);
            });

            self.player.on("pause", function onPauseEvent(event) {
                if(self.state === CYIVideojsVideoPlayer.State.Paused) {
                    return;
                }

                self.updateState(CYIVideojsVideoPlayer.State.Paused);
            });

            self.player.on("ended", function onVideoEndedEvent(event) {
                self.updateState(CYIVideojsVideoPlayer.State.Complete);
            });

            self.player.on("timeupdate", function onTimeUpdateEvent(event) {
                if(!self.player) {
                    return;
                }

                self.notifyVideoTimeChanged();
            });

            self.player.on("durationchange", function onDurationChangedEvent(event) {
                if(!self.player) {
                    return;
                }

                self.notifyVideoDurationChanged();
            });

            self.player.ready(function() {
                self.initialized = true;

                self.updateState(CYIVideojsVideoPlayer.State.Initialized);

                if(self.verbose) {
                    console.log(self.getDisplayName() + " initialized successfully!");
                }

                return resolve();
            });
        });
    }

    checkInitialized() {
        const self = this;

        if(!self.initialized) {
            throw CYIUtilities.createError(self.getDisplayName() + " not initialized!");
        }
    }

    hideUI() {
        const self = this;

        if(CYIUtilities.isInvalid(self.container)) {
            return;
        }

        // hide all child elements excluding the video element and text track display
        for(let i = 0; i < self.container.childNodes.length; i++) {
            const node = self.container.childNodes[i];

            if(node.tagName.toLowerCase() === "video" || node.classList.contains(CYIVideojsVideoPlayer.TextTrackDisplayClassName)) {
                continue;
            }

            node.style.display = "none";
        }
    }

    getDisplayName() {
        const self = this;

        return (self.nickname ? self.nickname + " " : "") + CYIVideojsVideoPlayer.getType() + " player";
    }

    getNickname() {
        const self = this;

        return self.nickname;
    }

    setNickname(nickname) {
        const self = this;

        const formattedName = CYIUtilities.trimString(nickname);

        if(CYIUtilities.isEmptyString(nickname)) {
            self.nickname = null;

            if(self.verbose) {
                console.log("Removing nickname from " + self.getDisplayName() + ".");
            }
        }
        else {
            if(self.verbose) {
                console.log("Changing " + self.getDisplayName() + " nickname to " + formattedName + ".");
            }

            self.nickname = formattedName;
        }
    }

    getPosition() {
        const self = this;

        self.checkInitialized();

        // retrieve the player container / video element absolute position by converting the values from pixel strings to integers
        const position = {
            x: CYIUtilities.parseInteger(self.container.style.left.substring(0, self.container.style.left.length - 2)),
            y: CYIUtilities.parseInteger(self.container.style.top.substring(0, self.container.style.top.length - 2))
        };

        if(!Number.isInteger(position.x) || !Number.isInteger(position.y)) {
            return null;
        }

        return position;
    }

    getSize() {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isInvalid(self.container)) {
            throw CYIUtilities.createError(self.getDisplayName() + " video element is not initialized!");
        }

        // retrieve the player container / video element's width and height by converting the values from pixel strings to integers
        const size = {
            width: CYIUtilities.parseInteger(self.container.style.width.substring(0, self.container.style.width.length - 2)),
            height: CYIUtilities.parseInteger(self.container.style.height.substring(0, self.container.style.height.length - 2))
        };

        if(!Number.isInteger(size.width) || !Number.isInteger(size.height)) {
            return null;
        }

        return size;
    }

    isVerbose() {
        const self = this;

        return self.verbose;
    }

    setVerbose(verbose) {
        const self = this;

        self.verbose = verbose;
    }

    getState() {
        const self = this;

        return self.state;
    }

    updateState(state) {
        const self = this;

        const previousState = self.state;

        if(!CYIVideojsVideoPlayerState.isValid(state)) {
            throw CYIUtilities.createError(self.getDisplayName() + " tried to transition to invalid state!");
        }

        if(!self.state.canTransitionTo(state)) {
            throw CYIUtilities.createError(self.getDisplayName() + " tried to incorrectly transition from " + previousState.displayName + " to " + state.displayName + ".");
        }

        self.state = state;

        self.sendEvent("stateChanged", self.state.id);

        if(self.verbose && self.verboseStateChanges) {
            console.log(self.getDisplayName() + " transitioned from " + previousState.displayName + " to " + state.displayName + ".");
        }
    }

    setVideoRectangle(x, y, width, height) {
        const self = this;

        if(CYIUtilities.isObject(x)) {
            const data = x;
            x = data.x;
            y = data.y;
            width = data.width;
            height = data.height;
        }

        if(CYIUtilities.isInvalid(self.container) || CYIUtilities.isInvalid(self.video)) {
            self.requestedVideoRectangle = new CYIRectangle(x, y, width, height);
            return;
        }

        const formattedXPosition = CYIUtilities.parseInteger(x);
        const formattedYPosition = CYIUtilities.parseInteger(y);
        const formattedWidth = CYIUtilities.parseInteger(width);
        const formattedHeight = CYIUtilities.parseInteger(height);

        if(!Number.isInteger(formattedXPosition)) {
            throw CYIUtilities.createError(self.getDisplayName() + " received an invalid x position for the video rectangle: " + x);
        }

        if(!Number.isInteger(formattedYPosition)) {
            throw CYIUtilities.createError(self.getDisplayName() + " received an invalid y position for the video rectangle: " + y);
        }

        if(!Number.isInteger(formattedWidth) || formattedWidth < 0) {
            throw CYIUtilities.createError(self.getDisplayName() + " received an invalid width for the video rectangle: " + width);
        }

        if(!Number.isInteger(formattedHeight) || formattedHeight < 0) {
            throw CYIUtilities.createError(self.getDisplayName() + " received an invalid height for the video rectangle: " + height);
        }

        self.container.style.position = "absolute";
        self.container.style.left = formattedXPosition + "px";
        self.container.style.top = formattedYPosition + "px";
        self.container.style.width = formattedWidth + "px";
        self.container.style.height = formattedHeight + "px";

        if(self.container !== self.video) {
            self.video.style.width = self.container.style.width;
            self.video.style.height = self.container.style.height;
        }

        self.requestedVideoRectangle = null;
    }

    configureDRM(drmConfiguration) {
        const self = this;

        self.checkInitialized();

        const videojsPlugins = videojs.getPlugins();

        if(CYIUtilities.isInvalid(videojsPlugins.eme)) {
            throw CYIUtilities.createError(CYIVideojsVideoPlayer.getType() + " is missing encrypted media extensions plugin!");
        }

        if(!self.isPaused()) {
            throw CYIUtilities.createError(self.getDisplayName() + " cannot configure DRM after playback has been initiated.");
        }

        var emeOptions = { };

        if(CYIUtilities.isValid(drmConfiguration)) {
            if(!CYIUtilities.isObjectStrict(drmConfiguration)) {
                throw CYIUtilities.createError(self.getDisplayName() + " requires a valid DRM configuration object.");
            }

            if(CYIUtilities.isNonEmptyObject(drmConfiguration)) {
                // initialize videojs-contrib-eme plugin
                self.player.eme();

                emeOptions.keySystems = { };

                if(CYIUtilities.isNonEmptyString(drmConfiguration.url)) {
                    if(CYIUtilities.isNonEmptyString(drmConfiguration.type)) {
                        var formattedDrmType = drmConfiguration.type.trim().toLowerCase();
                        var drmKey = CYIVideojsVideoPlayer.DRMKeys[formattedDrmType];

                        if(CYIUtilities.isNonEmptyString(drmKey)) {
                            var keySystemConfiguration = {
                                url: drmConfiguration.url
                            };

                            if(CYIUtilities.isNonEmptyObject(drmConfiguration.headers)) {
                                keySystemConfiguration.licenseHeaders = drmConfiguration.headers;
                            }

                            emeOptions.keySystems[drmKey] = keySystemConfiguration;
                        }
                        else {
                            throw CYIUtilities.createError(self.getDisplayName() + " tried to configure the player with an unsupported DRM type: " + drmConfiguration.type);
                        }
                    }
                    else {
                        throw CYIUtilities.createError(self.getDisplayName() + " tried to configure the player without a DRM type specified.");
                    }
                }
            }
        }

        if(CYIUtilities.isNonEmptyObject(emeOptions)) {
            self.player.eme.initializeMediaKeys(
                emeOptions,
                function(error) {
                    if(error) {
                        const newError = CYIVideojsVideoPlayer.formatError(error);
                        newError.originalMessage = newError.message;
                        newError.message = self.getDisplayName() + " failed to configure DRM with error: " + newError.message;
                        throw newError;
                    }

                    if(self.verbose) {
                        console.log(self.getDisplayName() + " DRM successfully configured.");
                    }
                },
                true // suppressErrorsIfPossible
            );
        }
    }

    prepare(url, format, startTimeSeconds, maxBitrateKbps, drmConfiguration) {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isObjectStrict(url)) {
            const data = url;
            url = data.url;
            format = data.format;
            startTimeSeconds = data.startTimeSeconds;
            maxBitrateKbps = data.maxBitrateKbps;
            drmConfiguration = data.drmConfiguration;
        }

        if(!self.isStreamFormatSupported(format, CYIUtilities.isObjectStrict(drmConfiguration) ? drmConfiguration.type : null)) {
            throw CYIUtilities.createError(CYIVideojsVideoPlayer.name + " does not support " + format + " stream formats" + (CYIUtilities.isObjectStrict(drmConfiguration) ? " with " + drmConfiguration.type + " DRM." : "."));
        }

        self.updateState(CYIVideojsVideoPlayer.State.Loading);

        if(CYIUtilities.isValid(self.streamFormat)) {
            self.streamFormat.format = format;
        }
        else {
            self.streamFormat = new CYIVideojsVideoPlayer.StreamFormat(format);
        }

        startTimeSeconds = CYIUtilities.parseFloatingPointNumber(startTimeSeconds, 0);

        if(startTimeSeconds < 0) {
            startTimeSeconds = 0;

            console.warn(self.getDisplayName() + " tried to prepare a video with a negative start time.");
        }

        if(CYIUtilities.isEmptyString(url)) {
            throw CYIUtilities.createError(self.getDisplayName() + " requires a non-empty url string to load when preparing.");
        }

        if(CYIUtilities.isValid(maxBitrateKbps)) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " does not support setting a maximum bitrate.");
            }
        }

        self.configureDRM(drmConfiguration);

        if(!CYIPlatformUtilities.isEmbedded) {
            self.container.style.visibility = "visible";
        }

        const sourceConfiguration = {
            src: url
        };

        if(CYIUtilities.isNonEmptyString(self.streamFormat.format)) {
            sourceConfiguration.type = CYIVideojsVideoPlayer.FormatMimeTypes[self.streamFormat.format.toLowerCase()]
        }

        if(startTimeSeconds > 0) {
            self.player.currentTime(startTimeSeconds);
        }

        self.player.src(sourceConfiguration);

        self.loaded = true;

        self.updateState(CYIVideojsVideoPlayer.State.Loaded);

        self.processExternalTextTrackQueue();
    }

    isPlaying() {
        const self = this;

        self.checkInitialized();

        return !self.player.paused();
    }

    isPaused() {
        const self = this;

        self.checkInitialized();

        return self.player.paused();
    }

    play() {
        const self = this;

        self.checkInitialized();

        if(!self.isPaused()) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " tried to start playback, but video is already playing.");
            }

            return;
        }

        if(!self.loaded) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " tried to play while media is not loaded.");
            }

            return;
        }

        self.player.play();

        if(self.verbose) {
            console.log(self.getDisplayName() + " playing.");
        }
    }

    pause() {
        const self = this;

        self.checkInitialized();

        if(self.isPaused()) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " tried to pause playback, but video is already paused.");
            }

            return;
        }

        if(!self.loaded) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " tried to pause while media is not loaded.");
            }

            return;
        }

        self.player.pause();

        if(self.verbose) {
            console.log(self.getDisplayName() + " paused.");
        }
    }

    stop() {
        const self = this;

        if(!self.initialized) {
            return;
        }

        // store the last known player position and size so that it can be used if the player is re-initialized later
        const playerPosition = self.getPosition();
        const playerSize = self.getSize();

        if(CYIUtilities.isValid(playerPosition) && CYIUtilities.isValid(playerSize)) {
            self.requestedVideoRectangle = new CYIRectangle(playerPosition.x, playerPosition.y, playerSize.width, playerSize.height);
        }

        // hide and reset the player
        if(!CYIPlatformUtilities.isEmbedded) {
            self.container.style.visibility = "hidden";
        }

        self.player.reset();

        self.loaded = false;
        self.buffering = false;
        self.shouldResumePlayback = null;
        self.requestedTextTrackId = null;
        self.requestedSeekTimeSeconds = NaN;
        self.externalTextTrackQueue.length = 0;

        self.resetExternalTextTrackIdCounter();

        if(self.state !== CYIVideojsVideoPlayer.State.Initialized) {
            self.updateState(CYIVideojsVideoPlayer.State.Initialized);
        }

        if(self.verbose) {
            console.log(self.getDisplayName() + " stopped.");
        }
    }

    suspend() {
        const self = this;

        if(!self.initialized || !self.loaded) {
            return;
        }

        if(!self.isPaused()) {
            self.shouldResumePlayback = true;

            self.pause();
        }

        if(self.verbose) {
            console.log(self.getDisplayName() + " suspended" + (self.shouldResumePlayback ? ", will resume playback on restore" : "") + ".");
        }
    }

    restore() {
        const self = this;

        if(!self.initialized || !self.loaded) {
            return;
        }

        if(self.shouldResumePlayback) {
            if(self.isPaused()) {
                if(self.verbose) {
                    console.log(self.getDisplayName() + " resuming playback after restore.");
                }

                self.play();
            }

            self.shouldResumePlayback = false;
        }

        if(self.verbose) {
            console.log(self.getDisplayName() + " restored.");
        }
    }

    destroy() {
        const self = this;

        if(!self.player) {
            return;
        }

        self.stop();
        self.state = CYIVideojsVideoPlayer.State.Uninitialized;

        self.player.dispose();

        self.player = null;
        self.initialized = false;
        self.video = null;
        self.container = null;

        CYIVideojsVideoPlayer.instance = null;

        if(self.verbose) {
            console.log(self.getDisplayName() + " disposed.");
        }
    }

    getCurrentTime() {
        const self = this;

        self.checkInitialized();

        return self.player.currentTime();
    }

    getDuration() {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isInvalidNumber(self.player.duration())) {
            return -1;
        }

        return self.player.duration();
    }

    seek(timeSeconds) {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isInvalidNumber(timeSeconds)) {
            return;
        }

        if(isNaN(self.player.duration())) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " tried to seek before player is ready, delaying seek.");
            }

            self.requestedSeekTimeSeconds = timeSeconds;

            return;
        }

        self.player.currentTime(CYIUtilities.clamp(timeSeconds, 0, self.player.duration()));

        if(self.verbose) {
            console.log(self.getDisplayName() + " seeked to " + timeSeconds + "s.");
        }

        self.notifyVideoTimeChanged();
    }

    isMuted() {
        const self = this;

        self.checkInitialized();

        return self.player.muted();
    }

    mute() {
        const self = this;

        self.checkInitialized();

        if(self.player.muted()) {
            return;
        }

        self.player.muted(true);

        if(self.verbose) {
            console.log(self.getDisplayName() + " muted.");
        }
    }

    unmute() {
        const self = this;

        self.checkInitialized();

        if(!self.player.muted()) {
            return;
        }

        self.player.muted(false);

        if(self.verbose) {
            console.log(self.getDisplayName() + " unmuted.");
        }
    }

    setMaxBitrate(maxBitrateKbps) {
        const self = this;

        if(self.verbose) {
            console.warn(self.getDisplayName() + " does not support setting a maximum bitrate.");
        }
    }

    isLive() {
        const self = this;

        self.checkInitialized();

        return self.player.liveTracker.isLive();
    }

    getActiveAudioTrack() {
        const self = this;

        self.checkInitialized();

        let audioTrack = null;
        const audioTracks = self.player.audioTracks();
        let formattedAudioTrack = null;

        for(let i = 0; i < audioTracks.length; i++) {
            audioTrack = audioTracks[i];

            if(!audioTrack.enabled) {
                continue;
            }

            formattedAudioTrack = {
                id: i,
                originalId: audioTrack.id,
                kind: audioTrack.kind,
                label: audioTrack.label,
                language: audioTrack.language,
                active: audioTrack.enabled
            };

            CYIVideojsVideoPlayer.generateAudioTrackTitle(formattedAudioTrack);

            return formattedAudioTrack;
        }

        return null;
    }

    getAudioTracks() {
        const self = this;

        self.checkInitialized();

        let audioTrack = null;
        const audioTracks = self.player.audioTracks();
        let formattedAudioTrack = null;
        const formattedAudioTracks = [];

        for(let i = 0; i < audioTracks.length; i++) {
            audioTrack = audioTracks[i];

            formattedAudioTrack = {
                id: i,
                originalId: audioTrack.id,
                kind: audioTrack.kind,
                label: audioTrack.label,
                language: audioTrack.language,
                active: audioTrack.enabled
            };

            CYIVideojsVideoPlayer.generateAudioTrackTitle(formattedAudioTrack);

            formattedAudioTracks.push(formattedAudioTrack);
        }

        return formattedAudioTracks;
    }

    selectAudioTrack(id) {
        const self = this;

        self.checkInitialized();

        const audioTracks = self.player.audioTracks();

        if(audioTracks.length === 0) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " has no audio tracks.");
            }

            return;
        }

        const formattedAudioTrackId = CYIUtilities.parseInteger(id);

        if(CYIUtilities.isInvalidNumber(formattedAudioTrackId) || formattedAudioTrackId < 0 || formattedAudioTrackId >= audioTracks.length) {
            throw CYIUtilities.createError(self.getDisplayName() + " tried to select an audio track using an invalid number: " + id);
        }

        const activeAudioTrack = self.getActiveAudioTrack();

        if(CYIUtilities.isValid(activeAudioTrack) && activeAudioTrack.id === formattedAudioTrackId) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " tried to select audio track #" + formattedAudioTrackId + " with id: " + activeAudioTrack.originalId + ", but it is already active.");
            }

            return true;
        }

        const selectedAudioTrack = audioTracks[formattedAudioTrackId];

        if(CYIUtilities.isInvalid(selectedAudioTrack)) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " tried to select audio track #" + formattedAudioTrackId + ", but it does not exist.");
            }

            return false;
        }

        selectedAudioTrack.enabled = true;

        if(self.verbose) {
            console.log(self.getDisplayName() + " selected audio track #" + formattedAudioTrackId + " with id: " + selectedAudioTrack.id + ".")
        }

        return false;
    }

    hasTextTracks() {
        const self = this;

        self.checkInitialized();

        let textTrack = null;
        const textTracks = self.player.textTracks();

        for(let i = 0; i < textTracks.length; i++) {
            textTrack = textTracks[i];

            if(textTrack.kind === "subtitles" || textTrack.kind === "captions") {
                return true;
            }
        }

        return false;
    }

    isTextTrackEnabled() {
        const self = this;

        self.checkInitialized();

        let textTrack = null;
        const textTracks = self.player.textTracks();

        for(let i = 0; i < textTracks.length; i++) {
            textTrack = textTracks[i];

            if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                continue;
            }

            if(textTrack.mode === "showing") {
                return true;
            }
        }

        return false;
    }

    enableTextTrack() {
        const self = this;

        self.checkInitialized();

        if(!self.hasTextTracks()) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " has no text tracks.");
            }

            return;
        }

        // check if any text tracks are already visible
        if(self.isTextTrackEnabled()) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " already has a text track enabled.");
            }

            return;
        }

        let textTrack = null;
        const textTracks = self.player.textTracks();

        // if a previous text track was enavled, try to re-enable it
        if(CYIUtilities.isNonEmptyString(self.requestedTextTrackId)) {
            for(let i = 0; i < textTracks.length; i++) {
                textTrack = textTracks[i];

                if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                    continue;
                }

                if(textTrack.id === self.requestedTextTrackId) {
                    textTrack.mode = "showing";

                    if(self.verbose) {
                        console.log(self.getDisplayName() + " enabled text track #" + i + " with id: " + self.requestedTextTrackId + ".");
                    }

                    return;
                }
            }
        }

        // otherwise, activate the first non-visible text track, if any exist
        for(let i = 0; i < textTracks.length; i++) {
            const textTrack = textTracks[i];

            if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                continue;
            }

            if(textTrack.mode !== "disabled") {
                continue;
            }

            self.requestedTextTrackId = textTrack.id;

            textTrack.mode = "showing";

            if(self.verbose) {
                console.log(self.getDisplayName() + " enabled text track #" + i + " with id: " + textTrack.id + (CYIUtilities.isNonEmptyString(textTrack.language) ? " (" + textTrack.language + ")" : "") + ".");
            }

            break;
        }
    }

    disableTextTrack() {
        const self = this;

        return self.disableActiveTextTracks();
    }

    disableActiveTextTracks() {
        const self = this;

        self.checkInitialized();

        let textTrackDisabled = false;
        let textTrack = null;
        const textTracks = self.player.textTracks();

        if(!self.hasTextTracks()) {
            return;
        }

        for(let i = 0; i < textTracks.length; i++) {
            textTrack = textTracks[i];

            if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                continue;
            }

            if(textTrack.mode === "showing") {
                textTrack.mode = "disabled";

                textTrackDisabled = true;

                if(self.verbose) {
                    console.log(self.getDisplayName() + " disabled text track #" + i + " with id: " + textTrack.id + (CYIUtilities.isNonEmptyString(textTrack.language) ? " (" + textTrack.language + ")" : "") + ".");
                }
            }
        }

        if(textTrackDisabled) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " active text tracks disabled.");
            }
        }
    }

    getActiveTextTrack() {
        const self = this;

        self.checkInitialized();

        let textTrack = null;
        const textTracks = self.player.textTracks();
        let formattedTextTrack = null;

        for(let i = 0; i < textTracks.length; i++) {
            textTrack = textTracks[i];

            if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                continue;
            }

            if(textTrack.mode === "showing") {
                formattedTextTrack = {
                    id: i,
                    originalId: textTrack.id,
                    kind: textTrack.kind,
                    mode: textTrack.mode,
                    label: textTrack.label,
                    language: textTrack.language,
                    default: textTrack.default,
                    src: textTrack.src,
                    srclang: textTrack.srclang,
                    active: textTrack.mode === "showing"
                };

                CYIVideojsVideoPlayer.generateTextTrackTitle(formattedTextTrack);

                return formattedTextTrack;
            }
        }

        return null;
    }

    getTextTracks() {
        const self = this;

        let textTrack = null;
        const textTracks = self.player.textTracks();
        let formattedTextTrack = null;
        const formattedTextTracks = [];

        for(let i = 0; i < textTracks.length; i++) {
            textTrack = textTracks[i];

            if(textTrack.kind !== "subtitles" && textTrack.kind !== "captions") {
                continue;
            }

            formattedTextTrack = {
                id: i,
                originalId: textTrack.id,
                kind: textTrack.kind,
                mode: textTrack.mode,
                label: textTrack.label,
                language: textTrack.language,
                default: textTrack.default,
                src: textTrack.src,
                srclang: textTrack.srclang,
                active: textTrack.mode === "showing"
            };

            CYIVideojsVideoPlayer.generateTextTrackTitle(formattedTextTrack);

            formattedTextTracks.push(formattedTextTrack);
        }

        return formattedTextTracks;
    }

    selectTextTrack(id, enable) {
        const self = this;

        self.checkInitialized();

        if(!self.hasTextTracks()) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " has no text tracks.");
            }

            return false;
        }

        const textTracks = self.player.textTracks();
        const formattedTextTrackId = CYIUtilities.parseInteger(id);

        if(CYIUtilities.isInvalidNumber(formattedTextTrackId) || formattedTextTrackId < 0 || formattedTextTrackId >= textTracks.length) {
            throw CYIUtilities.createError(self.getDisplayName() + " tried to select a text track using an invalid id: " + id);
        }

        let trackActivated = false;
        const activeTextTrack = self.getActiveTextTrack();
        let selectedTextTrack = textTracks[formattedTextTrackId];

        if(CYIUtilities.isObject(activeTextTrack) && activeTextTrack.id === formattedTextTrackId) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " tried to select text track #" + activeTextTrack.id + " with id: " + formattedTextTrackId + ", but it is already active.");
            }

            return true;
        }
        else if(CYIUtilities.isInvalid(selectedTextTrack)) {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " could not find a text track with id: " + formattedTextTrackId);
            }

            return false;
        }
        else if(selectedTextTrack.kind !== "subtitles" && selectedTextTrack.kind !== "captions") {
            if(self.verbose) {
                console.warn(self.getDisplayName() + " could not select text track with kind: " + selectedTextTrack.kind);
            }

            return false;
        }
        else if(selectedTextTrack.mode === "showing") {
            if(self.verbose) {
                console.log(self.getDisplayName() + " tried to select text track #" + activeTextTrack.id + " with id: " + formattedTextTrackId + ", but it is already active.");
            }

            return true;
        }
        else {
            self.disableActiveTextTracks();

            self.requestedTextTrackId = selectedTextTrack.id;

            selectedTextTrack.mode = "showing";

            if(self.verbose) {
                console.log(self.getDisplayName() + " selected text track #" + formattedTextTrackId + " with id: " + selectedTextTrack.id + (CYIUtilities.isNonEmptyString(selectedTextTrack.language) ? " (" + selectedTextTrack.language + ")." : ""));
            }

            return true;
        }
    }

    addExternalTextTrack(url, language, label, type, format, enable) {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isObjectStrict(url)) {
            const data = url;
            url = data.url;
            language = data.language;
            label = data.label;
            type = data.type;
            format = data.format;
            enable = data.enable;
        }

        url = CYIUtilities.trimString(url);
        language = CYIUtilities.trimString(language);
        label = CYIUtilities.trimString(label, "");
        format = CYIUtilities.trimString(format);
        type = CYIUtilities.trimString(type);
        enable = CYIUtilities.parseBoolean(enable, true);

        if(CYIUtilities.isEmptyString(url)) {
            throw CYIUtilities.createError(self.getDisplayName() + " missing or invalid external text track url value.");
        }

        if(CYIUtilities.isEmptyString(language)) {
            throw CYIUtilities.createError(self.getDisplayName() + " missing or invalid external text track language value.");
        }

        if(CYIUtilities.isEmptyString(format)) {
            throw CYIUtilities.createError(self.getDisplayName() + " missing or invalid external text track format value.");
        }

        if(CYIUtilities.isEmptyString(type)) {
            type = "captions";

            console.warn(self.getDisplayName() + " missing or invalid external text track type value, defaulting to caption.");
        }

        if(!self.loaded) {
            self.externalTextTrackQueue.push({
                url: url,
                language: language,
                label: label,
                type: type,
                format: format,
                enable: enable
            });

            return console.warn(self.getDisplayName() + " tried to add an external text track before video finished loading, storing in queue to add after the video has loaded.");
        }

        const generatedExternalTextTrackId = CYIVideojsVideoPlayer.ExternalTrackPrefix + self.externalTextTrackIdCounter++;

        const textTrackElement = self.player.addRemoteTextTrack(
            {
                id: generatedExternalTextTrackId,
                kind: type,
                mode: enable ? "showing" : "disabled",
                label: label,
                language: language,
                src: url
            },
            true // manualCleanup
        );

        textTrackElement.on("load", function() {
            if(self.verbose) {
                console.log(self.getDisplayName() + " loaded " + (enable ? "and enabled " : "") + "external text track with id: \"" + generatedExternalTextTrackId + "\".");
            }

            let textTrack = null;
            const textTracks = self.player.textTracks();

            for(let i = 0; i < textTracks.length; i++) {
                textTrack = textTracks[i];

                if(textTrack.id !== generatedExternalTextTrackId) {
                    continue;
                }

                const formattedTextTrack = {
                    id: i,
                    originalId: textTrack.id,
                    kind: textTrack.kind,
                    mode: textTrack.mode,
                    label: textTrack.label,
                    language: textTrack.language,
                    default: textTrack.default,
                    src: textTrack.src,
                    srclang: textTrack.srclang,
                    active: textTrack.mode === "showing"
                };

                CYIVideojsVideoPlayer.generateTextTrackTitle(formattedTextTrack);

                self.sendEvent("externalTextTrackAdded", formattedTextTrack);
            }
        });
    }

    processExternalTextTrackQueue() {
        const self = this;

        self.checkInitialized();

        if(!self.loaded || self.externalTextTrackQueue.length === 0) {
            return;
        }

        if(self.verbose) {
            console.log(self.getDisplayName() + " processing external text track queue...");
        }

        for(let i = 0; i < self.externalTextTrackQueue.length; i++) {
            if(self.verbose) {
                console.log(self.getDisplayName() + " adding external text track " + (i + 1) + " / " + self.externalTextTrackQueue.length + "...");
            }

            self.addExternalTextTrack(self.externalTextTrackQueue[i]);
        }

        self.externalTextTrackQueue.length = 0;
    }

    numberOfStreamFormats() {
        const self = this;

        return self.streamFormats.length;
    }

    hasStreamFormat(streamFormat) {
        const self = this;

        return self.indexOfStreamFormat() !== -1;
    }

    indexOfStreamFormat(streamFormat) {
        const self = this;

        let formattedStreamFormat = null;

        if(CYIVideojsVideoPlayerStreamFormat.isStreamFormat(streamFormat) || CYIUtilities.isObjectStrict(streamFormat)) {
            formattedStreamFormat = CYIUtilities.trimString(streamFormat.format);
        }
        else if(CYIUtilities.isNonEmptyString(streamFormat)) {
            formattedStreamFormat = CYIUtilities.trimString(streamFormat);
        }

        if(CYIUtilities.isEmptyString(formattedStreamFormat)) {
            return -1;
        }

        for(let i = 0; i < self.streamFormats.length; i++) {
            if(CYIUtilities.equalsIgnoreCase(self.streamFormats[i].format, formattedStreamFormat)) {
                return i;
            }
        }

        return -1;
    }

    getStreamFormat(streamFormat) {
        const self = this;

        const streamFormatIndex = self.indexOfStreamFormat(streamFormat);

        if(streamFormatIndex === -1) {
            return null;
        }

        return self.streamFormats[streamFormatIndex];
    }

    registerStreamFormat(streamFormat, drmTypes) {
        const self = this;

        let streamFormatInfo = null;

        if(CYIVideojsVideoPlayerStreamFormat.isStreamFormat(streamFormat)) {
            streamFormatInfo = streamFormat;
        }
        else {
            const formattedStreamFormat = CYIUtilities.trimString(streamFormat);

            if(CYIUtilities.isEmptyString(formattedStreamFormat)) {
                console.error(CYIVideojsVideoPlayer.name + " cannot register stream format with empty or invalid format.");
                return null;
            }

            const formattedDRMTypes = [];

            if(CYIUtilities.isNonEmptyArray(drmTypes)) {
                for(let i = 0; i < drmTypes.length; i++) {
                    const formattedDRMType = CYIUtilities.trimString(drmTypes[i]);

                    if(CYIUtilities.isEmptyString(formattedDRMType)) {
                        console.error(CYIVideojsVideoPlayer.name + " skipping registration of empty or invalid DRM type for " + formattedStreamFormat + " stream format.");
                        continue;
                    }

                    for(let j = 0; j < formattedDRMTypes.length; j++) {
                        if(CYIUtilities.equalsIgnoreCase(formattedDRMTypes[j], formattedDRMType)) {
                            console.warn(CYIVideojsVideoPlayer.name + " already has " + formattedDRMType + " DRM type registered for " + formattedStreamFormat + " stream format.");
                            continue;
                        }
                    }

                    if(!CYIPlatformUtilities.isDRMTypeSupported(formattedDRMType)) {
                        continue;
                    }

                    formattedDRMTypes.push(formattedDRMType);
                }
            }

            streamFormatInfo = new CYIVideojsVideoPlayerStreamFormat(formattedStreamFormat, formattedDRMTypes);
        }

        if(!streamFormatInfo.isValid()) {
            console.error(CYIVideojsVideoPlayer.name + " tried to register an invalid stream format!");
            return null;
        }

        const registeredStreamFormat = self.getStreamFormat(streamFormatInfo);

        if(CYIUtilities.isValid(registeredStreamFormat)) {
            console.warn(CYIVideojsVideoPlayer.name + " already has " + registeredStreamFormat.format + " stream format registered!");
            return null;
        }

        self.streamFormats.push(streamFormatInfo);

        return streamFormatInfo;
    }

    unregisterStreamFormat(streamFormat) {
        const self = this;

        const streamFormatIndex = self.indexOfStreamFormat(streamFormat);

        if(streamFormatIndex === -1) {
            return false;
        }

        return self.streamFormats.splice(streamFormatIndex, 1);
    }

    addDRMTypeToStreamFormat(streamFormat, drmType) {
        const self = this;

        const streamFormatInfo = self.getStreamFormat(streamFormat);

        if(!CYIVideojsVideoPlayerStreamFormat.isValid(streamFormatInfo)) {
            return false;
        }

        return streamFormatInfo.addDRMType(drmType);
    }

    removeDRMTypeFromStreamFormat(streamFormat, drmType) {
        const self = this;

        const streamFormatInfo = self.getStreamFormat(streamFormat);

        if(!CYIVideojsVideoPlayerStreamFormat.isValid(streamFormatInfo)) {
            return false;
        }

        return streamFormatInfo.removeDRMType(drmType);
    }

    removeAllDRMTypesFromStreamFormat(streamFormat) {
        const self = this;

        const streamFormatInfo = self.getStreamFormat(streamFormat);

        if(!CYIVideojsVideoPlayerStreamFormat.isValid(streamFormatInfo)) {
            return false;
        }

        streamFormatInfo.clearDRMTypes();

        return true;
    }

    isStreamFormatSupported(streamFormat, drmType, custom) {
        const self = this;

        const streamFormatInfo = self.getStreamFormat(streamFormat);

        if(!CYIVideojsVideoPlayerStreamFormat.isValid(streamFormatInfo)) {
            return false;
        }

        if(CYIUtilities.isNonEmptyString(drmType)) {
            // NOTE: Widevine Modular custom requests are not currently supported
            return streamFormatInfo.hasDRMType(drmType) && !custom;
        }

        return true;
    }

    clearStreamFormats() {
        const self = this;

        self.streamFormats.length = 0;
    }

    notifyLiveStatus() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("liveStatus", self.isLive());
    }

    notifyBitrateChanged() {
        const self = this;

        self.checkInitialized();

        const bitrateData = { };

        if(CYIUtilities.isValidNumber(self.initialAudioBitrateKbps) || CYIUtilities.isValidNumber(self.currentAudioBitrateKbps)) {
            bitrateData.initialAudioBitrateKbps = Math.floor(self.initialAudioBitrateKbps);
            bitrateData.currentAudioBitrateKbps = Math.floor(self.currentAudioBitrateKbps);
        }

        if(CYIUtilities.isValidNumber(self.initialVideoBitrateKbps) || CYIUtilities.isValidNumber(self.currentVideoBitrateKbps)) {
            bitrateData.initialVideoBitrateKbps = Math.floor(self.initialVideoBitrateKbps);
            bitrateData.currentVideoBitrateKbps = Math.floor(self.currentVideoBitrateKbps);
        }

        if(CYIUtilities.isValidNumber(self.initialTotalBitrateKbps) || CYIUtilities.isValidNumber(self.currentTotalBitrateKbps)) {
            bitrateData.initialTotalBitrateKbps = Math.floor(self.initialTotalBitrateKbps);
            bitrateData.currentTotalBitrateKbps = Math.floor(self.currentTotalBitrateKbps);
        }

        self.sendEvent("bitrateChanged", bitrateData);
    }

    notifyBufferingStateChanged(buffering) {
        const self = this;

        self.checkInitialized();

        buffering = CYIUtilities.parseBoolean(buffering);

        if(CYIUtilities.isInvalid(buffering)) {
            return console.error(self.getDisplayName() + " tried to send an invalid buffering state value, expected a valid boolean value.");
        }

        self.sendEvent("bufferingStateChanged", buffering);
    }

    notifyVideoTimeChanged() {
        const self = this;

        self.checkInitialized();

        const bufferedTimeRanges = self.video.buffered;
        let bufferedTimeRange = null;

        for(let i = 0; i < bufferedTimeRanges.length; i++) {
            if(self.video.currentTime >= bufferedTimeRanges.start(i) && self.video.currentTime <= bufferedTimeRanges.end(i)) {
                bufferedTimeRange = {
                    start: bufferedTimeRanges.start(i),
                    end: bufferedTimeRanges.end(i)
                };

                break;
            }
        }

        const data = {
            currentTimeSeconds: self.getCurrentTime()
        };

        if(CYIUtilities.isValid(bufferedTimeRange)) {
            data.bufferStartMs = Math.floor(bufferedTimeRange.start * 1000);
            data.bufferEndMs = Math.floor(bufferedTimeRange.end * 1000);
            data.bufferLengthMs = Math.floor((bufferedTimeRange.end - self.video.currentTime) * 1000);
        }
        else {
            data.bufferStartMs = 0;
            data.bufferEndMs = 0;
            data.bufferLengthMs = 0;
        }

        self.sendEvent("videoTimeChanged", data);
    }

    notifyVideoDurationChanged() {
        const self = this;

        self.checkInitialized();

        if(CYIUtilities.isInvalidNumber(self.getDuration()) || self.getDuration() < 0) {
            return;
        }

        self.sendEvent("videoDurationChanged", self.getDuration());
    }

    notifyActiveAudioTrackChanged() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("activeAudioTrackChanged", self.getActiveAudioTrack());
    }

    notifyAudioTracksChanged() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("audioTracksChanged", self.getAudioTracks());
    }

    notifyActiveTextTrackChanged() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("activeTextTrackChanged", self.getActiveTextTrack());
    }

    notifyTextTracksChanged() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("textTracksChanged", self.getTextTracks());
    }

    notifyTextTrackStatusChanged() {
        const self = this;

        self.checkInitialized();

        self.sendEvent("textTrackStatusChanged", self.isTextTrackEnabled());
    }

    notifyMetadataAvailable(identifier, value, timestamp, durationMs) {
        const self = this;

        self.checkInitialized();

        const metadata = CYIUtilities.isObjectStrict(identifier) ? identifier : {
            identifier: identifier,
            value: value,
            timestamp: timestamp,
            durationMs: durationMs
        };

        metadata.identifier = CYIUtilities.trimString(metadata.identifier);
        metadata.value = CYIUtilities.toString(metadata.value);
        metadata.timestamp = CYIUtilities.parseDate(metadata.timestamp, new Date()).getTime();
        metadata.durationMs = CYIUtilities.parseInteger(metadata.durationMs, -1);

        if(CYIUtilities.isEmptyString(metadata.identifier)) {
            if(self.verbose) {
                console.warn("Failed to send metadata event with empty identifier.");
            }

            return false;
        }

        self.sendEvent("metadataAvailable", metadata);

        return true;
    }

    sendEvent(eventName, data) {
        const self = this;

        return CYIMessaging.sendEvent({
            context: CYIVideojsVideoPlayer.name,
            name: eventName,
            data: data
        });
    }

    sendErrorEvent(eventName, error) {
        const self = this;

        return CYIMessaging.sendEvent({
            context: CYIVideojsVideoPlayer.name,
            name: eventName,
            error: error
        });
    }

    static formatError(error) {
        if(!CYIUtilities.isObject(error)) {
            return CYIUtilities.createError("Unknown error.");
        }

        const newError = CYIUtilities.createError(CYIUtilities.isNonEmptyString(error.message) ? error.message : "Unknown error.");

        if(CYIUtilities.isValid(error.stack)) {
            newError.stack = error.stack;
        }

        return newError;
    }
}

Object.defineProperty(CYIVideojsVideoPlayerVersion, "PlayerVersionRegex", {
    value: /((0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*))/,
    enumerable: false
});

Object.defineProperty(CYIVideojsVideoPlayer, "StreamFormat", {
    value: CYIVideojsVideoPlayerStreamFormat,
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "BitrateKbpsScale", {
    value: 1000,
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "State", {
    enumerable: true,
    value: CYIVideojsVideoPlayerState
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Invalid", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(-1, "Invalid")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Uninitialized", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(0, "Uninitialized")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Initialized", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(1, "Initialized")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Loading", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(2, "Loading")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Loaded", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(3, "Loaded")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Paused", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(4, "Paused")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Playing", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(5, "Playing")
});

Object.defineProperty(CYIVideojsVideoPlayerState, "Complete", {
    enumerable: true,
    value: new CYIVideojsVideoPlayerState(6, "Complete")
});

CYIVideojsVideoPlayerState.Uninitialized.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized
];

CYIVideojsVideoPlayerState.Initialized.transitionStates = [
    CYIVideojsVideoPlayerState.Loading
];

CYIVideojsVideoPlayerState.Loading.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized,
    CYIVideojsVideoPlayerState.Loaded
];

CYIVideojsVideoPlayerState.Loaded.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized,
    CYIVideojsVideoPlayerState.Paused,
    CYIVideojsVideoPlayerState.Playing
];

CYIVideojsVideoPlayerState.Paused.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized,
    CYIVideojsVideoPlayerState.Playing,
    CYIVideojsVideoPlayerState.Complete
];

CYIVideojsVideoPlayerState.Playing.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized,
    CYIVideojsVideoPlayerState.Paused,
    CYIVideojsVideoPlayerState.Complete
];

CYIVideojsVideoPlayerState.Complete.transitionStates = [
    CYIVideojsVideoPlayerState.Initialized
];

Object.defineProperty(CYIVideojsVideoPlayer, "States", {
    enumerable: true,
    value: [
        CYIVideojsVideoPlayerState.Uninitialized,
        CYIVideojsVideoPlayerState.Initialized,
        CYIVideojsVideoPlayerState.Loading,
        CYIVideojsVideoPlayerState.Loaded,
        CYIVideojsVideoPlayerState.Paused,
        CYIVideojsVideoPlayerState.Playing,
        CYIVideojsVideoPlayerState.Complete
    ]
});

Object.defineProperty(CYIVideojsVideoPlayer, "properties", {
    value: new CYIVideojsVideoPlayerProperties(),
    enumerable: false
});

Object.defineProperty(CYIVideojsVideoPlayer, "instance", {
    enumerable: true,
    get() {
        return CYIVideojsVideoPlayer.properties.instance;
    },
    set(value) {
        CYIVideojsVideoPlayer.properties.instance = value;
    }
});

Object.defineProperty(CYIVideojsVideoPlayer, "script", {
    enumerable: true,
    get() {
        return CYIVideojsVideoPlayer.properties.script;
    },
    set(value) {
        CYIVideojsVideoPlayer.properties.script = value;
    }
});

Object.defineProperty(CYIVideojsVideoPlayer, "dependenciesLoaded", {
    enumerable: true,
    get() {
        return CYIVideojsVideoPlayer.properties.dependenciesLoaded;
    },
    set(value) {
        CYIVideojsVideoPlayer.properties.dependenciesLoaded = value;
    }
});

Object.defineProperty(CYIVideojsVideoPlayer, "playerVersion", {
    enumerable: true,
    get() {
        return CYIVideojsVideoPlayer.properties.playerVersion;
    },
    set(value) {
        CYIVideojsVideoPlayer.properties.playerVersion = value;
    }
});

Object.defineProperty(CYIVideojsVideoPlayer, "ExternalTrackPrefix", {
    value: "external",
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "DRMKeys", {
    value: {
        playready: "com.microsoft.playready",
        widevine: "com.widevine.alpha"
    },
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "FormatMimeTypes", {
    value: {
        mp4: "video/mp4",
        dash: "application/dash+xml",
        hls: "application/x-mpegURL"
    },
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "TextTrackDisplayClassName", {
    value: "vjs-text-track-display",
    enumerable: true
});

Object.defineProperty(CYIVideojsVideoPlayer, "DefaultVideoPlayerFileNames", {
    value: [
        "video.js",
        "videojs-contrib-eme.js"
    ],
    enumerable: true
});

CYIVideojsVideoPlayer.script = document.currentScript;

window.CYIVideojsVideoPlayer = CYIVideojsVideoPlayer;
