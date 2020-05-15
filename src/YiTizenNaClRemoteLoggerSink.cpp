#include "YiTizenNaClRemoteLoggerSink.h"

#include <platform/YiWebBridgeLocator.h>
#include <utility/YiRapidJSONUtility.h>

#define LOG_TAG "CYITizenNaClRemoteLoggerSink"

static const char *REMOTE_LOGGER_CLASS_NAME = "CYIRemoteLogger";

static CYIWebMessagingBridge::FutureResponse CallTizenRemoteLoggerFunction(yi::rapidjson::Document &&message, const CYIString &functionName, yi::rapidjson::Value &&functionArgumentsValue = yi::rapidjson::Value(yi::rapidjson::kArrayType))
{
	return CYIWebBridgeLocator::GetWebMessagingBridge()->CallStaticFunctionWithArgs(std::move(message), REMOTE_LOGGER_CLASS_NAME, functionName, std::move(functionArgumentsValue));
}

static uint64_t RegisterTizenRemoteLoggerEventHandler(const CYIString &eventName, CYIWebMessagingBridge::EventCallback &&eventCallback)
{
	yi::rapidjson::Document filterDocument(yi::rapidjson::kObjectType);
	yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &filterAllocator = filterDocument.GetAllocator();

	filterDocument.AddMember(yi::rapidjson::StringRef(CYIWebMessagingBridge::EVENT_CONTEXT_ATTRIBUTE_NAME), yi::rapidjson::StringRef(REMOTE_LOGGER_CLASS_NAME), filterAllocator);

	yi::rapidjson::Value eventNameValue(eventName.GetData(), filterAllocator);
	filterDocument.AddMember(yi::rapidjson::StringRef(CYIWebMessagingBridge::EVENT_NAME_ATTRIBUTE_NAME), eventNameValue, filterAllocator);

	return CYIWebBridgeLocator::GetWebMessagingBridge()->RegisterEventHandler(std::move(filterDocument), std::move(eventCallback));
}

static void UnregisterTizenRemoteLoggerEventHandler(uint64_t &eventHandlerId)
{
	CYIWebBridgeLocator::GetWebMessagingBridge()->UnregisterEventHandler(eventHandlerId);
	eventHandlerId = 0;
}

CYITizenNaClRemoteLoggerSink::CYITizenNaClRemoteLoggerSink()
	: m_remoteLoggerInitialized(false)
	, m_remoteLoggerInitializedEventHandlerId(0)
{
	set_pattern("%^%Y-%m-%d %T.%e %P:%t [%n] %L/%s:%#:%!:   %v%$");

	m_remoteLoggerInitializedEventHandlerId = RegisterTizenRemoteLoggerEventHandler("initialized", [this](yi::rapidjson::Document &&) {
		m_remoteLoggerInitialized.store(true);
	});

	yi::rapidjson::Document messageDocument(yi::rapidjson::kObjectType);

	CallTizenRemoteLoggerFunction(std::move(messageDocument), "onLogSinkInitialized");
}

CYITizenNaClRemoteLoggerSink::~CYITizenNaClRemoteLoggerSink() {
	UnregisterTizenRemoteLoggerEventHandler(m_remoteLoggerInitializedEventHandlerId);
}

void CYITizenNaClRemoteLoggerSink::sink_it_(const CYILogMessage &message)
{
	static const CYIString REMOTE_LOG_FUNCTION_NAME("remoteLog");

	CYIString formattedMessage = CYILogSink::FormatMessage(message);
	formattedMessage.TrimRight(); // remove a redundant newline at the end since our JS logger will supply one anyway

	yi::rapidjson::Document messageDocument(yi::rapidjson::kObjectType);
	yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &messageAllocator = messageDocument.GetAllocator();

	CYIString level("debug");

	if(formattedMessage.IndexOf("[Yi] I/") != CYIString::NPos) {
		level = "info";
	}
	else if(formattedMessage.IndexOf("[Yi] W/") != CYIString::NPos) {
		level = "warning";
	}
	else if(formattedMessage.IndexOf("[Yi] E/") != CYIString::NPos || formattedMessage.IndexOf("[Yi] F/") != CYIString::NPos) {
		level = "error";
	}

	yi::rapidjson::Value argsArray(yi::rapidjson::kArrayType);
	yi::rapidjson::Value logLevel(level.GetData(), messageAllocator);
	argsArray.PushBack(logLevel, messageAllocator);

	yi::rapidjson::Value messageArray(yi::rapidjson::kArrayType);
	yi::rapidjson::Value messageValue(formattedMessage.GetData(), messageAllocator);
	messageArray.PushBack(messageValue, messageAllocator);
	argsArray.PushBack(messageArray, messageAllocator);

	CallTizenRemoteLoggerFunction(std::move(messageDocument), REMOTE_LOG_FUNCTION_NAME, std::move(argsArray));
}
