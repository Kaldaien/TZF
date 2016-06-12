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

#define _CRT_SECURE_NO_WARNINGS

#include <string>

#include "hook.h"
#include "log.h"
#include "command.h"
#include "sound.h"
#include "steam.h"

#include "framerate.h"
#include "render.h"

#include <d3d9.h>

#include "config.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>

typedef SHORT (WINAPI *GetAsyncKeyState_pfn)(
  _In_ int vKey
);

typedef UINT (WINAPI *GetRawInputData_pfn)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

typedef BOOL (WINAPI *GetCursorInfo_pfn)
  (_Inout_ PCURSORINFO pci);

typedef BOOL (WINAPI *GetCursorPos_pfn)
  (_Out_ LPPOINT lpPoint);

GetCursorInfo_pfn    GetCursorInfo_Original    = nullptr;
GetCursorPos_pfn     GetCursorPos_Original     = nullptr;
GetAsyncKeyState_pfn GetAsyncKeyState_Original = nullptr;
GetRawInputData_pfn  GetRawInputData_Original  = nullptr;

BOOL WINAPI GetCursorInfo_Detour (_Inout_ PCURSORINFO pci);
BOOL WINAPI GetCursorPos_Detour  (_Out_   LPPOINT     lpPoint);

struct window_t {
  DWORD proc_id;
  HWND  root;
};

BOOL
CALLBACK
TZF_EnumWindows (HWND hWnd, LPARAM lParam)
{
  window_t& win = *(window_t*)lParam;

  DWORD proc_id = 0;

  GetWindowThreadProcessId (hWnd, &proc_id);

  if (win.proc_id != proc_id) {
    if (GetWindow (hWnd, GW_OWNER) != (HWND)nullptr ||
        GetWindowTextLength (hWnd) < 30             ||
     (! IsWindowVisible     (hWnd)))
      return TRUE;
  }

  win.root = hWnd;
  return FALSE;
}

HWND
TZF_FindRootWindow (DWORD proc_id)
{
  window_t win;

  win.proc_id  = proc_id;
  win.root     = 0;

  EnumWindows (TZF_EnumWindows, (LPARAM)&win);

  return win.root;
}


// Returns the original cursor position and stores the new one in pPoint
POINT
CalcCursorPos (LPPOINT pPoint)
{
  float xscale, yscale;
  float xoff,   yoff;

  extern void TZF_ComputeAspectCoeffs ( float& xscale,
                                        float& yscale,
                                        float& xoff,
                                        float& yoff );

  TZF_ComputeAspectCoeffs (xscale, yscale, xoff, yoff);

  pPoint->x = (pPoint->x - xoff) * xscale;
  pPoint->y = (pPoint->y - yoff) * yscale;

  return *pPoint;
}


class TZF_InputHooker
{
private:
  HANDLE                  hMsgPump;
  struct hooks_t {
    HHOOK                 keyboard;
    HHOOK                 mouse;
  } hooks;

  static TZF_InputHooker* pInputHook;

  static char                text [16384];

  static BYTE keys_ [256];
  static bool visible;

  static bool command_issued;
  static std::string result_str;

  struct command_history_t {
    std::vector <std::string> history;
    int_fast32_t              idx     = -1;
  } static commands;

protected:
  TZF_InputHooker (void) { }

public:
  static TZF_InputHooker* getInstance (void)
  {
    if (pInputHook == NULL)
      pInputHook = new TZF_InputHooker ();

    return pInputHook;
  }

  void Start (void)
  {
    hMsgPump =
      CreateThread ( NULL,
                       NULL,
                         TZF_InputHooker::MessagePump,
                           &hooks,
                             NULL,
                               NULL );

    TZF_CreateDLLHook ( L"user32.dll", "GetCursorInfo",
                        GetCursorInfo_Detour,
              (LPVOID*)&GetCursorInfo_Original );

    TZF_CreateDLLHook ( L"user32.dll", "GetCursorPos",
                        GetCursorPos_Detour,
              (LPVOID*)&GetCursorPos_Original );
  }

  void End (void)
  {
    TerminateThread     (hMsgPump, 0);
    UnhookWindowsHookEx (hooks.keyboard);
    UnhookWindowsHookEx (hooks.mouse);
  }

  void Draw (void)
  {
    typedef BOOL (__stdcall *SK_DrawExternalOSD_pfn)(std::string app_name, std::string text);

    static HMODULE               hMod =
      GetModuleHandle (L"d3d9.dll");
    static SK_DrawExternalOSD_pfn SK_DrawExternalOSD
      =
      (SK_DrawExternalOSD_pfn)GetProcAddress (hMod, "SK_DrawExternalOSD");

    std::string output;

    static DWORD last_time = timeGetTime ();
    static bool  carret    = true;

    if (visible) {
      output += text;

      // Blink the Carret
      if (timeGetTime () - last_time > 333) {
        carret = ! carret;

        last_time = timeGetTime ();
      }

      if (carret)
        output += "-";

      // Show Command Results
      if (command_issued) {
        output += "\n";
        output += result_str;
      }
    }

    SK_DrawExternalOSD ("ToZ Fix", output.c_str ());
  }

  HANDLE GetThread (void)
  {
    return hMsgPump;
  }

  bool isVisible (void) { return visible; }

  static DWORD
  WINAPI
  MessagePump (LPVOID hook_ptr)
  {
    hooks_t* pHooks = (hooks_t *)hook_ptr;

    ZeroMemory (text, 16384);

    text [0] = '>';

    extern    HMODULE hDLLMod;

    DWORD dwThreadId;

    int hits = 0;

    DWORD dwTime = timeGetTime ();

    while (true) {
      // Spin until the game has a render window setup and various
      //   other resources loaded
      if (! tzf::RenderFix::pDevice) {
        Sleep (83);
        continue;
      }

      DWORD dwProc;

      dwThreadId =
        GetWindowThreadProcessId (tzf::RenderFix::hWndDevice, &dwProc);

      // Ugly hack, but a different window might be in the foreground...
      if (dwProc != GetCurrentProcessId ()) {
        //dll_log.Log (L"[Input Hook] *** Tried to hook the wrong process!!!");
        Sleep (83);
        continue;
      }

      break;
    }

    dll_log.Log ( L"[Input Hook]  # Found window in %03.01f seconds, "
                     L"installing keyboard hook...",
                   (float)(timeGetTime () - dwTime) / 1000.0f );

    dwTime = timeGetTime ();
    hits   = 1;

    while (! (pHooks->keyboard = SetWindowsHookEx ( WH_KEYBOARD,
                                                      KeyboardProc,
                                                        hDLLMod,
                                                          dwThreadId ))) {
      _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

      dll_log.Log ( L"[Input Hook]  @ SetWindowsHookEx failed: 0x%04X (%s)",
                    err.WCode (), err.ErrorMessage () );

      ++hits;

      if (hits >= 5) {
        dll_log.Log ( L"[Input Hook]  * Failed to install keyboard hook after %lu tries... "
          L"bailing out!",
          hits );
        return 0;
      }

      Sleep (1);
    }

    while (! (pHooks->mouse = SetWindowsHookEx ( WH_MOUSE,
                                                   MouseProc,
                                                     hDLLMod,
                                                       dwThreadId ))) {
      _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

      dll_log.Log ( L"[Input Hook]  @ SetWindowsHookEx failed: 0x%04X (%s)",
                    err.WCode (), err.ErrorMessage () );

      ++hits;

      if (hits >= 5) {
        dll_log.Log ( L"[Input Hook]  * Failed to install mouse hook after %lu tries... "
          L"bailing out!",
          hits );
        return 0;
      }

      Sleep (1);
    }

    dll_log.Log ( L"[Input Hook]  * Installed keyboard hook for command console... "
                        L"%lu %s (%lu ms!)",
                  hits,
                    hits > 1 ? L"tries" : L"try",
                      timeGetTime () - dwTime );

    while (true) {
      Sleep (10);
    }
    //193 - 199

    return 0;
  }

  static LRESULT
  CALLBACK
  MouseProc (int nCode, WPARAM wParam, LPARAM lParam)
  {
    MOUSEHOOKSTRUCT* pmh = (MOUSEHOOKSTRUCT *)lParam;

#if 0
    static bool fudging = false;

    if (game_state.hasFixedAspect () && tzf::RenderFix::pDevice != nullptr && (! fudging)) {
      GetCursorPos (&pmh->pt);

      extern POINT CalcCursorPos (LPPOINT pPos);
      extern POINT real_cursor;

      POINT adjusted_cursor;
      adjusted_cursor.x = pmh->pt.x;
      adjusted_cursor.y = pmh->pt.y;

      real_cursor = CalcCursorPos (&adjusted_cursor);

      pmh->pt.x = adjusted_cursor.x;
      pmh->pt.y = adjusted_cursor.y;

      //SetCursorPos (adjusted_cursor.x, adjusted_cursor.y);

      //tzf::RenderFix::pDevice->SetCursorPosition (adjusted_cursor.x,
                                                   //adjusted_cursor.y,
                                                    //0);//D3DCURSOR_IMMEDIATE_UPDATE);
      fudging = true;
      SendMessage (pmh->hwnd, WM_MOUSEMOVE, 0, LOWORD (pmh->pt.x) | HIWORD (pmh->pt.y));
      return 1;
    }
    else {
      fudging = false;
    }
#endif

    return CallNextHookEx (TZF_InputHooker::getInstance ()->hooks.mouse, nCode, wParam, lParam);
  }

  static LRESULT
  CALLBACK
  KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
  {
    typedef BOOL (__stdcall *SK_DrawExternalOSD_pfn)(std::string app_name, std::string text);

    static HMODULE               hMod =
      GetModuleHandle (config.system.injector.c_str ());
    static SK_DrawExternalOSD_pfn SK_DrawExternalOSD
                                      =
      (SK_DrawExternalOSD_pfn)GetProcAddress (hMod, "SK_DrawExternalOSD");

    if (nCode == 0) {
      BYTE    vkCode   = LOWORD (wParam) & 0xFF;
      BYTE    scanCode = HIWORD (lParam) & 0x7F;
      bool    repeated = LOWORD (lParam);
      bool    keyDown  = ! (lParam & 0x80000000);

      if (visible && vkCode == VK_BACK) {
        if (keyDown) {
          size_t len = strlen (text);
                 len--;

          if (len < 1)
            len = 1;

          text [len] = '\0';
        }
      }

      else if ((vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)) {
        if (keyDown) keys_ [VK_SHIFT] = 0x81; else keys_ [VK_SHIFT] = 0x00;
      }

      else if ((!repeated) && vkCode == VK_CAPITAL) {
        if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x81; else keys_ [VK_CAPITAL] = 0x00;
      }

      else if ((vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)) {
        if (keyDown) keys_ [VK_CONTROL] = 0x81; else keys_ [VK_CONTROL] = 0x00;
      }

      else if ((vkCode == VK_UP) || (vkCode == VK_DOWN)) {
        if (keyDown && visible) {
          if (vkCode == VK_UP)
            commands.idx--;
          else
            commands.idx++;

          // Clamp the index
          if (commands.idx < 0)
            commands.idx = 0;
          else if (commands.idx >= commands.history.size ())
            commands.idx = commands.history.size () - 1;

          if (commands.history.size ()) {
            strcpy (&text [1], commands.history [commands.idx].c_str ());
            command_issued = false;
          }
        }
      }

      else if (visible && vkCode == VK_RETURN) {
        if (keyDown && LOWORD (lParam) < 2) {
          size_t len = strlen (text+1);
          // Don't process empty or pure whitespace command lines
          if (len > 0 && strspn (text+1, " ") != len) {
            eTB_CommandResult result = command.ProcessCommandLine (text+1);

            if (result.getStatus ()) {
              // Don't repeat the same command over and over
              if (commands.history.size () == 0 ||
                  commands.history.back () != &text [1]) {
                commands.history.push_back (&text [1]);
              }

              commands.idx = commands.history.size ();

              text [1] = '\0';

              command_issued = true;
            }
            else {
              command_issued = false;
            }

            result_str = result.getWord   () + std::string (" ")   +
                         result.getArgs   () + std::string (":  ") +
                         result.getResult ();
          }
        }
      }

      else if (keyDown) {
        bool new_press = keys_ [vkCode] != 0x81;

        keys_ [vkCode] = 0x81;

        if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT]) {
          if (keys_ [VK_TAB] && new_press) {
            visible = ! visible;
            tzf::SteamFix::SetOverlayState (visible);

            return -1;
          }

          else if (keys_ ['1'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 60");
            command.ProcessCommandLine ("BattleFPS 60");
            command.ProcessCommandLine ("CutsceneFPS 60");
            command.ProcessCommandLine ("TickScale 1");
          }
          else if (keys_ ['2'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 30");
            command.ProcessCommandLine ("BattleFPS 30");
            command.ProcessCommandLine ("CutsceneFPS 30");
            command.ProcessCommandLine ("TickScale 2");
          }
          else if (keys_ ['3'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 20");
            command.ProcessCommandLine ("BattleFPS 20");
            command.ProcessCommandLine ("CutsceneFPS 20");
            command.ProcessCommandLine ("TickScale 3");
          }
          else if (keys_ ['4'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 15");
            command.ProcessCommandLine ("BattleFPS 15");
            command.ProcessCommandLine ("CutsceneFPS 15");
            command.ProcessCommandLine ("TickScale 4");
          }
          else if (keys_ ['5'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 12");
            command.ProcessCommandLine ("BattleFPS 12");
            command.ProcessCommandLine ("CutsceneFPS 12");
            command.ProcessCommandLine ("TickScale 5");
          }
          else if (keys_ ['6'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust false");
            command.ProcessCommandLine ("TargetFPS 10");
            command.ProcessCommandLine ("BattleFPS 10");
            command.ProcessCommandLine ("CutsceneFPS 10");
            command.ProcessCommandLine ("TickScale 6");
          }
          else if (keys_ ['9'] && new_press) {
            command.ProcessCommandLine ("AutoAdjust true");
            command.ProcessCommandLine ("TargetFPS 60");
            command.ProcessCommandLine ("BattleFPS 60");
            command.ProcessCommandLine ("CutsceneFPS 60");
            command.ProcessCommandLine ("TickScale 1");
          }
          else if (keys_ [VK_OEM_PERIOD]) {
            command.ProcessCommandLine ("AutoAdjust true");
            command.ProcessCommandLine ("TickScale 30");
          }
        }

        if (visible) {
          char key_str [2];
          key_str [1] = '\0';

          if (1 == ToAsciiEx ( vkCode,
                                scanCode,
                                keys_,
                              (LPWORD)key_str,
                                0,
                                GetKeyboardLayout (0) ) &&
                                  isprint ( *key_str ) ) {
            strncat (text, key_str, 1);
            command_issued = false;
          }
        }
      }

      else if ((! keyDown))
        keys_ [vkCode] = 0x00;

      if (visible) return -1;
    }

    return CallNextHookEx (TZF_InputHooker::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
  };
};



WNDPROC original_wndproc = nullptr;

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  int size = GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

  // Block keyboard input to the game while the console is active
  if (TZF_InputHooker::getInstance ()->isVisible () && uiCommand == RID_INPUT) {
    *pcbSize = 0;
    return 0;
  }

  return size;
}

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
#define TZF_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (TZF_InputHooker::getInstance ()->isVisible ()) {
    TZF_ConsumeVKey (vKey);
  }

  return GetAsyncKeyState_Original (vKey);
}


LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  bool console_visible =
    TZF_InputHooker::getInstance ()->isVisible ();

  if (console_visible) {
    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
    // Block RAW Input
    if (uMsg == WM_INPUT)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
    static POINT last_p = { LONG_MIN, LONG_MIN };

    POINT p;

    p.x = MAKEPOINTS (lParam).x;
    p.y = MAKEPOINTS (lParam).y;

    if (game_state.needsFixedMouseCoords () && config.render.aspect_correction) {
      // Only do this if cursor actually moved!
      //
      //   Otherwise, it tricks the game into thinking the input device changed
      //     from gamepad to mouse (and changes button icons).
      if (last_p.x != p.x || last_p.y != p.y) {
        CalcCursorPos (&p);

        last_p = p;
      }

      return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, MAKELPARAM (p.x, p.y));
    }

    last_p = p;
  }

  return CallWindowProc (original_wndproc, hWnd, uMsg, wParam, lParam);
}


BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  BOOL ret = GetCursorInfo_Original (pci);

  // Correct the cursor position for Aspect Ratio
  if (game_state.needsFixedMouseCoords () && config.render.aspect_correction) {
    POINT pt;

    pt.x = pci->ptScreenPos.x;
    pt.y = pci->ptScreenPos.y;

    CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (game_state.needsFixedMouseCoords () && config.render.aspect_correction)
    CalcCursorPos (lpPoint);

  // Defer initialization of the Window Message redirection stuff until
  //   the first time the game calls GetCursorPos (...)
  if (original_wndproc == nullptr && tzf::RenderFix::hWndDevice != NULL) {
    original_wndproc =
      (WNDPROC)GetWindowLong (tzf::RenderFix::hWndDevice, GWL_WNDPROC);

    SetWindowLong ( tzf::RenderFix::hWndDevice,
                      GWL_WNDPROC,
                        (LONG)DetourWindowProc );

    TZF_CreateDLLHook ( L"user32.dll", "GetRawInputData",
                        GetRawInputData_Detour,
              (LPVOID*)&GetRawInputData_Original );

    TZF_CreateDLLHook ( L"user32.dll", "GetAsyncKeyState",
                        GetAsyncKeyState_Detour,
              (LPVOID*)&GetAsyncKeyState_Original );
  }

  return ret;
}

MH_STATUS
WINAPI
TZF_CreateFuncHook ( LPCWSTR pwszFuncName,
                     LPVOID  pTarget,
                     LPVOID  pDetour,
                     LPVOID *ppOriginal )
{
  static HMODULE hParent = GetModuleHandle (L"d3d9.dll");

  typedef MH_STATUS (WINAPI *SK_CreateFuncHook_pfn)
      ( LPCWSTR pwszFuncName, LPVOID  pTarget,
        LPVOID  pDetour,      LPVOID *ppOriginal );
  static SK_CreateFuncHook_pfn SK_CreateFuncHook =
    (SK_CreateFuncHook_pfn)GetProcAddress (hParent, "SK_CreateFuncHook");

  return
    SK_CreateFuncHook (pwszFuncName, pTarget, pDetour, ppOriginal);
}

MH_STATUS
WINAPI
TZF_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr )
{
  static HMODULE hParent = GetModuleHandle (L"d3d9.dll");

  typedef MH_STATUS (WINAPI *SK_CreateDLLHook_pfn)(
        LPCWSTR pwszModule, LPCSTR  pszProcName,
        LPVOID  pDetour,    LPVOID *ppOriginal, 
        LPVOID *ppFuncAddr );
  static SK_CreateDLLHook_pfn SK_CreateDLLHook =
    (SK_CreateDLLHook_pfn)GetProcAddress (hParent, "SK_CreateDLLHook");

  return
    SK_CreateDLLHook (pwszModule,pszProcName,pDetour,ppOriginal,ppFuncAddr);
}

MH_STATUS
WINAPI
TZF_EnableHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (L"d3d9.dll");

  typedef MH_STATUS (WINAPI *SK_EnableHook_pfn)(LPVOID pTarget);
  static SK_EnableHook_pfn SK_EnableHook =
    (SK_EnableHook_pfn)GetProcAddress (hParent, "SK_EnableHook");

  return SK_EnableHook (pTarget);
}

MH_STATUS
WINAPI
TZF_DisableHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (L"d3d9.dll");

  typedef MH_STATUS (WINAPI *SK_DisableHook_pfn)(LPVOID pTarget);
  static SK_DisableHook_pfn SK_DisableHook =
    (SK_DisableHook_pfn)GetProcAddress (hParent, "SK_DisableHook");

  return SK_DisableHook (pTarget);
}

MH_STATUS
WINAPI
TZF_RemoveHook (LPVOID pTarget)
{
  static HMODULE hParent = GetModuleHandle (L"d3d9.dll");

  typedef MH_STATUS (WINAPI *SK_RemoveHook_pfn)(LPVOID pTarget);
  static SK_RemoveHook_pfn SK_RemoveHook =
    (SK_RemoveHook_pfn)GetProcAddress (hParent, "SK_RemoveHook");

  return SK_RemoveHook (pTarget);
}

MH_STATUS
WINAPI
TZF_Init_MinHook (void)
{
  MH_STATUS status = MH_OK;

  TZF_InputHooker* pHook = TZF_InputHooker::getInstance ();
  pHook->Start ();

  return status;
}

MH_STATUS
WINAPI
TZF_UnInit_MinHook (void)
{
  MH_STATUS status = MH_OK;

  TZF_InputHooker* pHook = TZF_InputHooker::getInstance ();
  pHook->End ();

  return status;
}

void
TZF_DrawCommandConsole (void)
{
  static int draws = 0;

  // Skip the first frame, so that the console appears below the
  //  other OSD.
  if (draws++ > 20) {
    TZF_InputHooker* pHook = TZF_InputHooker::getInstance ();
    pHook->Draw ();
  }
}


TZF_InputHooker* TZF_InputHooker::pInputHook;
char             TZF_InputHooker::text [16384];

BYTE TZF_InputHooker::keys_ [256] = { 0 };
bool TZF_InputHooker::visible     = false;

bool TZF_InputHooker::command_issued = false;
std::string TZF_InputHooker::result_str;

TZF_InputHooker::command_history_t TZF_InputHooker::commands;