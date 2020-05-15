#include "AirplayRoutePicker.h"

#include "logging/YiLogger.h"
#import <AVFoundation/AVAudioSession.h>
#import <MediaPlayer/MPVolumeView.h>
#import <apple/YiRootViewController.h>

AirplayRoutePicker::AirplayRoutePicker()
{
    //Since the frame location is updated constantly, in the constructor we can simply set it to 0,0,0,0 as it will change immediately
    m_pVolumeView = [[MPVolumeView alloc] initWithFrame:CGRectMake(0, 0, 0, 0)];
    [m_pVolumeView setShowsVolumeSlider:NO];
    [m_pVolumeView setShowsRouteButton:YES];
    [m_pVolumeView sizeToFit];
    [YiRootViewController.sharedInstance.renderSurfaceViewController.view addSubview:m_pVolumeView];

    //This is required for playback to continue in the background
    AVAudioSession *pAudioSession = [AVAudioSession sharedInstance];
    [pAudioSession setCategory:AVAudioSessionCategoryPlayback error:nil];
}

void AirplayRoutePicker::UpdateLocation(glm::vec3 location)
{
    CGFloat screenScale = [[UIScreen mainScreen] scale];

    float y = location.y / screenScale;
    float x = location.x / screenScale;

    [m_pVolumeView setFrame:CGRectMake(x, y, FRAME_SIZE, FRAME_SIZE)];
}

void AirplayRoutePicker::SetVisible(bool visible)
{
    [m_pVolumeView setShowsRouteButton:visible];
}
