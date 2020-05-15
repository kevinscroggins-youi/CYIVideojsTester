#include "IStreamPlanetFairPlayHandler.h"

#include <utility/YiParsingError.h>
#include <utility/YiRapidJSONUtility.h>

#include <regex>

#define LOG_TAG "IStreamPlanetFairPlayHandler"

static const CYIString CERTIFICATE_BASE_URL = "https://%1/api/AppCert/%2";
static const CYIString LICENSE_BASE_URL = "https://%1/api/license/%2";
static const CYIString username = "tizen-test3@mailinator.com";
static const CYIString password = "Test1234!";
static const CYIString apikey = "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ0eXBlIjoiYXBpIiwidWlkIjoiOWQxOWIzZWQtYzA1Zi00ZTBjLWEzMzgtNDE4NzA1MTAwZGEzIiwiYW5vbiI6ZmFsc2UsInBlcm1pc3Npb25zIjpudWxsLCJhcGlLZXkiOiI5ZDE5YjNlZC1jMDVmLTRlMGMtYTMzOC00MTg3MDUxMDBkYTMiLCJleHAiOjE1ODI3NTE4NDEsImlhdCI6MTUxOTY3OTg0MSwiaXNzIjoiT3JiaXMtT0FNLVYxIiwic3ViIjoiOWQxOWIzZWQtYzA1Zi00ZTBjLWEzMzgtNDE4NzA1MTAwZGEzIn0.iLa8Ch4k59Of4UL6mWJwHNeX-YBb4gcfsw46IMmbT9id-n-8Fj3g0Hz9l6d_GIZDz2Hi8OQsB-CFeycQGYBkgQ";

IStreamPlanetFairPlayHandler::IStreamPlanetFairPlayHandler(CYIAbstractVideoPlayer *pPlayer)
    : m_pPlayer(pPlayer)
{
}

std::unique_ptr<CYIFairPlayDRMConfiguration> IStreamPlanetFairPlayHandler::CreateDRMConfiguration()
{
    std::unique_ptr<CYIFairPlayDRMConfiguration> pDRMConfiguration(new CYIFairPlayDRMConfiguration());
    pDRMConfiguration->DRMRequestUrlAvailable.Connect(*this, &IStreamPlanetFairPlayHandler::OnDRMRequestUrlAvailable);
    pDRMConfiguration->SPCMessageAvailable.Connect(*this, &IStreamPlanetFairPlayHandler::OnSPCMessageAvailable);
    return pDRMConfiguration;
}

void IStreamPlanetFairPlayHandler::OnDRMRequestUrlAvailable(const CYIUrl &drmUrl)
{
    CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
    const CYIString drmUrlString = drmUrl.ToString();
    if (!ValidateUrl(drmUrlString))
    {
        YI_LOGE(LOG_TAG, "DRMUrl provided by the player for the media is not a valid IStreamPlanetUrl. Url: %s", drmUrlString.GetData());
        pFairPlayDRMConfiguration->NotifyFailure();
        return;
    }

    m_urlData = ParseUrlData(drmUrlString);
    if (!m_urlData.valid)
    {
        YI_LOGE(LOG_TAG, "DRMUrl was not successfully parsed. Url: %s", drmUrlString.GetData());
        pFairPlayDRMConfiguration->NotifyFailure();
        return;
    }

    const CYIUrl certificateFetchUrl(CERTIFICATE_BASE_URL.Arg(m_urlData.hostUrl).Arg(m_urlData.companyId));
    std::shared_ptr<CYIHTTPRequest> pCertificateFetchRequest(new CYIHTTPRequest(certificateFetchUrl, CYIHTTPRequest::Method::GET));
    pCertificateFetchRequest->NotifyComplete.Connect([this](const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode) {
        CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
        if (pFairPlayDRMConfiguration)
        {
            m_spcMessage = pHTTPResponse->GetBody();
            pFairPlayDRMConfiguration->RequestSPCMessage(m_spcMessage, m_urlData.assetId);
        }
    });
    pCertificateFetchRequest->NotifyError.Connect([this](const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage) {
        YI_LOGE(LOG_TAG, "Failed to request DRM certificate from the server. Url: %s Message: %s", pHTTPRequest->GetURL().ToString().GetData(), errorMessage.GetData());
        CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
        if (pFairPlayDRMConfiguration)
        {
            pFairPlayDRMConfiguration->NotifyFailure();
        }
    });
    CYIHTTPService::GetInstance()->EnqueueRequest(pCertificateFetchRequest);
}

void IStreamPlanetFairPlayHandler::OnSPCMessageAvailable(const CYIString &spcMessage)
{
    m_spcMessage = spcMessage;
    CYIUrl loginurl = CYIUrl("https://platform.stage.dtc.istreamplanet.net/oam/v1/user/tokens");

    std::shared_ptr<CYIHTTPRequest> pLoginRequest(new CYIHTTPRequest(loginurl, CYIHTTPRequest::Method::POST));
    pLoginRequest->AddHeader("Authorization", " Bearer " + apikey);
    pLoginRequest->AddHeader("Content-Type", " application/x-www-form-urlencoded");
    pLoginRequest->SetPostData("{\n  \"username\":\"" + username + "\",\n  \"password\":\"" + password + "\"\n}");
    pLoginRequest->NotifyComplete.Connect(*this, &IStreamPlanetFairPlayHandler::OnLoginRequestCompleted);
    pLoginRequest->NotifyError.Connect(*this, &IStreamPlanetFairPlayHandler::OnLoginRequestFailed);
    CYIHTTPService::GetInstance()->EnqueueRequest(pLoginRequest);
}

void IStreamPlanetFairPlayHandler::OnLoginRequestCompleted(const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode)
{
    CYIParsingError parsingError;
    CYIString sessionToken;
    CYIString uid;
    CYIString assetId = "c0fe1b7d2de4e5b94dc821091e5b2150";
    CYIString playbackUrl = "https://dtv-latam-abc.akamaized.net/hls/live/2003011-b/dtv/dtv-latam-boomerang/master.m3u8";

    std::unique_ptr<yi::rapidjson::Document> pDocumentWithSessionToken(CYIRapidJSONUtility::CreateDocumentFromString(pHTTPResponse->GetBody().GetData(), parsingError));
    CYIRapidJSONUtility::GetStringField(pDocumentWithSessionToken.get(), "sessionToken", sessionToken, parsingError);
    if (parsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "INCORRECT LOGIN, PASSWORD, OR API KEY.");
        return;
    }
    const yi::rapidjson::Value &account = (*pDocumentWithSessionToken)["account"];
    CYIRapidJSONUtility::GetStringField(&account, "uid", uid, parsingError);

    std::shared_ptr<CYIHTTPRequest> pTokenRequest(new CYIHTTPRequest(CYIUrl("https://platform.stage.dtc.istreamplanet.net/oem/v1/user/accounts/" + uid + "/entitlement?tokentype=isp-atlas"), CYIHTTPRequest::Method::POST));
    pTokenRequest->AddHeader("Authorization", " Bearer " + sessionToken);
    pTokenRequest->AddHeader("Content-Type", " application/json");
    pTokenRequest->SetPostData("{ \n  \"assetID\":\"" + assetId + "\",\n  \"playbackUrl\":\"" + playbackUrl + "\"\n}");
    pTokenRequest->NotifyComplete.Connect(*this, &IStreamPlanetFairPlayHandler::OnTokenRequestCompleted);
    pTokenRequest->NotifyError.Connect(*this, &IStreamPlanetFairPlayHandler::OnTokenRequestFailed);
    CYIHTTPService::GetInstance()->EnqueueRequest(pTokenRequest);
}

void IStreamPlanetFairPlayHandler::OnLoginRequestFailed(const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage)
{
    YI_LOGE(LOG_TAG, "Failed to request license (CKC Message) from the server. Url: %s Message: %s.", pHTTPRequest->GetURL().ToString().GetData(), errorMessage.GetData());
    CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
    if (pFairPlayDRMConfiguration)
    {
        pFairPlayDRMConfiguration->NotifyFailure();
    }
}

void IStreamPlanetFairPlayHandler::OnTokenRequestCompleted(const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode)
{
    CYIParsingError parsingError;
    CYIString entitlementToken;

    std::unique_ptr<yi::rapidjson::Document> pDocumentWithEntitlementToken(CYIRapidJSONUtility::CreateDocumentFromString(pHTTPResponse->GetBody().GetData(), parsingError));
    CYIRapidJSONUtility::GetStringField(pDocumentWithEntitlementToken.get(), "entitlementToken", entitlementToken, parsingError);

    const CYIUrl licenseFetchUrl(LICENSE_BASE_URL.Arg(m_urlData.hostUrl).Arg(m_urlData.companyId));
    std::shared_ptr<CYIHTTPRequest> pLicenseFetchRequest(new CYIHTTPRequest(licenseFetchUrl, CYIHTTPRequest::Method::POST));
    pLicenseFetchRequest->AddHeader("x-isp-token ", entitlementToken);
    pLicenseFetchRequest->SetPostData(m_spcMessage);
    pLicenseFetchRequest->NotifyComplete.Connect([this](const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &pHTTPResponse, const CYIHTTPService::HTTPStatusCode) {
        m_ckcMessage = pHTTPResponse->GetBody();
        CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
        if (pFairPlayDRMConfiguration)
        {
            pFairPlayDRMConfiguration->ProvideCKCMessage(m_ckcMessage);
        }
    });

    pLicenseFetchRequest->NotifyError.Connect([this](const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage) {
        YI_LOGE(LOG_TAG, "Failed to request license (CKC Message) from the server. Url: %s Message: %s", pHTTPRequest->GetURL().ToString().GetData(), errorMessage.GetData());
        CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
        if (pFairPlayDRMConfiguration)
        {
            pFairPlayDRMConfiguration->NotifyFailure();
        }
    });
    CYIHTTPService::GetInstance()->EnqueueRequest(pLicenseFetchRequest);
    m_spcMessage = "";
}

void IStreamPlanetFairPlayHandler::OnTokenRequestFailed(const std::shared_ptr<CYIHTTPRequest> &pHTTPRequest, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage)
{
    YI_LOGE(LOG_TAG, "Failed to request license (CKC Message) from the server. Url: %s Message: %s.", pHTTPRequest->GetURL().ToString().GetData(), errorMessage.GetData());
    CYIFairPlayDRMConfiguration *pFairPlayDRMConfiguration = static_cast<CYIFairPlayDRMConfiguration *>(m_pPlayer->GetDRMConfiguration());
    if (pFairPlayDRMConfiguration)
    {
        pFairPlayDRMConfiguration->NotifyFailure();
    }
}

bool IStreamPlanetFairPlayHandler::ValidateUrl(const CYIString &url) const
{
    // create regex which checks if the string matches the given format:
    // 'skd://' + host + '/' + guid (with dashes) + '/' + 40 character hash
    std::regex pattern("^(skd:\\/\\/)+(.*\\/{1})+([a-zA-Z0-9\\-]{36}\\/{1})+([a-zA-Z0-9]{40})$");
    return std::regex_match(url.GetData(), pattern);
}

IStreamPlanetFairPlayHandler::IStreamPlanetUrlData IStreamPlanetFairPlayHandler::ParseUrlData(const CYIString &drmUrl) const
{
    IStreamPlanetUrlData urlData;
    urlData.valid = false;

    std::vector<CYIString> parts = drmUrl.Split("/");
    if (parts.size() >= 4)
    {
        urlData.hostUrl = parts[1];
        urlData.companyId = parts[2];
        urlData.assetId = parts[3];
        urlData.valid = urlData.hostUrl.IsNotEmpty() && urlData.companyId.IsNotEmpty() && urlData.assetId.IsNotEmpty();
    }
    return urlData;
}
