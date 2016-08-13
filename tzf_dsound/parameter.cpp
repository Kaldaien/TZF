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
#define _CRT_NON_CONFORMING_SWPRINTFS

#include "parameter.h"

std::wstring
tzf::ParameterInt::get_value_str (void)
{
  wchar_t str [32];
  _itow (value, str, 10);

  return std::wstring (str);
}

int
tzf::ParameterInt::get_value (void)
{
  return value;
}

void
tzf::ParameterInt::set_value (int val)
{
  value = val;
}


void
tzf::ParameterInt::set_value_str (std::wstring str)
{
  value = _wtoi (str.c_str ());
}


void
tzf::ParameterInt::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

void
tzf::ParameterInt::store (int val)
{
  set_value      (val);
  iParameter::store ();
}

bool
tzf::ParameterInt::load (int& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
tzf::ParameterInt64::get_value_str (void)
{
  wchar_t str [32];
  _i64tow (value, str, 10);

  return std::wstring (str);
}

int64_t
tzf::ParameterInt64::get_value (void)
{
  return value;
}

void
tzf::ParameterInt64::set_value (int64_t val)
{
  value = val;
}


void
tzf::ParameterInt64::set_value_str (std::wstring str)
{
  value = _wtoll (str.c_str ());
}


void
tzf::ParameterInt64::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

void
tzf::ParameterInt64::store (int64_t val)
{
  set_value      (val);
  iParameter::store ();
}

bool
tzf::ParameterInt64::load (int64_t& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
tzf::ParameterBool::get_value_str (void)
{
  if (value == true)
    return L"true";

  return L"false";
}

bool
tzf::ParameterBool::get_value (void)
{
  return value;
}

void
tzf::ParameterBool::set_value (bool val)
{
  value = val;
}


void
tzf::ParameterBool::set_value_str (std::wstring str)
{
  if (str.length () == 1 &&
      str [0] == L'1')
    value = true;

  else if (str.length () == 4 &&
      towlower (str [0]) == L't' &&
      towlower (str [1]) == L'r' &&
      towlower (str [2]) == L'u' &&
      towlower (str [3]) == L'e')
    value = true;

  else
    value = false;
}


void
tzf::ParameterBool::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

void
tzf::ParameterBool::store (bool val)
{
  set_value      (val);
  iParameter::store ();
}

bool
tzf::ParameterBool::load (bool& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
tzf::ParameterFloat::get_value_str (void)
{
  wchar_t val_str [16];
  swprintf (val_str, L"%f", value);

  // Remove trailing 0's after the .
  size_t len = wcslen (val_str);
  for (size_t i = (len - 1); i > 1; i--) {
    if (val_str [i] == L'0' && val_str [i - 1] != L'.')
      len--;
    if (val_str [i] != L'0' && val_str [i] != L'\0')
      break;
  }

  val_str [len] = L'\0';

  return std::wstring (val_str);
}

float
tzf::ParameterFloat::get_value (void)
{
  return value;
}

void
tzf::ParameterFloat::set_value (float val)
{
  value = val;
}


void
tzf::ParameterFloat::set_value_str (std::wstring str)
{
  value = (float)wcstod (str.c_str (), NULL);
}


void
tzf::ParameterFloat::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

void
tzf::ParameterFloat::store (float val)
{
  set_value      (val);
  iParameter::store ();
}

bool
tzf::ParameterFloat::load (float& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}



std::wstring
tzf::ParameterStringW::get_value_str (void)
{
  return value;
}

std::wstring
tzf::ParameterStringW::get_value (void)
{
  return value;
}

void
tzf::ParameterStringW::set_value (std::wstring val)
{
  value = val;
}


void
tzf::ParameterStringW::set_value_str (std::wstring str)
{
  value = str;
}


void
tzf::ParameterStringW::store_str (std::wstring str)
{
  set_value_str  (str);
  iParameter::store ();
}

void
tzf::ParameterStringW::store (std::wstring val)
{
  set_value      (val);
  iParameter::store ();
}

bool
tzf::ParameterStringW::load (std::wstring& ref)
{
  bool bRet = 
    iParameter::load ();

  if (bRet)
    ref = get_value ();

  return bRet;
}


template <>
tzf::iParameter*
tzf::ParameterFactory::create_parameter <int> (const wchar_t* name)
{
  iParameter* param = new ParameterInt ();
  params.push_back (param);

  return param;
}

template <>
tzf::iParameter*
tzf::ParameterFactory::create_parameter <int64_t> (const wchar_t* name)
{
  iParameter* param = new ParameterInt64 ();
  params.push_back (param);

  return param;
}

template <>
tzf::iParameter*
tzf::ParameterFactory::create_parameter <bool> (const wchar_t* name)
{
  iParameter* param = new ParameterBool ();
  params.push_back (param);

  return param;
}

template <>
tzf::iParameter*
tzf::ParameterFactory::create_parameter <float> (const wchar_t* name)
{
  iParameter* param = new ParameterFloat ();
  params.push_back (param);

  return param;
}

template <>
tzf::iParameter*
tzf::ParameterFactory::create_parameter <std::wstring> (const wchar_t* name)
{
  iParameter* param = new ParameterStringW ();
  params.push_back (param);

  return param;
}