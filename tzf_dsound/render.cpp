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
#include "config.h"
#include "log.h"

#include <stdint.h>

#include <d3d9.h>
#include <d3d9types.h>

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
  if (pM1->_11 == 2.0f/1920.0f &&
      pM1->_22 == 2.0f/1080.0f &&
      pM1->_44 == 1.0f)
    dll_log.Log (L"Ortho Matrix (1)");

  if (pM2->_11 == 2.0f/1920.0f &&
      pM2->_22 == 2.0f/1080.0f &&
      pM2->_44 == 1.0f)
    dll_log.Log (L"Ortho Matrix (2)");

#if 0
//  if (pM2->_44 == 0.0f ||
      //pM1->_44 == 0.0f) {
//    dll_log.Log (L" [!] D3DMatrixMultiply (...) ");

    if (pM2->_44 == 0.0f &&
        pM2->_34 == -1.0f) {
      dll_log.Log (L"Right-Handed Perspective Projection Matrix(R) Detected: FOVY=%f\n",
                   tan (pM2->_22 * 2.0f));
    }

    if (pM2->_44 == 0.0f &&
        pM2->_34 == 1.0f) {
      dll_log.Log (L"Left-Handed Perspective Projection Matrix(R) Detected: FOVY=%f\n",
                   tan (pM2->_22 * 2.0f));
    }

    if (pM1->_44 == 0.0f &&
        pM1->_34 == -1.0f) {
      dll_log.Log (L"Right-Handed Perspective Projection Matrix(L) Detected: FOVY=%f\n",
                   tan (pM1->_22 * 2.0f));
    }

    if (pM1->_44 == 0.0f &&
        pM1->_34 == 1.0f) {
      dll_log.Log (L"Left-Handed Perspective Projection Matrix(L) Detected: FOVY=%f\n",
                   tan (pM1->_22 * 2.0f));
    }


    //if (tan (pM1->_22 * 2.0f) > 52.5f &&
        //tan (pM1->_22 * 2.0f) < 53.0f) {
    float fovy = tan ((pM1->_22 * 2.0f))  * (3.14159265f / 2.0f);
    if (fovy > 104 && fovy < 107)
      fovy = 70;

    pM1->_22 = atan (fovy / 2.0f * 2.0f * 3.14159265f);
    //dll_log.Log (L"FOVY1=%f\n",
                 //);
#endif

    //::tanpM1->_22 * 2.0f
  //}
//    dll_log.Log (L"FOVY2=%f\n",
//                 tan ((pM2->_22 * 2.0f)) * (3.14159265f / 2.0f));
  //}

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
  if (Type == D3DSAMP_MIPFILTER ||
      Type == D3DSAMP_MINFILTER ||
      Type == D3DSAMP_MAGFILTER ||
      Type == D3DSAMP_MIPMAPLODBIAS) {
    //dll_log.Log (L" [!] IDirect3DDevice9::SetSamplerState (...)");

    if (Type < 8) {
      //dll_log.Log (L" %s Filter: %x", Type == D3DSAMP_MIPFILTER ? L"Mip" : Type == D3DSAMP_MINFILTER ? L"Min" : L"Mag", Value);
      if (Type == D3DSAMP_MIPFILTER) {
        if (Value != D3DTEXF_NONE)
          Value = D3DTEXF_LINEAR;
      }
      if (Type == D3DSAMP_MAGFILTER ||
                  D3DSAMP_MINFILTER)
        Value = D3DTEXF_ANISOTROPIC;
    } else {
#if 0
      float bias = *((float *)(&Value));
      if (bias > 3.0f)
        bias = 3.0f;
      if (bias < -3.0f)
        bias = -3.0f;
      Value = *((LPDWORD)(&bias));
#endif
      //dll_log.Log (L" Mip Bias: %f", (float)Value);
    }
  }
  if (Type == D3DSAMP_MAXANISOTROPY)
    Value = 16;
  //if (Type == D3DSAMP_MAXMIPLEVEL)
    //Value = 0;
  return D3D9SetSamplerState_Original (This, Sampler, Type, Value);
}

IDirect3DVertexShader9* g_pVS;

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_t)(
  IDirect3DDevice9* This,
  UINT              StartRegister,
  CONST float*      pConstantData,
  UINT              Vector4fCount);

SetVertexShaderConstantF_t D3D9SetVertexShaderConstantF_Original = nullptr;

static uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
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

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_t)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

SetVertexShader_t D3D9SetVertexShader_Original = nullptr;

#include <map>
std::map <LPVOID, uint32_t> vs_checksums;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Detour (IDirect3DDevice9*       This,
                            IDirect3DVertexShader9* pShader)
{
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

//
// Bink Video Vertex Shader
//
const int VS_CHECKSUM_BINK = -831857998;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Detour (IDirect3DDevice9* This,
                                     UINT              StartRegister,
                                     CONST float*      pConstantData,
                                     UINT              Vector4fCount)
{
  if (config.render.letterbox_videos) {
  if (Vector4fCount == 5 && StartRegister == 0) {
    if (pConstantData [ 0] == 0.0015625f    && pConstantData [ 1] == 0.0f          && pConstantData [ 2] == 0.0f     && pConstantData [ 3] == 0.0f &&
        pConstantData [ 4] == 0.0f          && pConstantData [ 5] == 0.0027777778f && pConstantData [ 6] == 0.0f     && pConstantData [ 7] == 0.0f &&
        pConstantData [ 8] == 0.0f          && pConstantData [ 9] == 0.0f          && pConstantData [10] == 0.00005f && pConstantData [11] == 0.0f &&
        pConstantData [12] == 0.0f          && pConstantData [13] == 0.0f          && pConstantData [14] == 0.5f     && pConstantData [15] == 1.0f &&
        pConstantData [16] == 1.0f          && pConstantData [17] == 1.0f          && pConstantData [18] == 1.0f     && pConstantData [19] == 1.0f) {
      D3DVIEWPORT9 vp9_orig;
      This->GetViewport (&vp9_orig);

      int width = vp9_orig.Width;
      int height = (9.0f / 16.0f) * vp9_orig.Width;

      //dll_log.Log (L"checksum: %d", vs_crc32);
      //dll_log.Log (L" Viewport: (%dx%d) -> (%dx%d)", vp9_orig.Width, vp9_orig.Height,
                                                     //width, height);

     if (height != vp9_orig.Height && (vs_checksums [g_pVS] == VS_CHECKSUM_BINK)) {
        This->Clear (0, NULL, D3DCLEAR_TARGET, 0x0, 0.0f, 0x0);

        D3DVIEWPORT9 vp9;
        vp9.X     = vp9_orig.X;    vp9.Y      = vp9_orig.Y + (vp9_orig.Height - height) / 2;
        vp9.Width = width;         vp9.Height = height;
        vp9.MinZ  = vp9_orig.MinZ; vp9.MaxZ   = vp9_orig.MaxZ;
        This->SetViewport (&vp9);
      }
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


void
tzf::RenderFix::Init (void)
{
#if 0
  TZF_CreateDLLHook ( L"D3DX9_43.DLL", "D3DXMatrixMultiply", 
                      D3DXMatrixMultiply_Detour,
           (LPVOID *)&D3DXMatrixMultiply_Original );
#endif

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetSamplerState_Override",
                      D3D9SetSamplerState_Detour,
            (LPVOID*)&D3D9SetSamplerState_Original,
                     &SetSamplerState );

#if 0
  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetPixelShaderConstantF_Override",
                      D3D9SetPixelShaderConstantF_Detour,
            (LPVOID*)&D3D9SetPixelShaderConstantF_Original );
#endif

  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetVertexShaderConstantF_Override",
                      D3D9SetVertexShaderConstantF_Detour,
            (LPVOID*)&D3D9SetVertexShaderConstantF_Original );


  TZF_CreateDLLHook ( L"d3d9.dll", "D3D9SetVertexShader_Override",
                      D3D9SetVertexShader_Detour,
            (LPVOID*)&D3D9SetVertexShader_Original );


  UINT_PTR addr = (UINT_PTR)GetModuleHandle(L"Tales of Zestiria.exe");
  HANDLE hProc = GetCurrentProcess ();

#if 0
  float* fTest = (float *)addr;
  while (true) {
    if (*fTest < 1.777778f + 0.001f && *fTest > 1.777778f - 0.001f) {
      dll_log.Log (L"Aspect Ratio: Address=%08Xh",
                   fTest);
    }

    if (*fTest < 0.785398f + 0.001f && *fTest > 0.785398f - 0.001f) {
      dll_log.Log (L"FOVY: Address=%08Xh",
                   fTest);
    }
    ++fTest;
  }
#endif

#if 0
    float* fTest = ((float *)addr);
    while (true) {
      if (*fTest == 960.0f)
        dll_log.Log (L"%p, 960", fTest);

      if (*fTest == 540.0f)
        dll_log.Log (L"%p, 540", fTest);

      fTest++;
    }
#endif

#if 0
    while (true) {
      if (*(float *)addr == 1920.0f) {
        dll_log.Log (L"1920f: %08Xh", addr);
      }
      if (*(int *)addr == 1920) {
        dll_log.Log (L"1920lu: %08Xh", addr);
      }
      if (*(float *)addr == 1080.0f) {
        dll_log.Log (L"1080f: %08Xh", addr);
      }
      if (*(int *)addr == 1080) {
        dll_log.Log (L"1080lu: %08Xh", addr);
      }
      addr++;
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
  TZF_RemoveHook (SetSamplerState);
}

tzf::RenderFix::CommandProcessor::CommandProcessor (void)
{
  fovy_         = new eTB_VarStub <float> (&config.render.fovy,         this);
  aspect_ratio_ = new eTB_VarStub <float> (&config.render.aspect_ratio, this);

  eTB_Variable* aspect_correct_vids = new eTB_VarStub <bool> (&config.render.letterbox_videos);

  command.AddVariable ("AspectRatio",         aspect_ratio_);
  command.AddVariable ("FOVY",                fovy_);
  command.AddVariable ("AspectCorrectVideos", aspect_correct_vids);
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
       dll_log.Log ( L" * Changing Aspect Ratio from %f to %f",
                        original,
                         config.render.aspect_ratio );
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