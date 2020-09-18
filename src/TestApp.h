#ifndef _TEST_APP_H_
#define _TEST_APP_H_

#include <framework/YiApp.h>
#include <utility/YiRtti.h>

class TestApp : public CYIApp
{
public:
    TestApp();
    virtual ~TestApp();

    using CYIApp::UserUpdate;

    void InitForMasterApp(CYIApp *pMasterApp);
    void CleanUpForMasterApp();

    CYIApp *GetMasterApp();
    CYISceneManager *GetMasterAppSceneManager() const;

private:
    CYIApp *m_pMasterApp;
    YI_TYPE_BASES(TestApp);
};

#endif
