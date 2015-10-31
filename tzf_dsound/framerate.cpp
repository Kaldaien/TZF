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

#include "framerate.h"
#include "config.h"
#include "log.h"
#include "hook.h"

bool stutter_fix_installed = false;

static __declspec (thread) int           last_sleep     =   1;
static __declspec (thread) LARGE_INTEGER last_perfCount = { 0 };


typedef MMRESULT (WINAPI *timeBeginPeriod_t)(UINT uPeriod);
timeBeginPeriod_t timeBeginPeriod_Original = nullptr;

MMRESULT
WINAPI
timeBeginPeriod_Detour (UINT uPeriod)
{
  dll_log.Log ( L"[!] timeBeginPeriod (%d) - "
                L"[Calling Thread: 0x%04x]",
                  uPeriod,
                    GetCurrentThreadId () );


  return timeBeginPeriod_Original (uPeriod);
}

// We need to use TLS for this, I would imagine...


typedef void (WINAPI *Sleep_t)(DWORD dwMilliseconds);
Sleep_t Sleep_Original = nullptr;

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  last_sleep = dwMilliseconds;

  if (dwMilliseconds != 0 || config.framerate.allow_fake_sleep)
    Sleep_Original (dwMilliseconds);
}

typedef BOOL (WINAPI *QueryPerformanceCounter_t)(_Out_ LARGE_INTEGER *lpPerformanceCount);
QueryPerformanceCounter_t QueryPerformanceCounter_Original = nullptr;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  if (last_sleep != 0) {
    BOOL ret = QueryPerformanceCounter_Original (lpPerformanceCount);
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    return ret;
  } else {
    //LARGE_INTEGER freq;
    //QueryPerformanceFrequency (&freq);

    last_sleep = -1;
    BOOL ret = QueryPerformanceCounter_Original (lpPerformanceCount);
    // Double Speed - May be necessary, but simply removing Sleep (0) does most of the work here.
    lpPerformanceCount->QuadPart += (lpPerformanceCount->QuadPart - last_perfCount.QuadPart) * 
                                     config.framerate.fudget_factor/* * freq.QuadPart*/;
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    return ret;
  }
}


LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

void
tzf::FrameRateFix::Init (void)
{
  if (! config.framerate.stutter_fix)
    return;

  TZF_CreateDLLHook ( L"kernel32.dll", "QueryPerformanceCounter",
                      QueryPerformanceCounter_Detour, 
           (LPVOID *)&QueryPerformanceCounter_Original,
           (LPVOID *)&pfnQueryPerformanceCounter );

  TZF_CreateDLLHook ( L"kernel32.dll", "Sleep",
                      Sleep_Detour, 
           (LPVOID *)&Sleep_Original,
           (LPVOID *)&pfnSleep );

  stutter_fix_installed = true;
}

void
tzf::FrameRateFix::Shutdown(void)
{
  if (! config.framerate.stutter_fix)
    return;

  TZF_DisableHook (pfnQueryPerformanceCounter);
  TZF_DisableHook (pfnSleep);
}