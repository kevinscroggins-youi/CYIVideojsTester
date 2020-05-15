#ifndef ISTREAM_PLANET_FAIRPLAY_HANDLER
#define ISTREAM_PLANET_FAIRPLAY_HANDLER

#include <network/YiHTTPRequest.h>
#include <network/YiHTTPService.h>
#include <player/YiFairPlayDRMConfiguration.h>
#include <signal/YiSignalHandler.h>

class IStreamPlanetFairPlayHandler : public CYISignalHandler
{
public:
    IStreamPlanetFairPlayHandler(CYIAbstractVideoPlayer *pPlayer);
    std::unique_ptr<CYIFairPlayDRMConfiguration> CreateDRMConfiguration();

private:
    struct IStreamPlanetUrlData
    {
        CYIString hostUrl;
        CYIString companyId;
        CYIString assetId;
        bool valid;
    };

    void OnDRMRequestUrlAvailable(const CYIUrl &drmUrl);
    void OnSPCMessageAvailable(const CYIString &spcMessage);
    bool ValidateUrl(const CYIString &url) const;
    IStreamPlanetUrlData ParseUrlData(const CYIString &drmUrl) const;
    void FetchCertificate(IStreamPlanetUrlData urlData) const;
    void OnLoginRequestCompleted(const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode);
    void OnLoginRequestFailed(const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage);
    void OnTokenRequestCompleted(const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode);
    void OnTokenRequestFailed(const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage);

    CYIAbstractVideoPlayer *m_pPlayer;
    IStreamPlanetUrlData m_urlData;
    CYIString m_spcMessage;
    CYIString m_ckcMessage;
};
#endif // ISTREAM_PLANET_FAIRPLAY_HANDLER
