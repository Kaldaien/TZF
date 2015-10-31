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

#include "general_io.h"

//#define LOG_FILE_IO
#ifdef LOG_FILE_IO
typedef BOOL (WINAPI *ReadFile_t)(_In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped);

ReadFile_t ReadFile_Original = nullptr;

#include <memory>

BOOL
GetFileNameFromHandle (HANDLE hFile, WCHAR *pszFileName, const unsigned int uiMaxLen)
{
  pszFileName[0]=0;

  std::unique_ptr <WCHAR> ptrcFni (new WCHAR[_MAX_PATH + sizeof(FILE_NAME_INFO)]);
  FILE_NAME_INFO *pFni = reinterpret_cast <FILE_NAME_INFO *>(ptrcFni.get ());

  BOOL b = GetFileInformationByHandleEx (hFile, 
    FileNameInfo,
    pFni,
    sizeof(FILE_NAME_INFO) + (_MAX_PATH * sizeof (WCHAR)) );
  if ( b )
  {
    wcsncpy_s(pszFileName, 
      min(uiMaxLen, (pFni->FileNameLength / sizeof(pFni->FileName[0])) + 1 ), 
      pFni->FileName, 
      _TRUNCATE);
  }
  return b;
}

BOOL
WINAPI
ReadFile_Detour (_In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
  wchar_t wszFileName [MAX_PATH] = { L'\0' };

  GetFileNameFromHandle (hFile, wszFileName, MAX_PATH);

  dll_log.Log (L"Reading: %s ...", wszFileName);

  return ReadFile_Original (hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

typedef HANDLE (WINAPI *CreateFileW_t)(_In_     LPCTSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile);

CreateFileW_t CreateFileW_Original = nullptr;

HANDLE
WINAPI
CreateFileW_Detour (_In_     LPCWSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile)
{
  dll_log.Log (L" [!] CreateFile (%s, ...)", lpFileName);

  if (! _wcsicmp (lpFileName, L"raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return CreateFileW_Original (lpFileName, dwDesiredAccess, dwShareMode,
    lpSecurityAttributes, dwCreationDisposition,
    dwFlagsAndAttributes, hTemplateFile);
}

typedef HANDLE (WINAPI *CreateFileA_t)(_In_     LPCSTR                lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile);

CreateFileA_t CreateFileA_Original = nullptr;

HANDLE
WINAPI
CreateFileA_Detour (_In_     LPCSTR                lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile)
{
  dll_log.Log (L" [!] CreateFile (%hs, ...)", lpFileName);

  if (! stricmp (lpFileName, "raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return CreateFileA_Original (lpFileName, dwDesiredAccess, dwShareMode,
    lpSecurityAttributes, dwCreationDisposition,
    dwFlagsAndAttributes, hTemplateFile);
}


typedef HFILE (WINAPI *OpenFile_t)(_In_    LPCSTR     lpFileName,
  _Inout_ LPOFSTRUCT lpReOpenBuff,
  _In_    UINT       uStyle);

OpenFile_t OpenFile_Original = nullptr;

HFILE
WINAPI
OpenFile_Detour (_In_    LPCSTR     lpFileName,
  _Inout_ LPOFSTRUCT lpReOpenBuff,
  _In_    UINT       uStyle)
{
  dll_log.Log (L" [!] OpenFile (%hs, ...)", lpFileName);

  if (! stricmp (lpFileName, "raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return OpenFile_Original (lpFileName, lpReOpenBuff, uStyle);
}
#endif

void
tzf::FileIO::Init (void)
{
#ifdef LOG_FILE_IO
    TZF_CreateDLLHook ( L"kernel32.dll", "OpenFile",
                        OpenFile_Detour,
             (LPVOID *)&OpenFile_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "CreateFileW",
                        CreateFileW_Detour,
             (LPVOID *)&CreateFileW_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "CreateFileA",
                        CreateFileA_Detour,
             (LPVOID *)&CreateFileA_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "ReadFile",
                        ReadFile_Detour,
             (LPVOID *)&ReadFile_Original );
#endif

}

void
tzf::FileIO::Shutdown (void)
{
}