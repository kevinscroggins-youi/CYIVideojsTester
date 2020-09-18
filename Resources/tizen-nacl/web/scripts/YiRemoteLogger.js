function CYIRemoteLogger() { }

CYIRemoteLogger.enabled = false; // flag for enabling remote console debugging using XMLHttpRequest to RESTful API server
CYIRemoteLogger.address = "10.100.123.123"; // host or ip address of the remote console API server
CYIRemoteLogger.port = 3000; // port the remote console API server is listening on
CYIRemoteLogger.batchingEnabled = true;
CYIRemoteLogger.messageIdCounter = 1;
CYIRemoteLogger.consoleLogTypes = ["log", "info", "warn", "error"];
CYIRemoteLogger.printLogFunction = null;
CYIRemoteLogger.legacy = null;
CYIRemoteLogger.consoleHooked = false;
CYIRemoteLogger.consoleAlreadyHooked = false;
CYIRemoteLogger.printLogHooked = false;
CYIRemoteLogger.shouldHookPrintLogFunction = false;
CYIRemoteLogger.originalConsoleLogFunctions = { };
CYIRemoteLogger.modifiedConsoleLogFunctions = { };
CYIRemoteLogger.originalPrintLogFunction = null;
CYIRemoteLogger.originalPrintLogFunctionRequiresTypeArgument = false;
CYIRemoteLogger.originalHookConsoleLoggingFunction = null;
CYIRemoteLogger.originalUnhookConsoleLoggingFunction = null;
CYIRemoteLogger.messageQueue = [];
CYIRemoteLogger.messageSendIntervalMs = 250;
CYIRemoteLogger.maxBatchSize = 100;
CYIRemoteLogger.messageBatchTimer = null;
CYIRemoteLogger.messageResendTimer = null;
CYIRemoteLogger.verbose = true;
CYIRemoteLogger.initialized = false;
CYIRemoteLogger.logSinkInitialized = false;
CYIRemoteLogger.metadataHeaders = null;
CYIRemoteLogger.sessionTimestamp = 0;
CYIRemoteLogger.version = "1.1.10"; // should match the version on the RESTful API server, but there is backwards and forwards compatibility as well, for now

CYIRemoteLogger.initialize = function initialize() {
	// do not allow the remote logger to be initialized if it is disabled
	if(!CYIRemoteLogger.enabled) {
		return false;
	}

	// check if the remote logger is already initialized
	if(CYIRemoteLogger.initialized) {
		return true;
	}

	CYIRemoteLogger.sendDebugRemoteLog("Initializing Tizen remote logger with remote API address: '" + CYIRemoteLogger.address + ":" + CYIRemoteLogger.port + "'...");

	if(typeof application !== "undefined") {
		CYIRemoteLogger.printLogFunction = application.printLog;
		CYIRemoteLogger.legacy = true;
	}
	else if(typeof CYIApplication !== "undefined") {
		CYIRemoteLogger.printLogFunction = CYILogger.log;
		CYIRemoteLogger.legacy = false;
	}

	// create unique timestamp for the current session
	CYIRemoteLogger.sessionTimestamp = new Date().getTime();

	// intialize metadata header values
	CYIRemoteLogger.initializeMetadataHeaders();

	// backup the original printLog and console hooking / unhooking functions
	if(CYIRemoteLogger.legacy !== null) {
		CYIRemoteLogger.originalPrintLogFunction = CYIRemoteLogger.legacy ? application.printLog : CYILogger.log;
		CYIRemoteLogger.originalPrintLogFunctionRequiresTypeArgument = typeof CYILogger !== "undefined" && CYILogger.appendToLogArea instanceof Function;
		CYIRemoteLogger.originalHookConsoleLoggingFunction = CYIRemoteLogger.legacy ? application.hookConsoleLogging : CYILogger.hookConsoleLogging;
		CYIRemoteLogger.originalUnhookConsoleLoggingFunction = CYIRemoteLogger.legacy ? application.unhookConsoleLogging : CYILogger.unhookConsoleLogging;
	}

	// hook logging functions
	CYIRemoteLogger.hookLoggingFunctions();

	// hook application console logging hooking functions
	if((CYIRemoteLogger.originalHookConsoleLoggingFunction instanceof Function) && (CYIRemoteLogger.originalUnhookConsoleLoggingFunction instanceof Function)) {
		function hookConsoleLogging() {
			// invoke the original hook console logging function
			CYIRemoteLogger.originalHookConsoleLoggingFunction.apply(CYIRemoteLogger.legacy ? application : CYILogger, arguments);

			// invoke the remote logger hook console logging function
			CYIRemoteLogger.hookConsoleLogging.apply(CYIRemoteLogger, arguments);
		}

		function unhookConsoleLogging() {
			// invoke the remote logger unhook console logging function
			CYIRemoteLogger.unhookConsoleLogging.apply(CYIRemoteLogger, arguments);

			// invoke the original unhook console logging function
			CYIRemoteLogger.originalUnhookConsoleLoggingFunction.apply(CYIRemoteLogger.legacy ? application : CYILogger, arguments);
		}

		if(CYIRemoteLogger.legacy) {
			application.hookConsoleLogging = hookConsoleLogging;
			application.unhookConsoleLogging = unhookConsoleLogging;
		}
		else {
			CYILogger.hookConsoleLogging = hookConsoleLogging;
			CYILogger.unhookConsoleLogging = unhookConsoleLogging;
		}
	}

	// check if log message batching is enabled
	if(CYIRemoteLogger.batchingEnabled) {
		if(CYIRemoteLogger.verbose) {
			var message = "Message batching enabled, starting message batching timer with " + (CYIRemoteLogger.messageSendIntervalMs / 1000) + "s interval.";

			if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
				CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
			}

			CYIRemoteLogger.sendDebugRemoteLog(message);
		}

		// start a timer to send groups of messages in each request at regular intervals to reduce the amount of network traffic
		CYIRemoteLogger.messageBatchTimer = setInterval(function() {
			if(CYIRemoteLogger.messageQueue.length > 1000) {
				CYIRemoteLogger.sendDebugRemoteLog("Message queue size has exceeded 1000 messages, the logger may be overloaded and no longer be able to send message data to the remote logging system. Try lowering your max batch size in the remote logger client.", "error");
			}

			// send a batch of messages to the remote server, but limit the number of messages sent at one time to avoid overloading the server
			CYIRemoteLogger.sendRemoteLogBatch(CYIRemoteLogger.messageQueue.slice(0, CYIRemoteLogger.maxBatchSize));
		}, CYIRemoteLogger.messageSendIntervalMs);
	}
	else {
		if(CYIRemoteLogger.verbose) {
			var message = "Message batching is disabled, starting message resend timer with " + (CYIRemoteLogger.messageSendIntervalMs / 1000) + "s interval.";

			if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
				CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
			}

			CYIRemoteLogger.sendDebugRemoteLog(message);
		}

		// start a timer to automatically re-send messages which were not received at regular intervals to ensure that connectivity interruptions do not prevent messages from getting through
		CYIRemoteLogger.messageResendTimer = setInterval(function() {
			for(var i = 0; i < CYIRemoteLogger.messageQueue.length; i++) {
				CYIRemoteLogger.sendRemoteLog(CYIRemoteLogger.messageQueue[i]);
			}
		}, CYIRemoteLogger.messageSendIntervalMs);
	}

	CYIRemoteLogger.initialized = true;

	if(typeof CYIMessaging !== "undefined") {
		CYIMessaging.sendEvent({
			context: CYIRemoteLogger.name,
			name: "initialized"
		});
	}

	CYIRemoteLogger.sendDebugRemoteLog("Tizen remote logger initialized successfully!");

	return true;
};

CYIRemoteLogger.initializeMetadataHeaders = function initializeMetadataHeaders() {
	if(CYIRemoteLogger.metadataHeaders) {
		return;
	}

	// configure request headers with session and remote logger metadata
	CYIRemoteLogger.metadataHeaders = {
		"Remote-Logger-Version": CYIRemoteLogger.version,
		"Session-Timestamp": CYIRemoteLogger.sessionTimestamp
	};

	// configure request headers with device and application metadata
	try {
		CYIRemoteLogger.metadataHeaders["Device-Duid"] = webapis.productinfo.getDuid();
		CYIRemoteLogger.metadataHeaders["Device-Model"] = webapis.productinfo.getModel();
		CYIRemoteLogger.metadataHeaders["Device-Model-Name"] = tizen.systeminfo.getCapability("http://tizen.org/system/model_name");
		CYIRemoteLogger.metadataHeaders["Device-Platform-Name"] = tizen.systeminfo.getCapability("http://tizen.org/system/platform.name");
		CYIRemoteLogger.metadataHeaders["Device-Platform-Version"] = tizen.systeminfo.getCapability("http://tizen.org/feature/platform.native.api.version");
		CYIRemoteLogger.metadataHeaders["Device-Firmware-Version"] = webapis.productinfo.getFirmware();
		CYIRemoteLogger.metadataHeaders["Device-Manufacturer-Name"] = tizen.systeminfo.getCapability("http://tizen.org/system/manufacturer");
		CYIRemoteLogger.metadataHeaders["Application-Identifier"] = tizen.application.getCurrentApplication().appInfo.id;
	}
	catch(error) {
		var message = "Failed to retrieve device metadata: " + error.message;

		if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
			CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
		}

		CYIRemoteLogger.sendDebugRemoteLog(message);
	}
};

CYIRemoteLogger.onLogSinkInitialized = function onLogSinkInitialized() {
	if(CYIRemoteLogger.logSinkInitialized) {
		return;
	}

	CYIRemoteLogger.logSinkInitialized = true;

	CYIRemoteLogger.unhookPrintLogFunction();

	CYIRemoteLogger.shouldHookPrintLogFunction = false;
};

CYIRemoteLogger.hookLoggingFunctions = function hookLoggingFunctions() {
	// do not allow logging functions to be hooked if disabled or already hooked
	if(!CYIRemoteLogger.enabled || (CYIRemoteLogger.consoleHooked || CYIRemoteLogger.printLogHooked)) {
		return;
	}

	CYIRemoteLogger.hookConsoleLogging();

	CYIRemoteLogger.hookPrintLogFunction();
};

CYIRemoteLogger.hookConsoleLogging = function hookConsoleLogging() {
	// do not allow console logging to be hooked if disabled or already hooked
	if(!CYIRemoteLogger.enabled || CYIRemoteLogger.consoleHooked) {
		return;
	}

	if(CYIRemoteLogger.verbose) {
		var message = "Injecting remote log hooks into original console logging functions...";

		if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
			CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
		}

		CYIRemoteLogger.sendDebugRemoteLog(message);
	}

	var consoleLogType = null;

	// inject new console logging function wrappers
	for(var i = 0; i < CYIRemoteLogger.consoleLogTypes.length; i++) {
		consoleLogType = CYIRemoteLogger.consoleLogTypes[i];

		// check if the current console logging function is already hooked
		var functionAlreadyHooked = console[consoleLogType] !== CYIRemoteLogger.originalConsoleLogFunctions[consoleLogType];

		if(functionAlreadyHooked) {
			CYIRemoteLogger.consoleAlreadyHooked = true;
		}

		// backup the newest version of the current console logging function
		CYIRemoteLogger.modifiedConsoleLogFunctions[consoleLogType] = console[consoleLogType];

		// set the new wrapper function hook for the current console logging function
		console[consoleLogType] = (function(consoleLogType) {
			return function() {
				// invoke the original console logging function
				CYIRemoteLogger.modifiedConsoleLogFunctions[consoleLogType].apply(console, arguments);

				// invoke the remote logging function with the same arguments
				CYIRemoteLogger.remoteLog(consoleLogType, Array.prototype.slice.call(arguments));
			}
		})(consoleLogType);
	}

	CYIRemoteLogger.consoleHooked = true;
};

CYIRemoteLogger.hookPrintLogFunction = function hookPrintLogFunction() {
	// do not allow printLog function to be hooked if disabled or already hooked
	if(!CYIRemoteLogger.enabled || CYIRemoteLogger.printLogHooked || !(CYIRemoteLogger.originalPrintLogFunction instanceof Function)) {
		return;
	}

	// determine if printLog function should be hooked
	CYIRemoteLogger.shouldHookPrintLogFunction = CYIRemoteLogger.legacy && (!CYIRemoteLogger.consoleAlreadyHooked || ((CYIRemoteLogger.originalHookConsoleLoggingFunction instanceof Function) && (CYIRemoteLogger.originalUnhookConsoleLoggingFunction instanceof Function)));

	// check if the console logging functions have not already been hooked
	if(CYIRemoteLogger.shouldHookPrintLogFunction) {
		if(CYIRemoteLogger.verbose) {
			var message = "Console log functions not already hooked, injecting remote logging into application printLog function.";

			if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
				CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
			}

			CYIRemoteLogger.sendDebugRemoteLog(message);
		}

		// assume that the printLog function does not pipe to the standard console logging functions, so pipe its output to the remote log function
		function printLog() {
			var logType = "debug";
			var doNotRemoteLog = false;

			// prevent duplicate logs
			if(arguments.length !== 0 && typeof arguments[0] === "string") {
				var logMessage = arguments[0].trim();

				if(logMessage.indexOf("JavaScript", 2) === 2) {
					doNotRemoteLog = true;
				}

				if(logMessage.indexOf("I/") === 0) {
					logType = "info";
				}
				else if(logMessage.indexOf("W/") === 0) {
					logType = "warning";
				}
				else if(logMessage.indexOf("E/") === 0 || logMessage.indexOf("F/") === 0) {
					logType = "error";
				}
			}

			// invoke the original printLog function
			if(CYIRemoteLogger.originalPrintLogFunctionRequiresTypeArgument) {
				CYIRemoteLogger.originalPrintLogFunction.apply(CYILogger, [logType, arguments]);
			}
			else {
				CYIRemoteLogger.originalPrintLogFunction.apply(CYIRemoteLogger.legacy ? application : CYILogger, arguments);
			}

			if(doNotRemoteLog) {
				return;
			}

			// invoke the remote logging function with the same arguments
			CYIRemoteLogger.remoteLog(logType, Array.prototype.slice.call(arguments));
		}

		if(CYIRemoteLogger.legacy) {
			application.printLog = printLog;
		}
		else {
			CYILogger.log = printLog;
		}
	}
	else {
		if(CYIRemoteLogger.verbose) {
			var message = "Console log functions already hooked, not injecting remote logging into application printLog function.";

			if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
				CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
			}

			CYIRemoteLogger.sendDebugRemoteLog(message);
		}
	}

	CYIRemoteLogger.printLogHooked = true;
};

CYIRemoteLogger.unhookLoggingFunctions = function unhookLoggingFunctions() {
	// do not allow console logging to be unhooked if disabled or not hooked
	if(!CYIRemoteLogger.initialized || !CYIRemoteLogger.enabled || !(CYIRemoteLogger.consoleHooked || CYIRemoteLogger.printLogHooked)) {
		return;
	}

	CYIRemoteLogger.unhookConsoleLogging();

	CYIRemoteLogger.unhookPrintLogFunction();
};

CYIRemoteLogger.unhookPrintLogFunction = function unhookPrintLogFunction() {
	// do not allow console logging to be unhooked if disabled or not hooked
	if(!CYIRemoteLogger.initialized || !CYIRemoteLogger.enabled || !CYIRemoteLogger.printLogHooked || !(CYIRemoteLogger.originalPrintLogFunction instanceof Function)) {
		return;
	}

	// restore original printLog function if it was hooked by the remote logger
	if(CYIRemoteLogger.shouldHookPrintLogFunction) {
		if(CYIRemoteLogger.verbose) {
			var message = "Restoring printLog to original function.";

			if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
				CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
			}

			CYIRemoteLogger.sendDebugRemoteLog(message);
		}

		if(CYIRemoteLogger.legacy) {
			application.printLog = CYIRemoteLogger.originalPrintLogFunction;
		}
		else {
			CYILogger.log = CYIRemoteLogger.originalPrintLogFunction;
		}
	}

	CYIRemoteLogger.printLogHooked = false;
};

CYIRemoteLogger.unhookConsoleLogging = function unhookConsoleLogging() {
	// do not allow console logging to be unhooked if disabled or not hooked
	if(!CYIRemoteLogger.initialized || !CYIRemoteLogger.enabled || !CYIRemoteLogger.consoleHooked) {
		return;
	}

	if(CYIRemoteLogger.verbose) {
		var message = "Unhooking remote logging from console logging functions...";

		if(CYIRemoteLogger.originalPrintLogFunction instanceof Function) {
			CYIRemoteLogger.callOriginalPrintLogFunction("debug", message);
		}

		CYIRemoteLogger.sendDebugRemoteLog(message);
	}

	// restore original console logging functions
	var consoleLogType = null;

	// inject new console logging function wrappers
	for(var i = 0; i < CYIRemoteLogger.consoleLogTypes.length; i++) {
		consoleLogType = CYIRemoteLogger.consoleLogTypes[i];

		console[consoleLogType] = CYIRemoteLogger.modifiedConsoleLogFunctions[consoleLogType];
	}

	CYIRemoteLogger.consoleHooked = false;
};

CYIRemoteLogger.backupOriginalConsoleLogFunctions = function backupOriginalConsoleLogFunctions() {
	// do not allow original console log functions to be backed up if not enabled
	if(!CYIRemoteLogger.enabled) {
		return false;
	}

	var consoleLogType = null;

	// saves references to the original console logging functions
	for(var i = 0; i < CYIRemoteLogger.consoleLogTypes.length; i++) {
		consoleLogType = CYIRemoteLogger.consoleLogTypes[i];

		CYIRemoteLogger.originalConsoleLogFunctions[consoleLogType] = console[consoleLogType];
	}
};

CYIRemoteLogger.backupModifiedConsoleLogFunctions = function backupModifiedConsoleLogFunctions() {
	// do not allow modified console log functions to be backed up if not enabled
	if(!CYIRemoteLogger.enabled) {
		return;
	}

	var consoleLogType = null;

	// saves references to the modified console logging functions
	for(var i = 0; i < CYIRemoteLogger.consoleLogTypes.length; i++) {
		consoleLogType = CYIRemoteLogger.consoleLogTypes[i];

		CYIRemoteLogger.modifiedConsoleLogFunctions[consoleLogType] = console[consoleLogType];
	}
};

CYIRemoteLogger.formatMessage = function formatMessage(args) {
	// make sure arguments is an array or array-like if it is a string
	if(typeof args === "string") {
		args = [args];
	}

	// verifies that arguments is an array
	if(!Array.isArray(args)) {
		return "";
	}

	var formattedMessage = "";

	for(var i = 0; i < args.length; i++) {
		if(formattedMessage.length !== 0) {
			formattedMessage += " ";
		}

		formattedMessage += CYIRemoteLogger.toString(args[i]);
	}

	return formattedMessage.trim();
};

CYIRemoteLogger.toString = function toString(value) {
	if(value === undefined) {
		return "undefined";
	}
	else if(value === null) {
		return "null";
	}
	else if(typeof value === "string") {
		return value;
	}
	else if(value === Infinity) {
		return "Infinity";
	}
	else if(value === -Infinity) {
		return "-Infinity";
	}
	else if(typeof value === "number" && isNaN(value)) {
		return "NaN";
	}
	else if(value instanceof Date) {
		return value.toString();
	}
	else if(value instanceof RegExp) {
		var flags = "";

		for(var flag in regExpFlags) {
			if(value[flag]) {
				flags += regExpFlags[flag];
			}
		}

		return "/" + value.source + "/" + flags;
	}
	else if(value instanceof Function) {
		return value.toString();
	}
	else if(value instanceof Error) {
		if(value.stack !== undefined && value.stack !== null) {
			return value.stack;
		}

		return value.message;
	}

	return JSON.stringify(value);
};

CYIRemoteLogger.callOriginalPrintLogFunction = function callOriginalPrintLogFunction(type, message) {
	if(CYIRemoteLogger.originalPrintLogFunctionRequiresTypeArgument) {
		if(typeof message === "string") {
			message = [message];
		}

		if(!Array.isArray(message)) {
			return;
		}

		CYIRemoteLogger.originalPrintLogFunction(type, message);
	}
	else {
		CYIRemoteLogger.originalPrintLogFunction(message);
	}
};

CYIRemoteLogger.createLog = function createLogData(type, args) {
	if(type === "log") {
		type = "debug";
	}

	// creates the log message data
	return {
		id: CYIRemoteLogger.messageIdCounter++,
		message: CYIRemoteLogger.formatMessage(args),
		type: type,
		timestamp: new Date().getTime(),
		sent: null,
		sendCount: 0
	};
};

CYIRemoteLogger.remoteLog = function remoteLog(type, args) {
	// check that the remote logger is enabled and initialized
	if(!CYIRemoteLogger.enabled || !CYIRemoteLogger.initialized) {
		return;
	}

	// create the log data based on the paremeters
	var logData = CYIRemoteLogger.createLog(type, args);

	if(logData === null) {
		return;
	}

	CYIRemoteLogger.messageQueue.push(logData);

	// determine if log message should be added to the log message queue or sent immediately
	if(!CYIRemoteLogger.batchingEnabled) {
		CYIRemoteLogger.sendRemoteLog(logData);
	}
};

CYIRemoteLogger.sendRemoteLog = function sendRemoteLog(messageData) {
	// check that the remote logger is enabled and initialized
	if(!CYIRemoteLogger.enabled || !CYIRemoteLogger.initialized) {
		return;
	}

	for(var i = 0; i < messageData.length; i++) {
		messageData.sent = new Date().getTime();
		messageData.sendCount++;
	}

	// send immediate single log message request
	return CYIRemoteLogger.sendRequest(
		CYIRemoteLogger.createRequestConfiguration("logger", messageData),
		function(error, data, status) {
			CYIRemoteLogger.processResponse(error, data, status, messageData);
		}
	);
};

CYIRemoteLogger.sendRemoteLogBatch = function sendRemoteLogBatch(batchMessageData) {
	// check that the remote logger is enabled and initialized
	if(!CYIRemoteLogger.enabled || !CYIRemoteLogger.initialized) {
		return;
	}

	// ignore empty or invalid data
	if(!Array.isArray(batchMessageData) || batchMessageData.length === 0) {
		return;
	}

	for(var i = 0; i < batchMessageData.length; i++) {
		batchMessageData[i].sent = new Date().getTime();
		batchMessageData[i].sendCount++;
	}

	if(batchMessageData.length > 500) {
		CYIRemoteLogger.sendDebugRemoteLog("Message batch size exceeds 500 messages, this may overload the request and lock up the remote logging system.", "warning");
	}

	// send batch log message request
	return CYIRemoteLogger.sendRequest(
		CYIRemoteLogger.createRequestConfiguration("logger/batch", batchMessageData),
		function(error, data, status) {
			CYIRemoteLogger.processResponse(error, data, status, batchMessageData);
		}
	);
};

CYIRemoteLogger.createRequestConfiguration = function createRequestConfiguration(path, messageData) {
	return {
		url: "http://" + CYIRemoteLogger.address + ":" + CYIRemoteLogger.port + "/" + path,
		method: "POST",
		timeout: 10000,
		headers: CYIRemoteLogger.metadataHeaders instanceof Object && CYIRemoteLogger.metadataHeaders.constructor === Object ? CYIRemoteLogger.metadataHeaders : { },
		data: messageData
	};
};

CYIRemoteLogger.processResponse = function processResponse(error, data, status, messageData) {
	if(error) {
		return;
	}

	var messageIds = [];

	if(status === 200) {
		if(CYIRemoteLogger.batchingEnabled) {
			messageIds = data.messageIds;
		}
		else {
			messageIds.push(data.messageId);
		}
	}
	else {
		if(CYIRemoteLogger.batchingEnabled) {
			for(var i = 0; i < messageData.length; i++) {
				messageIds.push(messageData[i].id);
			}
		}
		else {
			messageIds.push(messageData.id);
		}
	}

	for(var i = 0; i < messageIds.length; i++) {
		for(var j = 0; j < CYIRemoteLogger.messageQueue.length; j++) {

			if(CYIRemoteLogger.messageQueue[j].id === messageIds[i]) {
				CYIRemoteLogger.messageQueue.splice(j, 1);
				break;
			}
		}
	}
};

CYIRemoteLogger.sendDebugRemoteLog = function sendDebugRemoteLog(message, logType) {
	return CYIRemoteLogger.sendRequest(
		{
			url: "http://" + CYIRemoteLogger.address + ":" + CYIRemoteLogger.port + "/logger",
			method: "POST",
			timeout: 10000,
			data: {
				message: message,
				type: logType || "debug"
			}
		},
		function(error, data, status) { }
	);
};

CYIRemoteLogger.sendRequest = function sendRequest(options, callback) {
	if(!(callback instanceof Object && callback.constructor === Function)) {
		throw new Error("Missing or invalid callback function!");
	}

	if(!(options instanceof Object && options.constructor === Object)) {
		throw new Error("Missing or invalid request options, expected strict object!");
	}

	if(typeof options.method !== "string" || options.method.trim().length === 0) {
		throw new Error("Missing or invalid request method type, expected non-empty string!")
	}

	if(typeof options.url !== "string" || options.url.trim().length === 0) {
		throw new Error("Missing or invalid request url, expected non-empty string!")
	}

	if(options.timeout !== undefined && options.timeout !== null && (!Number.isInteger(options.timeout) || options.timeout < 0)) {
		throw new Error("Missing or invalid request timeout, expected positive integer!")
	}

	if(options.headers !== undefined && options.headers !== null && (!(options.headers instanceof Object) || options.headers.constructor !== Object)) {
		throw new Error("Missing or invalid request headers, expected strict object!")
	}

	var request = new XMLHttpRequest();

	request.open(options.method, options.url, true);

	if(options.headers instanceof Object && options.headers.constructor === Object) {
		var headerName = null;
		var headerNames = Object.keys(options.headers);

		for(var i = 0; i < headerNames.length; i++) {
			headerName = headerNames[i];

			request.setRequestHeader(headerName, options.headers[headerName]);
		}
	}

	request.setRequestHeader("Content-Type", "application/json");

	request.timeout = options.timeout;

	request.addEventListener("load", function(event) {
		if(typeof request.responseText === "string" && request.responseText.length !== 0) {
			var responseJSON = null;

			try {
				responseJSON = JSON.parse(request.responseText);
			}
			catch(error) {
				error.message = "Invalid response data: " + error.message;
				error.status = request.status;

				return callback(error, null, request.status);
			}

			if(responseJSON instanceof Object) {
				if(responseJSON.constructor === Object && responseJSON.error instanceof Object && responseJSON.error.constructor === Object) {
					var message = responseJSON.error.message;

					if(typeof message !== "string" || message.trim().length === 0) {
						message = "Unknown error.";
					}

					var error = new Error(message);

					var attributeName = null;
					var attributeNames = Object.getOwnPropertyNames(responseJSON.error);

					for(var i = 0; i < attributeNames.length; i++) {
						attributeName = attributeNames[i];

						error[attributeName] = responseJSON.error[attributeName];
					}

					error.status = request.status;

					return callback(error, null, request.status);
				}
				else {
					return callback(null, responseJSON, request.status);
				}
			}
		}
		else {
			return callback(null, null, request.status);
		}
	});

	request.addEventListener("error", function(event) {
		var error = new Error("Connection failed!");
		error.status = 0;

		return callback(error, null, error.status);
	});

	if(options.data instanceof Object) {
		request.send(JSON.stringify(options.data));
	}
	else {
		request.send();
	}
};

CYIRemoteLogger.backupOriginalConsoleLogFunctions();

if(typeof CYIMain !== "undefined") {
	CYIRemoteLogger.initialize();
}
else {
	window.addEventListener("DOMContentLoaded", function(event) {
		// initialize the remote logger after the tizen logging hooks have been initialized (this will miss some logs during the initialization process)
		setTimeout(function() {
			CYIRemoteLogger.initialize();
		}, 0);
	});
}

window.addEventListener("error", function(event) {
	if(event instanceof ErrorEvent) {
		var error = event.message;

		if(event.error !== undefined && event.error !== null) {
			error = event.error;
		}

		if(error instanceof Error) {
			CYIRemoteLogger.remoteLog("error", "Unhandled Error: " + error.message);
			CYIRemoteLogger.remoteLog("error", error.stack);
		}
		else {
			CYIRemoteLogger.remoteLog("error", "Unhandled Error: " + event.message);
			CYIRemoteLogger.remoteLog("error", "at " + event.filename + ":" + event.lineno + (isNaN(event.colno) ? "" : event.colno));
		}
	}

	return false;
});

window.addEventListener("unhandledrejection", function(event) {
	if(event instanceof PromiseRejectionEvent) {
		var reason = event.reason;

		if(reason instanceof Object) {
			CYIRemoteLogger.remoteLog("error", "Unhandled Promise Rejection: " + reason.message);
		}
	}

	return false;
});
