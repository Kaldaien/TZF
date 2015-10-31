#include <Windows.h>

#include "steam.h"
#include "log.h"
#include "config.h"
#include "hook.h"

#include <stdint.h>

typedef uint32_t AppId_t;

class ISteamVideo
{
public:

  // Get a URL suitable for streaming the given Video app ID's video
  virtual void GetVideoURL( AppId_t unVideoAppID ) = 0;

  // returns true if user is uploading a live broadcast
  virtual bool IsBroadcasting( int *pnNumViewers ) = 0;
};

enum { k_iClientVideoCallbacks = 4600 };

#define k_iClientVideo_BroadcastUploadStart (k_iClientVideoCallbacks + 4)

class SteamVideoFake : public ISteamVideo {
public:
  SteamVideoFake (ISteamVideo* real) : real_ (real) { };

  // Get a URL suitable for streaming the given Video app ID's video
  virtual void GetVideoURL( AppId_t unVideoAppID ) {
    return real_->GetVideoURL (unVideoAppID);
  }

  // returns true if user is uploading a live broadcast
  virtual bool IsBroadcasting( int *pnNumViewers ) {
    *pnNumViewers = 0;
    return false;
  }

protected:
private:
  ISteamVideo* real_;
};

SteamVideoFake* faker = nullptr;

#define S_CALLTYPE __cdecl

typedef ISteamVideo* (S_CALLTYPE *SteamVideo_t)(void);

LPVOID       SteamVideo          = nullptr;
SteamVideo_t SteamVideo_Original = nullptr;

ISteamVideo*
S_CALLTYPE
SteamVideo_Detour (void)
{
  ISteamVideo* pVideo = SteamVideo_Original ();

  int x;
  if (pVideo != nullptr && pVideo->IsBroadcasting (&x) == true) {
    if (faker != nullptr) {
      delete faker;
    }

    faker = new SteamVideoFake (pVideo);

    return faker;
  }

  return pVideo;
}


void
tzf::SteamFix::Init (void)
{
  if (! config.steam.allow_broadcasts)
    return;

  TZF_CreateDLLHook ( L"steam_api.dll", "SteamVideo",
                      SteamVideo_Detour,
           (LPVOID *)&SteamVideo_Original,
           (LPVOID *)&SteamVideo );

  TZF_EnableHook (SteamVideo);
}

void
tzf::SteamFix::Shutdown (void)
{
  if (! config.steam.allow_broadcasts)
    return;

  TZF_RemoveHook (SteamVideo);
}