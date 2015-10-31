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
#include "hook.h"
#include "log.h"

typedef DECLSPEC_IMPORT HMODULE (WINAPI *LoadLibraryA_t)(LPCSTR  lpFileName);
typedef DECLSPEC_IMPORT HMODULE (WINAPI *LoadLibraryW_t)(LPCWSTR lpFileName);

LoadLibraryA_t LoadLibraryA_Original = nullptr;
LoadLibraryW_t LoadLibraryW_Original = nullptr;

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (strstr (lpFileName, "steam_api") ||
      strstr (lpFileName, "SteamworksNative")) {
    //TZF::SteamAPI::Init (false);
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (wcsstr (lpFileName, L"steam_api") ||
      wcsstr (lpFileName, L"SteamworksNative")) {
    //TZF::SteamAPI::Init (false);
  }

  return hMod;
}





MH_STATUS
WINAPI
TZF_CreateFuncHook ( LPCWSTR pwszFuncName,
                     LPVOID  pTarget,
                     LPVOID  pDetour,
                     LPVOID *ppOriginal )
{
  MH_STATUS status =
    MH_CreateHook ( pTarget,
                    pDetour,
                    ppOriginal );

  if (status != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Install Hook for '%s' "
                  L"[Address: %04Xh]!  (Status: \"%hs\")",
                    pwszFuncName,
                      pTarget,
                        MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
TZF_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr )
{
#if 1
  HMODULE hMod = GetModuleHandle (pwszModule);

  if (hMod == NULL) {
    if (LoadLibraryW_Original != nullptr) {
      hMod = LoadLibraryW_Original (pwszModule);
    } else {
      hMod = LoadLibraryW (pwszModule);
    }
  }

  LPVOID pFuncAddr =
    GetProcAddress (hMod, pszProcName);

  MH_STATUS status =
    MH_CreateHook ( pFuncAddr,
      pDetour,
      ppOriginal );
#else
  MH_STATUS status =
    MH_CreateHookApi ( pwszModule,
      pszProcName,
      pDetour,
      ppOriginal );
#endif

  if (status != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    pszProcName,
                      pwszModule,
                        MH_StatusToString (status) );
  }

  if (ppFuncAddr != nullptr)
    *ppFuncAddr = pFuncAddr;

  return status;
}

MH_STATUS
WINAPI
TZF_EnableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK)
  {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log( L" [ MinHook ] Failed to Enable Hook with Address: %04Xh!"
                   L" (Status: \"%hs\")",
                     pTarget,
                       MH_StatusToString (status) );
    } else {
      dll_log.Log ( L" [ MinHook ] Failed to Enable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
TZF_DisableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_DisableHook (pTarget);

  if (status != MH_OK)
  {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log ( L" [ MinHook ] Failed to Disable Hook with Address: %04Xh!"
                    L" (Status: \"%hs\")",
                      pTarget,
                        MH_StatusToString (status) );
    } else {
      dll_log.Log ( L" [ MinHook ] Failed to Disable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
TZF_RemoveHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_RemoveHook (pTarget);

  if (status != MH_OK)
  {
    dll_log.Log ( L" [ MinHook ] Failed to Remove Hook with Address: %04Xh! "
                  L"(Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
TZF_Init_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Initialize ()) != MH_OK)
  {
    dll_log.Log ( L" [ MinHook ] Failed to Initialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

#if 0
  //
  // Hook LoadLibrary so that we can watch for things like steam_api*.dll
  //
  TZF_CreateDLLHook ( L"kernel32.dll",
                       "LoadLibraryA",
                      LoadLibraryA_Detour,
           (LPVOID *)&LoadLibraryA_Original );

  TZF_CreateDLLHook ( L"kernel32.dll",
                       "LoadLibraryW",
                      LoadLibraryW_Detour,
           (LPVOID *)&LoadLibraryW_Original );
#endif

  //TZF_EnableHook (MH_ALL_HOOKS);

  return status;
}

MH_STATUS
WINAPI
TZF_UnInit_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Uninitialize ()) != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Uninitialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

  return status;
}