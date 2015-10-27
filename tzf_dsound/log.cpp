/**
* This file is part of Batman "Fix".
*
* Batman "Fix" is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* The Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Batman "Fix" is distributed in the hope that it will be useful,
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Batman "Fix". If not, see <http://www.gnu.org/licenses/>.
**/

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include "log.h"

WORD
TZF_Timestamp (wchar_t* const out)
{
  SYSTEMTIME stLogTime;
  GetLocalTime (&stLogTime);

  wchar_t date [64] = { L'\0' };
  wchar_t time [64] = { L'\0' };

  GetDateFormat (LOCALE_INVARIANT,DATE_SHORTDATE,   &stLogTime,NULL,date,64);
  GetTimeFormat (LOCALE_INVARIANT,TIME_NOTIMEMARKER,&stLogTime,NULL,time,64);

  out [0] = L'\0';

  lstrcatW (out, date);
  lstrcatW (out, L" ");
  lstrcatW (out, time);
  lstrcatW (out, L".");

  return stLogTime.wMilliseconds;
}

tzf_logger_t dll_log;


void
tzf_logger_t::close (void)
{
  if (fLog != NULL) {
    fflush (fLog);
    fclose (fLog);
  }

  initialized = false;
  silent      = true;

  DeleteCriticalSection (&log_mutex);
}

bool
tzf_logger_t::init (const char* const szFileName,
  const char* const szMode)
{
  if (initialized)
    return true;

  fLog = fopen (szFileName, szMode);

  BOOL bRet = InitializeCriticalSectionAndSpinCount (&log_mutex, 2500);

  if ((! bRet) || (fLog == NULL)) {
    silent = true;
    return false;
  }

  initialized = true;
  return initialized;
}

void
tzf_logger_t::LogEx (bool                 _Timestamp,
  _In_z_ _Printf_format_string_
                     wchar_t const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  if (_Timestamp) {
    wchar_t wszLogTime [128];

    WORD ms = TZF_Timestamp (wszLogTime);

    fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);
  }

  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fflush (fLog);

  LeaveCriticalSection (&log_mutex);
}

void
tzf_logger_t::Log   (_In_z_ _Printf_format_string_
                      wchar_t const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  wchar_t wszLogTime [128];

  WORD ms = TZF_Timestamp (wszLogTime);

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf  (fLog, L"\n");
  fflush    (fLog);

  LeaveCriticalSection (&log_mutex);
}

void
tzf_logger_t::Log   (_In_z_ _Printf_format_string_
                      char const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  wchar_t wszLogTime [128];

  WORD ms = TZF_Timestamp (wszLogTime);

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_start (_ArgList, _Format);
  {
    vfprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf  (fLog, L"\n");
  fflush    (fLog);

  LeaveCriticalSection (&log_mutex);
}