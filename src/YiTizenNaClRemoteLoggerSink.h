#ifndef _YI_TIZEN_NACL_REMOTE_LOGGER_SINK_H_
#define _YI_TIZEN_NACL_REMOTE_LOGGER_SINK_H_

#include <logging/YiLogSink.h>

class CYITizenNaClRemoteLoggerSink : public CYILogSink
{
public:
	CYITizenNaClRemoteLoggerSink();
	virtual ~CYITizenNaClRemoteLoggerSink();

protected:
	virtual void sink_it_(const CYILogMessage &message) override;

private:
	std::atomic_bool m_remoteLoggerInitialized;
	uint64_t m_remoteLoggerInitializedEventHandlerId;
};

#endif // _YI_TIZEN_NACL_REMOTE_LOGGER_SINK_H_
