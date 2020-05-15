#ifndef _YI_VIDEOJS_VIDEO_SURFACE_H_
#define _YI_VIDEOJS_VIDEO_SURFACE_H_

#include <player/YiVideoSurfacePlatform.h>

class CYIVideojsVideoPlayerPriv;

class CYIVideojsVideoSurface : public CYIVideoSurfacePlatform
{
public:
    CYIVideojsVideoSurface(CYIVideojsVideoPlayerPriv *pPlayerPriv);
    virtual ~CYIVideojsVideoSurface();

protected:
    virtual void SetVideoRectangle(const YI_RECT_REL &videoRectangle) override;
    virtual void OnAttached(CYIVideoSurfaceView *pVideoSurfaceView) override;
    virtual void OnDetached(CYIVideoSurfaceView *pVideoSurfaceView) override;

private:
    CYIVideojsVideoPlayerPriv *m_pPlayerPriv;
};

#endif // _YI_VIDEOJS_VIDEO_SURFACE_H_
