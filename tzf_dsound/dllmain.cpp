/**
 * This file is part of Tales of Zestiria "Fix".
 *
 * Tales of Zestiria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Zestiria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Zestiria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <Windows.h>

#include "config.h"
#include "log.h"

#include "sound.h"
#include "framerate.h"
#include "general_io.h"
#include "steam.h"
#include "render.h"

#include "command.h"

#include "hook.h"

#pragma comment (lib, "kernel32.lib")

DWORD
WINAPI DllThread (LPVOID user)
{
  if (TZF_Init_MinHook() == MH_OK) {
    tzf::SoundFix::Init     ();
    tzf::FrameRateFix::Init ();
    tzf::FileIO::Init       ();
    tzf::SteamFix::Init     ();
    tzf::RenderFix::Init    ();

    TZF_EnableHook (MH_ALL_HOOKS);
  }

  return 0;
}


HMODULE hDLLMod = { 0 };

BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD    ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
  {
    hDLLMod = hModule;

    dll_log.init ("logs/tzfix.log", "w");
    dll_log.Log  (L"tzfix.log created");

    if (! TZF_LoadConfig ()) {
      config.audio.channels             = 8;
      config.audio.sample_hz            = 48000;
      config.audio.compatibility        = false;
      config.audio.enable_fix           = true;

      config.framerate.stutter_fix      = true;
      config.framerate.fudge_factor     = 1.666f;
      config.framerate.allow_fake_sleep = false;
      config.framerate.yield_processor  = true;
      config.framerate.minimize_latency = false;

      config.file_io.capture            = false;

      config.steam.allow_broadcasts     = false;

      config.render.aspect_ratio        = 1.777778f;
      config.render.fovy                = 0.785398f;

      config.render.aspect_addr         = 0x00D52398;
      config.render.fovy_addr           = 0x00D5239C;
      config.render.blackbar_videos     = true;
      config.render.aspect_correction   = false;
      config.render.complete_mipmaps    = false;
      config.render.postproc_ratio      =  1.0f;
      config.render.shadow_rescale      = -2;
      config.render.disable_scissor     = false;

      // Save a new config if none exists
      TZF_SaveConfig ();
    }



    CreateThread (NULL, NULL, DllThread, 0, 0, NULL);
  } break;

  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    tzf::SoundFix::Shutdown     ();
    tzf::FrameRateFix::Shutdown ();
    tzf::FileIO::Shutdown       ();
    tzf::SteamFix::Shutdown     ();
    tzf::RenderFix::Shutdown    ();

    TZF_UnInit_MinHook ();
    TZF_SaveConfig     ();

    dll_log.LogEx      (true,  L"Closing log file... ");
    dll_log.close      ();
    dll_log.LogEx      (false, L"done!");
    break;
  }

  return TRUE;
}