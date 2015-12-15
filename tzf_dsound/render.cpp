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

#include "render.h"
#include "framerate.h"
#include "config.h"
#include "log.h"
#include "scanner.h"

#include <stdint.h>

#include <d3d9.h>
#include <d3d9types.h>

#include <set>

// Textures that are missing mipmaps
std::set <IDirect3DBaseTexture9 *> incomplete_textures;
bool pre_limit      = false;

uint32_t
TZF_MakeShadowBitShift (uint32_t dim)
{
  uint32_t shift = abs (config.render.shadow_rescale);

  // If this is 64x64 and we want all shadows the same resolution, then add
  //   an extra shift.
  shift += ((config.render.shadow_rescale) < 0L) * (dim == 64UL);

  return shift;
}


void
TZF_ComputeAspectCoeffs (float& x, float& y, float& xoff, float& yoff)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction || config.render.blackbar_videos))
    return;

  float rescale = (1.77777778f / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    int width = (16.0f / 9.0f) * tzf::RenderFix::height;
    int x_off = (tzf::RenderFix::width - width) / 2;

    x    = (float)tzf::RenderFix::width / (float)width;
    xoff = x_off;
  } else {
    int height = (9.0f / 16.0f) * tzf::RenderFix::width;
    int y_off  = (tzf::RenderFix::height - height) / 2;

    y    = (float)tzf::RenderFix::height / (float)height;
    yoff = y_off;
  }
}

typedef D3DMATRIX* (WINAPI *D3DXMatrixMultiply_t)(_Inout_    D3DMATRIX *pOut,
                                                  _In_ const D3DMATRIX *pM1,
                                                  _In_ const D3DMATRIX *pM2);

D3DXMatrixMultiply_t D3DXMatrixMultiply_Original = nullptr;

D3DMATRIX*
WINAPI
D3DXMatrixMultiply_Detour (_Inout_       D3DMATRIX *pOut,
                           _In_          D3DMATRIX *pM1,
                           _In_          D3DMATRIX *pM2)
{
  return D3DXMatrixMultiply_Original (pOut, pM1, pM2);
}

#include "hook.h"

typedef HRESULT (STDMETHODCALLTYPE *SetSamplerState_t)
(IDirect3DDevice9*   This,
  DWORD               Sampler,
  D3DSAMPLERSTATETYPE Type,
  DWORD               Value);

SetSamplerState_t D3D9SetSamplerState_Original = nullptr;

LPVOID SetSamplerState = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetSamplerState_Detour (IDirect3DDevice9*   This,
                            DWORD               Sampler,
                            D3DSAMPLERSTATETYPE Type,
                            DWORD               Value)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9SetSamplerState_Original (This, Sampler, Type, Value);

  static int aniso = 1;

  //dll_log.Log ( L" [!] IDirect3DDevice9::SetSamplerState (%lu, %lu, %lu)",
                  //Sampler, Type, Value );

  // Pending removal - these are not configurable tweaks and not particularly useful
  if (Type == D3DSAMP_MIPFILTER ||
      Type == D3DSAMP_MINFILTER ||
      Type == D3DSAMP_MAGFILTER ||
      Type == D3DSAMP_MIPMAPLODBIAS) {
    //dll_log.Log (L" [!] IDirect3DDevice9::SetSamplerState (...)");

    if (Type < 8) {
      //if (Value != D3DTEXF_ANISOTROPIC)
        //D3D9SetSamplerState_Original (This, Sampler, D3DSAMP_MAXANISOTROPY, aniso);

      //dll_log.Log (L" %s Filter: %x", Type == D3DSAMP_MIPFILTER ? L"Mip" : Type == D3DSAMP_MINFILTER ? L"Min" : L"Mag", Value);
      if (Type == D3DSAMP_MIPFILTER) {
        if (Value != D3DTEXF_NONE)
          Value = D3DTEXF_ANISOTROPIC;
      }
      if (Type == D3DSAMP_MAGFILTER ||
                  D3DSAMP_MINFILTER)
        if (Value != D3DTEXF_POINT)
          Value = D3DTEXF_ANISOTROPIC;
    } else {
      float bias = *(float *)&Value;

      // Bad game, bad! Negative LOD Bias is UGLY AS HELL!
      if (bias < 0.0f)
        bias = 0.0f;

      Value = *(DWORD *)&bias;
      //dll_log.Log (L" Mip Bias: %f", (double)*(float *)&Value);
    }
  }
  if (Type == D3DSAMP_MAXANISOTROPY) {
    aniso = Value;
    //Value = 16;
  }
  //if (Type == D3DSAMP_MAXMIPLEVEL)
    //Value = 0;

  return D3D9SetSamplerState_Original (This, Sampler, Type, Value);
}

IDirect3DVertexShader9* g_pVS;
IDirect3DPixelShader9*  g_pPS;

static uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
crc32(uint32_t crc, const void *buf, size_t size)
{
  const uint8_t *p;

  p = (uint8_t *)buf;
  crc = crc ^ ~0U;

  while (size--)
    crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ ~0U;
}

#include <map>
std::unordered_map <LPVOID, uint32_t> vs_checksums;
std::unordered_map <LPVOID, uint32_t> ps_checksums;

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_t)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

SetVertexShader_t D3D9SetVertexShader_Original = nullptr;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Detour (IDirect3DDevice9*       This,
                            IDirect3DVertexShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9SetVertexShader_Original (This, pShader);

  if (g_pVS != pShader) {
    if (pShader != nullptr) {
      if (vs_checksums.find (pShader) == vs_checksums.end ()) {
        UINT len = 2048;
        char szFunc [2048];

        pShader->GetFunction (szFunc, &len);

        vs_checksums [pShader] = crc32 (0, szFunc, len);
      }
    }
  }

  g_pVS = pShader;
  return D3D9SetVertexShader_Original (This, pShader);
}


typedef HRESULT (STDMETHODCALLTYPE *SetPixelShader_t)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

SetPixelShader_t D3D9SetPixelShader_Original = nullptr;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShader_Detour (IDirect3DDevice9*      This,
                           IDirect3DPixelShader9* pShader)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9SetPixelShader_Original (This, pShader);

  if (g_pPS != pShader) {
    if (pShader != nullptr) {
      if (ps_checksums.find (pShader) == ps_checksums.end ()) {
        UINT len = 8191;
        char szFunc [8192] = { 0 };

        pShader->GetFunction (szFunc, &len);

        ps_checksums [pShader] = crc32 (0, szFunc, len);
      }
    }
  }

  g_pPS = pShader;
  return D3D9SetPixelShader_Original (This, pShader);
}


//
// Bink Video Vertex Shader
//
const uint32_t VS_CHECKSUM_BINK = 3463109298UL;

const uint32_t PS_CHECKSUM_UI   =  363447431UL;
const uint32_t VS_CHECKSUM_UI   =  657093040UL;

//
// Model Shadow Shaders (primary)
//
const uint32_t PS_CHECKSUM_CHAR_SHADOW = 1180797962UL;
const uint32_t VS_CHECKSUM_CHAR_SHADOW =  446150694UL;


typedef void (STDMETHODCALLTYPE *BMF_BeginBufferSwap_t)(void);
BMF_BeginBufferSwap_t BMF_BeginBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *BMF_EndBufferSwap_t)
  (HRESULT   hr,
   IUnknown* device);
BMF_EndBufferSwap_t BMF_EndBufferSwap = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *SetScissorRect_t)(
  IDirect3DDevice9* This,
  const RECT*       pRect);

SetScissorRect_t D3D9SetScissorRect_Original = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *EndScene_t)
(IDirect3DDevice9* This);

EndScene_t D3D9EndScene_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Detour (IDirect3DDevice9* This)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9EndScene_Original (This);

  if ( game_state.hasFixedAspect () &&
       (config.render.aspect_correction ||
       (config.render.blackbar_videos   &&
        tzf::RenderFix::bink))      &&
       config.render.clear_blackbars ) {
    D3DCOLOR color = 0xff000000;

    int width = tzf::RenderFix::width;
    int height = (9.0f / 16.0f) * width;

    // We can't do this, so instead we need to sidebar the stuff
    if (height > tzf::RenderFix::height) {
      width  = (16.0f / 9.0f) * tzf::RenderFix::height;
      height = tzf::RenderFix::height;
    }

    if (height != tzf::RenderFix::height) {
      RECT top;
      top.top    = 0;
      top.left   = 0;
      top.right  = tzf::RenderFix::width;
      top.bottom = top.top + (tzf::RenderFix::height - height) / 2 + 1;
      D3D9SetScissorRect_Original (tzf::RenderFix::pDevice, &top);
      tzf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, 1);

      tzf::RenderFix::pDevice->Clear (0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0xff);

      RECT bottom;
      bottom.top    = tzf::RenderFix::height - (tzf::RenderFix::height - height) / 2;
      bottom.left   = 0;
      bottom.right  = tzf::RenderFix::width;
      bottom.bottom = tzf::RenderFix::height;
      D3D9SetScissorRect_Original (tzf::RenderFix::pDevice, &bottom);

      tzf::RenderFix::pDevice->Clear (0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0xff);

      tzf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, 0);
    }

    if (width != tzf::RenderFix::width) {
      RECT left;
      left.top    = 0;
      left.left   = 0;
      left.right  = left.left + (tzf::RenderFix::width - width) / 2 + 1;
      left.bottom = tzf::RenderFix::height;
      D3D9SetScissorRect_Original (tzf::RenderFix::pDevice, &left);
      tzf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, 1);

      tzf::RenderFix::pDevice->Clear (0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0xff);

      RECT right;
      right.top    = 0;
      right.left   = tzf::RenderFix::width - (tzf::RenderFix::width - width) / 2;
      right.right  = tzf::RenderFix::width;
      right.bottom = tzf::RenderFix::height;
      D3D9SetScissorRect_Original (tzf::RenderFix::pDevice, &right);
      tzf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, 1);

      tzf::RenderFix::pDevice->Clear (0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0xff);

      tzf::RenderFix::pDevice->SetRenderState (D3DRS_SCISSORTESTENABLE, 0);
    }
  }

  HRESULT hr = D3D9EndScene_Original (This);

  return hr;
}

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
D3D9EndFrame_Pre (void)
{
  if (pre_limit)
    tzf::FrameRateFix::RenderTick ();

  return BMF_BeginBufferSwap ();
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndFrame_Post (HRESULT hr, IUnknown* device)
{
  // Ignore anything that's not the primary render device.
  if (device != tzf::RenderFix::pDevice)
    return BMF_EndBufferSwap (hr, device);

  tzf::RenderFix::dwRenderThreadID = GetCurrentThreadId ();

  TZF_DrawCommandConsole ();

  hr = BMF_EndBufferSwap (hr, device);

  if (! pre_limit)
    tzf::FrameRateFix::RenderTick ();

  // Don't do the silly stuff below if this option is not enabled
  if (! config.framerate.minimize_latency) 
    return hr;

  if (SUCCEEDED (hr) && device != nullptr) {
    IDirect3DSurface9* pBackBuffer = nullptr;
    IDirect3DDevice9*  pDev        = nullptr;

    if (SUCCEEDED (device->QueryInterface ( __uuidof (IDirect3DDevice9),
                                              (void **)&pDev )))
    {
      if (SUCCEEDED ( pDev->GetBackBuffer ( 0,
                                              0,
                                                D3DBACKBUFFER_TYPE_MONO,
                                                  &pBackBuffer ) ))
      {
        D3DLOCKED_RECT lock;

        //
        // This will block and stall the pipeline; effectively enforcing a
        //   pre-rendered frame limit of 1 without using any vendor-specific
        //     driver settings. 
        //
        //   Huzzah!
        //
        //DWORD dwTime = timeGetTime ();

        if (SUCCEEDED ( pBackBuffer->LockRect ( &lock,
                                                  nullptr,
                                                    D3DLOCK_NO_DIRTY_UPDATE |
                                                      D3DLOCK_READONLY      |
                                                        D3DLOCK_NOSYSLOCK )))
          pBackBuffer->UnlockRect ();

#if 0
        DWORD dwWait = timeGetTime () - dwTime;
        if (dwWait > 0) {
          dll_log.Log (L"Locked Backbuffer and waited %lu ms", dwWait);
        }
#endif

        pBackBuffer->Release ();
      }

      pDev->Release ();
    }
  }

  return hr;
}

typedef enum D3DXIMAGE_FILEFORMAT { 
  D3DXIFF_BMP          = 0,
  D3DXIFF_JPG          = 1,
  D3DXIFF_TGA          = 2,
  D3DXIFF_PNG          = 3,
  D3DXIFF_DDS          = 4,
  D3DXIFF_PPM          = 5,
  D3DXIFF_DIB          = 6,
  D3DXIFF_HDR          = 7,
  D3DXIFF_PFM          = 8,
  D3DXIFF_FORCE_DWORD  = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

typedef HRESULT (STDMETHODCALLTYPE *D3DXSaveTextureToFile_t)(
  _In_       LPCWSTR                pDestFile,
  _In_       D3DXIMAGE_FILEFORMAT   DestFormat,
  _In_       LPDIRECT3DBASETEXTURE9 pSrcTexture,
  _In_ const PALETTEENTRY           *pSrcPalette
  );

D3DXSaveTextureToFile_t
  D3DXSaveTextureToFile = nullptr;

typedef HRESULT (STDMETHODCALLTYPE *UpdateTexture_t)
  (IDirect3DDevice9      *This,
   IDirect3DBaseTexture9 *pSourceTexture,
   IDirect3DBaseTexture9 *pDestinationTexture);

UpdateTexture_t D3D9UpdateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9UpdateTexture_Detour (IDirect3DDevice9      *This,
                          IDirect3DBaseTexture9 *pSourceTexture,
                          IDirect3DBaseTexture9 *pDestinationTexture)
{
  HRESULT hr = D3D9UpdateTexture_Original (This, pSourceTexture,
                                                 pDestinationTexture);

  if (SUCCEEDED (hr)) {
#if 0
    if ( incomplete_textures.find (pDestinationTexture) != 
         incomplete_textures.end () ) {
      dll_log.Log (L" Generating Mipmap LODs for incomplete texture!");
      (pDestinationTexture->GenerateMipSubLevels ());
    }
#endif
#ifdef DUMP_TEXTURES
    if (SUCCEEDED (hr)) {
      if (D3DXSaveTextureToFile == nullptr) {
        D3DXSaveTextureToFile =
          (D3DXSaveTextureToFile_t)
          GetProcAddress ( tzf::RenderFix::d3dx9_43_dll,
            "D3DXSaveTextureToFileW" );
      }

      if (D3DXSaveTextureToFile != nullptr) {
        wchar_t wszFileName [MAX_PATH] = { L'\0' };
        _swprintf ( wszFileName, L"textures\\%x.png",
          pSourceTexture );
        D3DXSaveTextureToFile (wszFileName, D3DXIFF_PNG, pDestinationTexture, NULL);
      }
    }
#endif
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_t)
  (IDirect3DDevice9   *This,
   UINT                Width,
   UINT                Height,
   UINT                Levels,
   DWORD               Usage,
   D3DFORMAT           Format,
   D3DPOOL             Pool,
   IDirect3DTexture9 **ppTexture,
   HANDLE             *pSharedHandle);

CreateTexture_t D3D9CreateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Detour (IDirect3DDevice9   *This,
                          UINT                Width,
                          UINT                Height,
                          UINT                Levels,
                          DWORD               Usage,
                          D3DFORMAT           Format,
                          D3DPOOL             Pool,
                          IDirect3DTexture9 **ppTexture,
                          HANDLE             *pSharedHandle)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9CreateTexture_Original ( This, Width, Height, Levels, Usage,
                                          Format, Pool, ppTexture, pSharedHandle );


#if 0
  if (Usage == D3DUSAGE_RENDERTARGET)
  dll_log.Log (L" [!] IDirect3DDevice9::CreateTexture (%lu, %lu, %lu, %lu, "
                                                  L"%lu, %lu, %08Xh, %08Xh)",
                 Width, Height, Levels, Usage, Format, Pool, ppTexture,
                 pSharedHandle);
#endif

#if 0
  bool full_mipmaps = true;
  if (Levels != 1) {
    if (Levels < (log2 (max (Width, Height))) + 1) {

      // Restrict to DXT1 or DXT5
      if (Format == 827611204UL ||
          Format == 894720068UL) {
        full_mipmaps = false;
      }
    }
  }
#endif

  //
  // Model Shadows
  //
  if (Width == Height && (Width == 64 || Width == 128) &&
                          Usage == D3DUSAGE_RENDERTARGET) {
    // Assert (Levels == 1)
    //
    //   If Levels is not 1, then we've kind of screwed up because now we don't
    //     have a complete mipchain anymore.

    uint32_t shift = TZF_MakeShadowBitShift (Width);

    Width  <<= shift;
    Height <<= shift;
  }

  //
  // Post-Processing (512x256) - FIXME damnit!
  //
  if (Width  == 512 &&
      Height == 256 && Usage == D3DUSAGE_RENDERTARGET) {
    if (config.render.postproc_ratio > 0.0f) {
      Width  = tzf::RenderFix::width  * config.render.postproc_ratio;
      Height = tzf::RenderFix::height * config.render.postproc_ratio;
    }
  }

 if (Usage == D3DUSAGE_DEPTHSTENCIL) {
    if (Width == Height && (Height == 512 || Height == 1024 || Height == 2048)) {
      uint32_t shift = config.render.env_shadow_rescale;

      Width  <<= shift;
      Height <<= shift;
    }
  }

  int levels = Levels;

  HRESULT hr = 
    D3D9CreateTexture_Original (This, Width, Height, levels, Usage,
                                Format, Pool, ppTexture, pSharedHandle);

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_t)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Discard,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateDepthStencilSurface_t D3D9CreateDepthStencilSurface_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Detour (IDirect3DDevice9     *This,
                                      UINT                  Width,
                                      UINT                  Height,
                                      D3DFORMAT             Format,
                                      D3DMULTISAMPLE_TYPE   MultiSample,
                                      DWORD                 MultisampleQuality,
                                      BOOL                  Discard,
                                      IDirect3DSurface9   **ppSurface,
                                      HANDLE               *pSharedHandle)
{
  dll_log.Log (L" [!] IDirect3DDevice9::CreateDepthStencilSurface (%lu, %lu, "
                      L"%lu, %lu, %lu, %lu, %08Xh, %08Xh)",
                 Width, Height, Format, MultiSample, MultisampleQuality,
                 Discard, ppSurface, pSharedHandle);

  return D3D9CreateDepthStencilSurface_Original (This, Width, Height, Format,
                                                 MultiSample, MultisampleQuality,
                                                 Discard, ppSurface, pSharedHandle);
}

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_t)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Lockable,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateRenderTarget_t D3D9CreateRenderTarget_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Detour (IDirect3DDevice9     *This,
                               UINT                  Width,
                               UINT                  Height,
                               D3DFORMAT             Format,
                               D3DMULTISAMPLE_TYPE   MultiSample,
                               DWORD                 MultisampleQuality,
                               BOOL                  Lockable,
                               IDirect3DSurface9   **ppSurface,
                               HANDLE               *pSharedHandle)
{
  dll_log.Log (L" [!] IDirect3DDevice9::CreateRenderTarget (%lu, %lu, "
                      L"%lu, %lu, %lu, %lu, %08Xh, %08Xh)",
                 Width, Height, Format, MultiSample, MultisampleQuality,
                 Lockable, ppSurface, pSharedHandle);

  return D3D9CreateRenderTarget_Original (This, Width, Height, Format,
                                          MultiSample, MultisampleQuality,
                                          Lockable, ppSurface, pSharedHandle);
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetScissorRect_Detour (IDirect3DDevice9* This,
                     const RECT*             pRect)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9SetScissorRect_Original (This, pRect);

  // If we don't care about aspect ratio, then just early-out
  if (! config.render.aspect_correction)
    return D3D9SetScissorRect_Original (This, pRect);

  // Otherwise, fix this because the UI's scissor rectangles are
  //   completely wrong after we start messing with viewport scaling.

  RECT fixed_scissor;
  fixed_scissor.bottom = pRect->bottom;
  fixed_scissor.top    = pRect->top;
  fixed_scissor.left   = pRect->left;
  fixed_scissor.right  = pRect->right;

  float x_scale, y_scale;
  float x_off,   y_off;
  TZF_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

  // Wider
  if (config.render.aspect_ratio > 1.7777f) {
    float left_ndc  = 2.0f * ((float)pRect->left  / (float)tzf::RenderFix::width) - 1.0f;
    float right_ndc = 2.0f * ((float)pRect->right / (float)tzf::RenderFix::width) - 1.0f;

    int width = (16.0f / 9.0f) * tzf::RenderFix::height;

    fixed_scissor.left  = (left_ndc  * width + width) / 2.0f + x_off;
    fixed_scissor.right = (right_ndc * width + width) / 2.0f + x_off;
  } else {
    float top_ndc    = 2.0f * ((float)pRect->top    / (float)tzf::RenderFix::height) - 1.0f;
    float bottom_ndc = 2.0f * ((float)pRect->bottom / (float)tzf::RenderFix::height) - 1.0f;

    int height = (9.0f / 16.0f) * tzf::RenderFix::width;

    fixed_scissor.top    = (top_ndc    * height + height) / 2.0f + y_off;
    fixed_scissor.bottom = (bottom_ndc * height + height) / 2.0f + y_off;
  }

  return D3D9SetScissorRect_Original (This, &fixed_scissor);
}

typedef HRESULT (STDMETHODCALLTYPE *SetViewport_t)(
        IDirect3DDevice9* This,
  CONST D3DVIEWPORT9*     pViewport);

SetViewport_t D3D9SetViewport_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetViewport_Detour (IDirect3DDevice9* This,
                  CONST D3DVIEWPORT9*     pViewport)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice)
    return D3D9SetViewport_Original (This, pViewport);

  //
  // Adjust Character Drop Shadows
  //
  if (pViewport->Width == pViewport->Height &&
     (pViewport->Width == 64 || pViewport->Width == 128)) {
    D3DVIEWPORT9 rescaled_shadow = *pViewport;

    uint32_t shift = TZF_MakeShadowBitShift (pViewport->Width);

    rescaled_shadow.Width  <<= shift;
    rescaled_shadow.Height <<= shift;

    return D3D9SetViewport_Original (This, &rescaled_shadow);
  }

#if 0
  //
  // Environmental Shadows
  //
  if (pViewport->Width == pViewport->Height && 
      (pViewport->Width ==  512 ||
       pViewport->Width == 1024 ||
       pViewport->Width == 2048)) {
    D3DVIEWPORT9 rescaled_shadow = *pViewport;

    rescaled_shadow.Width  <<= config.render.env_shadow_rescale;
    rescaled_shadow.Height <<= config.render.env_shadow_rescale;

    return D3D9SetViewport_Original (This, &rescaled_shadow);
  }
#endif

  //
  // Adjust Post-Processing
  //
  if (pViewport->Width  == 512 &&
      pViewport->Height == 256 && config.render.postproc_ratio > 0.0f) {
    D3DVIEWPORT9 rescaled_post_proc = *pViewport;

    float scale_x, scale_y;
    float x_off,   y_off;
    scale_x = 1.0f; scale_y = 1.0f;
    x_off   = 0.0f;   y_off = 0.0f;
    //TZF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

    rescaled_post_proc.Width  = tzf::RenderFix::width  * config.render.postproc_ratio * scale_x;
    rescaled_post_proc.Height = tzf::RenderFix::height * config.render.postproc_ratio * scale_y;
    rescaled_post_proc.X       += x_off;
    rescaled_post_proc.Y       += y_off;

    return D3D9SetViewport_Original (This, &rescaled_post_proc);
  }

  return D3D9SetViewport_Original (This, pViewport);
}

#if 0
float data [20];
memcpy (data, pConstantData, sizeof (float) * 20);

float rescale = ((16.0f / 9.0f) / config.render.aspect_ratio);

// Wider
if (config.render.aspect_ratio > (16.0f / 9.0f)) {
  int width = (16.0f / 9.0f) * tzf::RenderFix::height;
  int x_off = (tzf::RenderFix::width - width) / 2;

  data [ 0] *= rescale;
  data [ 3] *= rescale;
  data [ 4] *= rescale;
  data [ 8] *= rescale;
  data [12] *= rescale;
} else {
  int height = (9.0f / 16.0f) * tzf::RenderFix::width;
  int y_off  = (tzf::RenderFix::height - height) / 2;

  data [ 1] *= rescale;
  data [ 5] *= rescale;
  data [ 7] *= rescale;
  data [ 9] *= rescale;
  data [13] *= rescale;
}

return D3D9SetVertexShaderConstantF_Original (This, StartRegister, data, Vector4fCount);
#endif

void
TZF_AdjustViewport (IDirect3DDevice9* This, bool UI)
{
  D3DVIEWPORT9 vp9_orig;
  This->GetViewport (&vp9_orig);

  if (! UI) {
    vp9_orig.X = 0;
    vp9_orig.Y = 0;
    vp9_orig.Width  = tzf::RenderFix::width;
    vp9_orig.Height = tzf::RenderFix::height;
    This->SetViewport (&vp9_orig);
    return;
  }

  vp9_orig.X = 0;
  vp9_orig.Y = 0;
  vp9_orig.Width  = tzf::RenderFix::width;
  vp9_orig.Height = tzf::RenderFix::height;

  DWORD width = vp9_orig.Width;
  DWORD height = (9.0f / 16.0f) * vp9_orig.Width;

  // We can't do this, so instead we need to sidebar the stuff
  if (height > vp9_orig.Height) {
    width  = (16.0f / 9.0f) * vp9_orig.Height;
    height = vp9_orig.Height;
  }

  if (height != vp9_orig.Height) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X;    vp9.Y      = vp9_orig.Y + (vp9_orig.Height - height) / 2;
    vp9.Width = width;         vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ; vp9.MaxZ   = vp9_orig.MaxZ;

    This->SetViewport (&vp9);
  }

  // Sidebar Videos
  if (width != vp9_orig.Width) {
    D3DVIEWPORT9 vp9;
    vp9.X     = vp9_orig.X + (vp9_orig.Width - width) / 2; vp9.Y = vp9_orig.Y;
    vp9.Width = width;                                     vp9.Height = height;
    vp9.MinZ  = vp9_orig.MinZ;                             vp9.MaxZ   = vp9_orig.MaxZ;

    This->SetViewport (&vp9);
  }
}

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive_t)(
  IDirect3DDevice9* This,
  D3DPRIMITIVETYPE  Type,
  INT               BaseVertexIndex,
  UINT              MinVertexIndex,
  UINT              NumVertices,
  UINT              startIndex,
  UINT              primCount);

DrawIndexedPrimitive_t D3D9DrawIndexedPrimitive_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitive_Detour (IDirect3DDevice9* This,
                                 D3DPRIMITIVETYPE  Type,
                                 INT               BaseVertexIndex,
                                 UINT              MinVertexIndex,
                                 UINT              NumVertices,
                                 UINT              startIndex,
                                 UINT              primCount)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice) {
    return D3D9DrawIndexedPrimitive_Original ( This, Type,
                                                 BaseVertexIndex, MinVertexIndex,
                                                   NumVertices, startIndex,
                                                     primCount );
  }

  if ((config.render.aspect_correction && Type == D3DPT_TRIANGLESTRIP && (vs_checksums [g_pVS] == 0x52BD224A ||
                                                                          vs_checksums [g_pVS] == 0x272A71B0 || // Splash Screen
                                                                          vs_checksums [g_pVS] == 0x66E0873 // Fadein / Fadeout Effect
                                                                          )) ||
     (config.render.blackbar_videos && tzf::RenderFix::bink && vs_checksums [g_pVS] == VS_CHECKSUM_BINK)) {
    TZF_AdjustViewport (This, true);
  }
  else if (Type == D3DPT_TRIANGLESTRIP) {
    //dll_log.Log (L" Consider Vertex Shader 0x%04X...", vs_checksums [g_pVS]);
  }

  return D3D9DrawIndexedPrimitive_Original ( This, Type,
                                              BaseVertexIndex, MinVertexIndex,
                                                NumVertices, startIndex,
                                                  primCount );
}


typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_t)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_t D3D9SetVertexShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Detour (IDirect3DDevice9* This,
                                     UINT              StartRegister,
                                     CONST float*      pConstantData,
                                     UINT              Vector4fCount)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice) {
    return D3D9SetVertexShaderConstantF_Original ( This,
                                                     StartRegister,
                                                       pConstantData,
                                                         Vector4fCount );
  }


  //
  // Model Shadows
  //
  if (StartRegister == 240 && Vector4fCount == 1) {
    uint32_t shift;
    uint32_t dim = 0;

    if (pConstantData [0] == -1.0f / 64.0f) {
      dim = 64UL;
      //dll_log.Log (L" 64x64 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksums [g_pVS], ps_checksums [g_pPS]);
    }

    if (pConstantData [0] == -1.0f / 128.0f) {
      dim = 128UL;
      //dll_log.Log (L" 128x128 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksums [g_pVS], ps_checksums [g_pPS]);
    }

    shift = TZF_MakeShadowBitShift (dim);

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    newData [0] = -1.0f / (dim << shift);
    newData [1] =  1.0f / (dim << shift);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log.Log (L" Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    if (dim != 0) {
      return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
    }
  }

  //
  // Post-Processing
  //
  if (StartRegister     == 240 &&
      Vector4fCount     == 1   &&
      pConstantData [0] == -1.0f / 512.0f &&
      pConstantData [1] ==  1.0f / 256.0f &&
      config.render.postproc_ratio > 0.0f) {
    if (SUCCEEDED (This->GetRenderTarget (0, &tzf::RenderFix::pPostProcessSurface)))
      tzf::RenderFix::pPostProcessSurface->Release ();

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    float scale_x, scale_y;
    float x_off,   y_off;
    scale_x = 1.0f; scale_y = 1.0f;
    x_off   = 0.0f;   y_off = 0.0f;
    //TZF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

    newData [0] = -1.0f / ((float)tzf::RenderFix::width  * config.render.postproc_ratio * scale_x);
    newData [1] =  1.0f / ((float)tzf::RenderFix::height * config.render.postproc_ratio * scale_y);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log.Log (L" Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
  }

  //
  // Env Shadow
  //
  if (StartRegister == 240 && Vector4fCount == 1) {
    uint32_t shift;
    uint32_t dim = 0;

    if (pConstantData [0] == -1.0f / 512.0f) {
      dim = 512UL;
      //dll_log.Log (L" 512x512 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksums [g_pVS], ps_checksums [g_pPS]);
    }

    if (pConstantData [0] == -1.0f / 1024.0f) {
      dim = 1024UL;
      //dll_log.Log (L" 1024x1024 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksums [g_pVS], ps_checksums [g_pPS]);
    }

    if (pConstantData [0] == -1.0f / 2048.0f) {
      dim = 2048UL;
      //dll_log.Log (L" 2048x2048 Shadow: VS CRC: %lu, PS CRC: %lu", vs_checksums [g_pVS], ps_checksums [g_pPS]);
    }

    shift = config.render.env_shadow_rescale;

    float newData [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    newData [0] = -1.0f / (dim << shift);
    newData [1] =  1.0f / (dim << shift);

    if (pConstantData [2] != 0.0f || 
        pConstantData [3] != 0.0f) {
      dll_log.Log (L" Assertion failed: non-zero 2 or 3 (line %lu)", __LINE__);
    }

    if (dim != 0) {
      return D3D9SetVertexShaderConstantF_Original (This, 240, newData, 1);
    }
  }


  //
  // Model Shadows and Post-Processing
  //
  if (StartRegister == 0 && (Vector4fCount == 2 || Vector4fCount == 3)) {
    IDirect3DSurface9* pSurf = nullptr;

    if (This == tzf::RenderFix::pDevice && SUCCEEDED (This->GetRenderTarget (0, &pSurf)) && pSurf != nullptr) {
      D3DSURFACE_DESC desc;
      pSurf->GetDesc (&desc);
      pSurf->Release ();

      //
      // Post-Processing
      //
      if (config.render.postproc_ratio > 0.0f) {
        if (desc.Width  == tzf::RenderFix::width  &&
            desc.Height == tzf::RenderFix::height) {
          if (pSurf == tzf::RenderFix::pPostProcessSurface) {
            float newData [12];

            float scale_x, scale_y;
            float x_off, y_off;

            scale_x = 1.0f; scale_y = 1.0f;
            x_off   = 0.0f;   y_off = 0.0f;

            //TZF_ComputeAspectScale (scale_x, scale_y, x_off, y_off);

            float rescale_x = 512.0f / ((float)tzf::RenderFix::width  * config.render.postproc_ratio * scale_x);
            float rescale_y = 256.0f / ((float)tzf::RenderFix::height * config.render.postproc_ratio * scale_y);

            for (int i = 0; i < 8; i += 2) {
              newData [i] = pConstantData [i] * rescale_x;
            }

            for (int i = 1; i < 8; i += 2) {
              newData [i] = pConstantData [i] * rescale_y;
            }

            return D3D9SetVertexShaderConstantF_Original (This, 0, newData, Vector4fCount);
          }
        }
      }

      //
      // Model Shadows
      //
      if (desc.Width == desc.Height) {
        float newData [12];

        uint32_t shift = TZF_MakeShadowBitShift (desc.Width);

        for (UINT i = 0; i < Vector4fCount * 4; i++) {
          newData [i] = pConstantData [i] / (float)(1 << shift);
        }

        return D3D9SetVertexShaderConstantF_Original (This, 0, newData, Vector4fCount);
      }
    }
  }

  return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



typedef HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF_t)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_t D3D9SetPixelShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShaderConstantF_Detour (IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount)
{
  return D3D9SetPixelShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



#define D3DX_DEFAULT ((UINT) -1)
struct D3DXIMAGE_INFO;

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileInMemoryEx_t)
(
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCVOID            pSrcData,
  _In_    UINT               SrcDataSize,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
  );

D3DXCreateTextureFromFileInMemoryEx_t
  D3DXCreateTextureFromFileInMemoryEx_Original = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateTextureFromFileInMemoryEx_Detour (
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCVOID            pSrcData,
  _In_    UINT               SrcDataSize,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
)
{
  UINT Levels = MipLevels;

  // Forcefully complete mipmap chains?
  if (config.render.complete_mipmaps && Levels > 1)
    Levels = D3DX_DEFAULT;

  HRESULT hr =
    D3DXCreateTextureFromFileInMemoryEx_Original (
      pDevice, pSrcData, SrcDataSize, Width, Height,
      Levels, Usage, Format, Pool,
      Filter, MipFilter, ColorKey, pSrcInfo, pPalette, ppTexture);

#ifdef DUMP_TEXTURES
  if (SUCCEEDED (hr)) {
    if (D3DXSaveTextureToFile == nullptr) {
      D3DXSaveTextureToFile =
        (D3DXSaveTextureToFile_t)
        GetProcAddress ( tzf::RenderFix::d3dx9_43_dll,
          "D3DXSaveTextureToFileW" );
    }

    if (D3DXSaveTextureToFile != nullptr) {
      wchar_t wszFileName [MAX_PATH] = { L'\0' };
      _swprintf ( wszFileName, L"textures\\%x.png",
                    pSrcData );
      D3DXSaveTextureToFile (wszFileName, D3DXIFF_PNG, *ppTexture, pPalette);
    }
  }
#endif

  return hr;
}

void
tzf::RenderFix::Init (void)
{
#if 0
  TZF_CreateDLLHook ( L"D3DX9_43.DLL", "D3DXMatrixMultiply", 
                      D3DXMatrixMultiply_Detour,
           (LPVOID *)&D3DXMatrixMultiply_Original );
#endif



#if 1
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetSamplerState_Override",
                      D3D9SetSamplerState_Detour,
            (LPVOID*)&D3D9SetSamplerState_Original,
                     &SetSamplerState );

  TZF_EnableHook (SetSamplerState);
#endif



#if 0
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetPixelShaderConstantF_Override",
                      D3D9SetPixelShaderConstantF_Detour,
            (LPVOID*)&D3D9SetPixelShaderConstantF_Original );
#endif




#if 1
  // Needed for shadow re-scaling
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetViewport_Override",
                      D3D9SetViewport_Detour,
            (LPVOID*)&D3D9SetViewport_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9DrawIndexedPrimitive_Override",
                      D3D9DrawIndexedPrimitive_Detour,
            (LPVOID*)&D3D9DrawIndexedPrimitive_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetVertexShaderConstantF_Override",
                      D3D9SetVertexShaderConstantF_Detour,
            (LPVOID*)&D3D9SetVertexShaderConstantF_Original );


  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetVertexShader_Override",
                      D3D9SetVertexShader_Detour,
            (LPVOID*)&D3D9SetVertexShader_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetPixelShader_Override",
                      D3D9SetPixelShader_Detour,
            (LPVOID*)&D3D9SetPixelShader_Original );

  // Needed for UI re-scaling
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetScissorRect_Override",
                      D3D9SetScissorRect_Detour,
            (LPVOID*)&D3D9SetScissorRect_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9EndScene_Override",
                      D3D9EndScene_Detour,
            (LPVOID*)&D3D9EndScene_Original );

  // Needed for shadow re-scaling
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9CreateTexture_Override",
                      D3D9CreateTexture_Detour,
            (LPVOID*)&D3D9CreateTexture_Original );
#endif




#if 1
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9CreateRenderTarget_Override",
                      D3D9CreateRenderTarget_Detour,
            (LPVOID*)&D3D9CreateRenderTarget_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9CreateDepthStencilSurface_Override",
                      D3D9CreateDepthStencilSurface_Detour,
            (LPVOID*)&D3D9CreateDepthStencilSurface_Original );

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9UpdateTexture_Override",
                      D3D9UpdateTexture_Detour,
            (LPVOID*)&D3D9UpdateTexture_Original );
#endif


  d3dx9_43_dll = LoadLibrary (L"D3DX9_43.DLL");
  user32_dll   = LoadLibrary (L"User32.dll");

  // Needed for mipmap completeness
  TZF_CreateDLLHook ( L"D3DX9_43.DLL", "D3DXCreateTextureFromFileInMemoryEx",
                      D3DXCreateTextureFromFileInMemoryEx_Detour,
           (LPVOID *)&D3DXCreateTextureFromFileInMemoryEx_Original );


  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_BeginBufferSwap",
                      D3D9EndFrame_Pre,
            (LPVOID*)&BMF_BeginBufferSwap );

  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_EndBufferSwap",
                      D3D9EndFrame_Post,
            (LPVOID*)&BMF_EndBufferSwap );



#if 0
  UINT_PTR addr = (UINT_PTR)GetModuleHandle(L"Tales of Zestiria.exe");
  HANDLE hProc = GetCurrentProcess ();

  float* fTest = (float *)addr;
  while (true) {
    if (*fTest < (1.0 / 60.0) + 0.0001 && *fTest > (1.0 / 60.0) - 0.0001)
      dll_log.Log (L"Frame Delta: Address=%08Xh",
                     //fTest);
#if 0
    if (*fTest < 1.777778f + 0.001f && *fTest > 1.777778f - 0.001f) {
      dll_log.Log (L"Aspect Ratio: Address=%08Xh",
                   fTest);
    }

    if (*fTest < 0.785398f + 0.001f && *fTest > 0.785398f - 0.001f) {
      dll_log.Log (L"FOVY: Address=%08Xh",
                   fTest);
    }
#endif
    ++fTest;
  }
#endif

#if 0
    float* fTest = (float *)0x00D93690;

    for (int i = 0; i < 1000; fTest++) {
      dll_log.Log (L"%p, %f", fTest, *fTest);
    }
#endif

  CommandProcessor* comm_proc = CommandProcessor::getInstance ();

#if 0
  if (config.render.aspect_ratio != 1.777778f) {
    eTB_VarStub <float>* aspect =
   (eTB_VarStub <float>*)command.FindVariable ("AspectRatio");
    aspect->setValue (config.render.aspect_ratio);
  }

  if (config.render.fovy != 0.785398f) {
    eTB_VarStub <float>* fovy =
   (eTB_VarStub <float>*)command.FindVariable ("FOVY");
    fovy->setValue (config.render.fovy);
  }
#endif
}

void
tzf::RenderFix::Shutdown (void)
{
  FreeLibrary (d3dx9_43_dll);
  //TZF_RemoveHook (SetSamplerState);
}

tzf::RenderFix::CommandProcessor::CommandProcessor (void)
{
  fovy_         = new eTB_VarStub <float> (&config.render.fovy,         this);
  aspect_ratio_ = new eTB_VarStub <float> (&config.render.aspect_ratio, this);

  eTB_Variable* aspect_correct_vids = new eTB_VarStub <bool>  (&config.render.blackbar_videos);
  eTB_Variable* aspect_correction   = new eTB_VarStub <bool>  (&config.render.aspect_correction);

  eTB_Variable* complete_mipmaps    = new eTB_VarStub <bool>  (&config.render.complete_mipmaps);
  eTB_Variable* rescale_shadows     = new eTB_VarStub <int>   (&config.render.shadow_rescale);
  eTB_Variable* rescale_env_shadows = new eTB_VarStub <int>   (&config.render.env_shadow_rescale);
  eTB_Variable* postproc_ratio      = new eTB_VarStub <float> (&config.render.postproc_ratio);
  eTB_Variable* prelimit            = new eTB_VarStub <bool>  (&pre_limit);
  eTB_Variable* clear_blackbars     = new eTB_VarStub <bool>  (&config.render.clear_blackbars);

  command.AddVariable ("AspectRatio",         aspect_ratio_);
  command.AddVariable ("FOVY",                fovy_);

  command.AddVariable ("AspectCorrectVideos", aspect_correct_vids);
  command.AddVariable ("AspectCorrection",    aspect_correction);
  command.AddVariable ("CompleteMipmaps",     complete_mipmaps);
  command.AddVariable ("RescaleShadows",      rescale_shadows);
  command.AddVariable ("RescaleEnvShadows",   rescale_env_shadows);
  command.AddVariable ("PostProcessRatio",    postproc_ratio);
  command.AddVariable ("PreLimitFPS",         prelimit);
  command.AddVariable ("ClearBlackbars",      clear_blackbars);

   uint8_t signature [] = { 0x39, 0x8E, 0xE3, 0x3F,
                            0xDB, 0x0F, 0x49, 0x3F };

  if (*(float *)config.render.aspect_addr != 16.0f / 9.0f) {
    void* addr = TZF_Scan (signature, sizeof (float) * 2, nullptr);
    if (addr != nullptr) {
      dll_log.Log (L"Scanned Aspect Ratio Address: %06Xh", addr);
      config.render.aspect_addr = (DWORD)addr;
      dll_log.Log (L"Scanned FOVY Address: %06Xh", (float *)addr + 1);
      config.render.fovy_addr = (DWORD)((float *)addr + 1);
    }
    else {
      dll_log.Log (L" >> ERROR: Unable to find Aspect Ratio Address!");
    }
  }
}

bool
tzf::RenderFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  DWORD dwOld;

  if (var == aspect_ratio_) {
    VirtualProtect ((LPVOID)config.render.aspect_addr, 4, PAGE_READWRITE, &dwOld);
    float original = *((float *)config.render.aspect_addr);

    if (((original < 1.777778f + 0.01f && original > 1.777778f - 0.01f) ||
         (original == config.render.aspect_ratio))
            && val != nullptr) {
      config.render.aspect_ratio = *(float *)val;

      if (original != config.render.aspect_ratio) {
        dll_log.Log ( L" * Changing Aspect Ratio from %f to %f",
                         original,
                          config.render.aspect_ratio );
       }

       *((float *)config.render.aspect_addr) = config.render.aspect_ratio;
    }
    else {
      if (val != nullptr)
        dll_log.Log ( L" * Unable to change Aspect Ratio, invalid memory address... (%f)",
                        *((float *)config.render.aspect_addr) );
    }
  }

  if (var == fovy_) {
    VirtualProtect ((LPVOID)config.render.fovy_addr, 4, PAGE_READWRITE, &dwOld);
    float original = *((float *)config.render.fovy_addr);

    if (((original < 0.785398f + 0.01f && original > 0.785398f - 0.01f) ||
         (original == config.render.fovy))
            && val != nullptr) {
      config.render.fovy = *(float *)val;
      dll_log.Log ( L" * Changing FOVY from %f to %f",
                       original,
                         config.render.fovy );
      *((float *)config.render.fovy_addr) = config.render.fovy;
    }
    else {
      if (val != nullptr)
        dll_log.Log ( L" * Unable to change FOVY, invalid memory address... (%f)",
                        *((float *)config.render.fovy_addr) );
    }
  }

  return true;
}


tzf::RenderFix::CommandProcessor* tzf::RenderFix::CommandProcessor::pCommProc;

HWND              tzf::RenderFix::hWndDevice = NULL;
IDirect3DDevice9* tzf::RenderFix::pDevice    = nullptr;

uint32_t tzf::RenderFix::width            = 0UL;
uint32_t tzf::RenderFix::height           = 0UL;
uint32_t tzf::RenderFix::dwRenderThreadID = 0UL;

IDirect3DSurface9* tzf::RenderFix::pPostProcessSurface = nullptr;
bool               tzf::RenderFix::bink                = false;

HMODULE            tzf::RenderFix::d3dx9_43_dll        = 0;
HMODULE            tzf::RenderFix::user32_dll          = 0;