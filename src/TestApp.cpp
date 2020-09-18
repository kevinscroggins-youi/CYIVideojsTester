#include "TestApp.h"

#include <framework/YiAppContext.h>

YI_TYPE_DEF(TestApp);

TestApp::TestApp()
    : m_pMasterApp(nullptr)
{
}

TestApp::~TestApp()
{
}

void TestApp::InitForMasterApp(CYIApp *pMasterApp)
{
    SetAssetsPath(pMasterApp->GetAssetsPath());
    SetDataPath(pMasterApp->GetDataPath());
    SetExternalPath(pMasterApp->GetExternalPath());

    // MasterApp conforms to TestApp settings
    // This allows queries from various widgets in the dev panel
    // to get the right information through MasterApp
    pMasterApp->SetHUDVisibility(this->IsHUDVisible());
    pMasterApp->SetRenderingThrottling(this->IsRenderingThrottled());

    m_pMasterApp = pMasterApp;
    // Set context app back to the master app because instancing the sub app will override this.
    CYIAppContext::GetInstance()->SetApp(m_pMasterApp);

    UserInit();
    UserStart();
}

void TestApp::CleanUpForMasterApp()
{
}

CYIApp *TestApp::GetMasterApp()
{
    if (m_pMasterApp)
    {
        return m_pMasterApp;
    }
    else
    {
        return this;
    }
}

CYISceneManager *TestApp::GetMasterAppSceneManager() const
{
    if (m_pMasterApp)
    {
        return m_pMasterApp->GetSceneManager();
    }
    else
    {
        return GetSceneManager();
    }
}
