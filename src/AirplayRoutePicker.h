#ifndef _AIRPLAY_ROUTE_PICKER_H_
#define _AIRPLAY_ROUTE_PICKER_H_

#import <framework/YiFramework.h>

#if __OBJC__
@class MPVolumeView;
#else
class MPVolumeView;
#endif
class AirplayRoutePicker
{
public:
    AirplayRoutePicker();
    void SetVisible(bool visible);
    void UpdateLocation(glm::vec3 location);

private:
    MPVolumeView *m_pVolumeView;
    static constexpr int FRAME_SIZE = 20;
};
#endif
