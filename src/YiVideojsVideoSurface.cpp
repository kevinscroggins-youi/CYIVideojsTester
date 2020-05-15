#include "YiVideojsVideoSurface.h"

#include "YiVideojsVideoPlayerPriv.h"

#define LOG_TAG "CYIVideojsVideoSurface"

CYIVideojsVideoSurface::CYIVideojsVideoSurface(CYIVideojsVideoPlayerPriv *pPlayerPriv)
    : CYIVideoSurfacePlatform(CYIVideoSurface::Capabilities::Translate | CYIVideoSurface::Capabilities::Scale)
    , m_pPlayerPriv(pPlayerPriv)
{
}

CYIVideojsVideoSurface::~CYIVideojsVideoSurface()
{
    m_pPlayerPriv = nullptr; // not owned by surface
}

void CYIVideojsVideoSurface::SetVideoRectangle(const YI_RECT_REL &videoRectangle)
{
    m_pPlayerPriv->SetVideoRectangle(videoRectangle);
    SetSize(glm::ivec2(videoRectangle.width, videoRectangle.height));
}

void CYIVideojsVideoSurface::OnAttached(CYIVideoSurfaceView *pVideoSurfaceView)
{
    CYIVideoSurfacePlatform::OnAttached(pVideoSurfaceView);
}

void CYIVideojsVideoSurface::OnDetached(CYIVideoSurfaceView *pVideoSurfaceView)
{
    CYIVideoSurfacePlatform::OnDetached(pVideoSurfaceView);
}
