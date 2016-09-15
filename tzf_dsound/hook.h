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
#ifndef __TZF__HOOK_H__
#define __TZF__HOOK_H__

// MinHook Error Codes.
typedef enum MH_STATUS
{
    // Unknown error. Should not be returned.
    MH_UNKNOWN = -1,

    // Successful.
    MH_OK = 0,

    // MinHook is already initialized.
    MH_ERROR_ALREADY_INITIALIZED,

    // MinHook is not initialized yet, or already uninitialized.
    MH_ERROR_NOT_INITIALIZED,

    // The hook for the specified target function is already created.
    MH_ERROR_ALREADY_CREATED,

    // The hook for the specified target function is not created yet.
    MH_ERROR_NOT_CREATED,

    // The hook for the specified target function is already enabled.
    MH_ERROR_ENABLED,

    // The hook for the specified target function is not enabled yet, or already
    // disabled.
    MH_ERROR_DISABLED,

    // The specified pointer is invalid. It points the address of non-allocated
    // and/or non-executable region.
    MH_ERROR_NOT_EXECUTABLE,

    // The specified target function cannot be hooked.
    MH_ERROR_UNSUPPORTED_FUNCTION,

    // Failed to allocate memory.
    MH_ERROR_MEMORY_ALLOC,

    // Failed to change the memory protection.
    MH_ERROR_MEMORY_PROTECT,

    // The specified module is not loaded.
    MH_ERROR_MODULE_NOT_FOUND,

    // The specified function is not found.
    MH_ERROR_FUNCTION_NOT_FOUND
}
MH_STATUS;

typedef const char*    LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef void*          LPVOID;

MH_STATUS
__stdcall
TZF_CreateFuncHook ( LPCWSTR pwszFuncName,
                     LPVOID  pTarget,
                     LPVOID  pDetour,
                     LPVOID *ppOriginal );

MH_STATUS
__stdcall
TZF_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID* ppFuncAddr = nullptr );

MH_STATUS
__stdcall
TZF_CreateDLLHook2 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                     LPVOID  pDetour,    LPVOID *ppOriginal,
                     LPVOID* ppFuncAddr = nullptr );

MH_STATUS
__stdcall
TZF_EnableHook (LPVOID pTarget);

MH_STATUS
__stdcall
TZF_DisableHook (LPVOID pTarget);

MH_STATUS
__stdcall
TZF_RemoveHook (LPVOID pTarget);

MH_STATUS
__stdcall
TZF_ApplyQueuedHooks (void);

MH_STATUS
__stdcall
TZF_Init_MinHook (void);

MH_STATUS
__stdcall
TZF_UnInit_MinHook (void);

#endif __TZF__HOOK_H__