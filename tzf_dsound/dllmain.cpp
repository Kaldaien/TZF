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
      config.audio.channels          = 6;
      config.audio.sample_hz         = 44100;
      config.audio.compatibility     = false;
      config.audio.enable_fix        = true;

      config.framerate.stutter_fix   = true;
      config.framerate.fudge_factor  = 2.0f;
      config.framerate.allow_fake_sleep = true;

      config.file_io.capture         = false;

      config.steam.allow_broadcasts  = false;

      config.render.aspect_ratio     = 1.777778f;
      config.render.fovy             = 0.785398f;

      config.render.aspect_addr      = 0x00D52398;
      config.render.fovy_addr        = 0x00D5239C;
      config.render.trilinear_filter = true;

      // Save a new config if none exists
      TZF_SaveConfig ();
    }

    command.AddVariable ("FudgeFactor",    new eTB_VarStub <float> (&config.framerate.fudge_factor));
    command.AddVariable ("AllowFakeSleep", new eTB_VarStub <bool>  (&config.framerate.allow_fake_sleep));

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