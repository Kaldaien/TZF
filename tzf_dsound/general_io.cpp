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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Zestiria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <Windows.h>

#include "general_io.h"

#include "config.h"
#include "log.h"
#include "hook.h"

#include <string>
#include <vector>
#include <set>

namespace tzf
{
class FileTracer {
public:
   FileTracer (void) {  
     addTracePattern  (L"Tales of Zestiria");
     addIgnorePattern (L"\\\\?\\");
     addIgnorePattern (L"NVIDIA");
  }

  ~FileTracer (void) {
  }

  void addIgnorePattern (std::wstring ignore) {
    ignore_names_.push_back (ignore);
  }

  void addTracePattern (std::wstring trace) {
    trace_names_.push_back (trace);
  }

  void addFile (std::wstring name, HANDLE file) {
    if (isTracedName (name)) {
      trace_handles_.insert (file);
    }
  }

  bool isTracedName (std::wstring name) {
    bool trace = false;

    for (int i = 0; i < trace_names_.size (); i++) {
      if (name.find (trace_names_ [i]) != std::wstring::npos) {
        trace = true;
        break;
      }
    }

    for (int i = 0; i < ignore_names_.size (); i++) {
      if (name.find (ignore_names_ [i]) != std::wstring::npos) {
        trace = false;
        break;
      }
    }

    return trace;
  }

  bool isTracedFile (HANDLE file) {
    if (trace_handles_.find (file) != trace_handles_.end ())
      return true;
    return false;
  }

protected:
  std::vector <std::wstring> ignore_names_;
  std::vector <std::wstring> trace_names_;

private:
  std::set    <HANDLE>       trace_handles_;
} *tracer = nullptr;
};

typedef BOOL (WINAPI *ReadFile_fn)
      ( _In_        HANDLE       hFile,
        _Out_       LPVOID       lpBuffer,
        _In_        DWORD        nNumberOfBytesToRead,
        _Out_opt_   LPDWORD      lpNumberOfBytesRead,
        _Inout_opt_ LPOVERLAPPED lpOverlapped );

ReadFile_fn ReadFile_Original = nullptr;

#include <memory>

BOOL
GetFileNameFromHandle ( HANDLE   hFile,
                        wchar_t *pwszFileName,
            const unsigned int   uiMaxLen )
{
  *pwszFileName = L'\0';

  std::unique_ptr <wchar_t> ptrcFni (
      new wchar_t [_MAX_PATH + sizeof FILE_NAME_INFO]
  );

  FILE_NAME_INFO *pFni =
    reinterpret_cast <FILE_NAME_INFO *>(ptrcFni.get ());

  BOOL success =
    GetFileInformationByHandleEx ( hFile, 
                                     FileNameInfo,
                                       pFni,
                                         sizeof FILE_NAME_INFO +
                            (_MAX_PATH * sizeof (wchar_t)) );
  if (success) {
    wcsncpy_s ( pwszFileName,
                  min (uiMaxLen,
                    (pFni->FileNameLength / sizeof pFni->FileName [0]) + 1),
                     pFni->FileName,
                       _TRUNCATE );
  }

  return success;
}

BOOL
WINAPI
ReadFile_Detour ( _In_      HANDLE       hFile,
                  _Out_     LPVOID       lpBuffer,
                  _In_      DWORD        nNumberOfBytesToRead,
                  _Out_opt_ LPDWORD      lpNumberOfBytesRead,
                _Inout_opt_ LPOVERLAPPED lpOverlapped )
{
  if (tzf::tracer->isTracedFile (hFile)) {
    wchar_t wszFileName [MAX_PATH] = { L'\0' };

    GetFileNameFromHandle (hFile, wszFileName, MAX_PATH);

    DWORD dwPos =
      SetFilePointer (hFile, 0L, nullptr, FILE_CURRENT);

    dll_log.Log ( L"[  FileIO  ] Reading: %-90s (%10lu bytes at 0x%06X)",
                    wszFileName,
                      nNumberOfBytesToRead,
                        dwPos );
  }

  return
    ReadFile_Original ( hFile,
                          lpBuffer,
                            nNumberOfBytesToRead,
                              lpNumberOfBytesRead,
                                lpOverlapped );
}

typedef HANDLE (WINAPI *CreateFileW_fn)(
  _In_     LPCTSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile
);

CreateFileW_fn CreateFileW_Original = nullptr;

HANDLE
WINAPI
CreateFileW_Detour ( _In_     LPCWSTR               lpFileName,
                     _In_     DWORD                 dwDesiredAccess,
                     _In_     DWORD                 dwShareMode,
                     _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     _In_     DWORD                 dwCreationDisposition,
                     _In_     DWORD                 dwFlagsAndAttributes,
                     _In_opt_ HANDLE                hTemplateFile )
{
  dll_log.Log (L"[  FileIO  ] [!] CreateFileW (%s, ...)", lpFileName);

  HANDLE hFile =
    CreateFileW_Original ( lpFileName, dwDesiredAccess, dwShareMode,
                           lpSecurityAttributes, dwCreationDisposition,
                           dwFlagsAndAttributes, hTemplateFile );

  if (hFile != 0) {
    wchar_t wszFileName [MAX_PATH] = { L'\0' };

    GetFileNameFromHandle (hFile, wszFileName, MAX_PATH);

    if (wcslen (wszFileName))
      tzf::tracer->addFile (wszFileName, hFile);
  }

  return hFile;
}

typedef HANDLE (WINAPI *CreateFileA_fn)(
   _In_     LPCSTR                lpFileName,
   _In_     DWORD                 dwDesiredAccess,
   _In_     DWORD                 dwShareMode,
   _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
   _In_     DWORD                 dwCreationDisposition,
   _In_     DWORD                 dwFlagsAndAttributes,
   _In_opt_ HANDLE                hTemplateFile );

CreateFileA_fn CreateFileA_Original = nullptr;

HANDLE
WINAPI
CreateFileA_Detour ( _In_     LPCSTR                lpFileName,
                     _In_     DWORD                 dwDesiredAccess,
                     _In_     DWORD                 dwShareMode,
                     _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     _In_     DWORD                 dwCreationDisposition,
                     _In_     DWORD                 dwFlagsAndAttributes,
                     _In_opt_ HANDLE                hTemplateFile )
{
  dll_log.Log ( L"[  FileIO  ] [!] CreateFileA (%hs, 0x%X, 0x%X, ..., 0x%X, 0x%X)",
                lpFileName, dwDesiredAccess, dwShareMode,
                dwCreationDisposition, dwFlagsAndAttributes );

  HANDLE hFile =
    CreateFileA_Original ( lpFileName,           dwDesiredAccess,
                           dwShareMode,
                           lpSecurityAttributes, dwCreationDisposition,
                           dwFlagsAndAttributes, hTemplateFile );

  if (hFile != 0) {
    wchar_t wszFileName [MAX_PATH] = { L'\0' };

    GetFileNameFromHandle (hFile, wszFileName, MAX_PATH);

    if (wcslen (wszFileName))
      tzf::tracer->addFile (wszFileName, hFile);
  }

  return hFile;
}


typedef HFILE (WINAPI *OpenFile_fn)( _In_    LPCSTR     lpFileName,
                                     _Inout_ LPOFSTRUCT lpReOpenBuff,
                                     _In_    UINT       uStyle );

OpenFile_fn OpenFile_Original = nullptr;

HFILE
WINAPI
OpenFile_Detour ( _In_    LPCSTR     lpFileName,
                  _Inout_ LPOFSTRUCT lpReOpenBuff,
                  _In_    UINT       uStyle )
{
  dll_log.Log (L"[  FileIO  ] [!] OpenFile (%hs, ...)", lpFileName);

  HFILE hFile =
    OpenFile_Original (lpFileName, lpReOpenBuff, uStyle);

  if (hFile != 0) {
    wchar_t wszFileName [MAX_PATH] = { L'\0' };

    GetFileNameFromHandle ((HANDLE)hFile, wszFileName, MAX_PATH);

    if (wcslen (wszFileName))
      tzf::tracer->addFile (wszFileName, (HANDLE)hFile);
  }

  return hFile;
}

void
tzf::FileIO::Init (void)
{
  //config.file_io.capture = true;
  if (config.file_io.capture) {

    if (tracer == nullptr)
      tracer = new FileTracer ();

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
  }
}

void
tzf::FileIO::Shutdown (void)
{
  if (tracer != nullptr) {
    delete tracer;
    tracer = nullptr;
  }
}