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
 *S
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/
#ifndef __TZFIX__TEXTURES_H__
#define __TZFIX__TEXTURES_H__

#define NOMINMAX

#include "log.h"
extern iSK_Logger* tex_log;

#include "render.h"
#include <d3d9.h>

#include <set>
#include <map>
#include <limits>
#include <cstdint>
#include <algorithm>

interface ISKTextureD3D9;


typedef enum D3DXIMAGE_FILEFORMAT {
  D3DXIFF_BMP = 0,
  D3DXIFF_JPG = 1,
  D3DXIFF_TGA = 2,
  D3DXIFF_PNG = 3,
  D3DXIFF_DDS = 4,
  D3DXIFF_PPM = 5,
  D3DXIFF_DIB = 6,
  D3DXIFF_HDR = 7,
  D3DXIFF_PFM = 8,
  D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

#define D3DX_DEFAULT ((UINT) -1)
typedef struct D3DXIMAGE_INFO {
  UINT                 Width;
  UINT                 Height;
  UINT                 Depth;
  UINT                 MipLevels;
  D3DFORMAT            Format;
  D3DRESOURCETYPE      ResourceType;
  D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO, *LPD3DXIMAGE_INFO;


struct tzf_tex_thread_stats_s {
  ULONGLONG bytes_loaded;
  LONG      jobs_retired;

  struct {
    FILETIME start, end;
    FILETIME user,  kernel;
    FILETIME idle; // Computed: (now - start) - (user + kernel)
  } runtime;
};


namespace tzf {
namespace RenderFix {
#if 0
  class Sampler {
    int                id;
    IDirect3DTexture9* pTex;
  };
#endif

    struct tzf_draw_states_s {
      bool         has_aniso      = false; // Has he game even once set anisotropy?!
      int          max_aniso      = 4;
      bool         has_msaa       = false;
      bool         use_msaa       = true;  // Allow MSAA toggle via console
                                           //  without changing the swapchain.
      D3DVIEWPORT9 vp             = { 0 };
      bool         postprocessing = false;
      bool         fullscreen     = false;

      DWORD        srcblend       = 0;
      DWORD        dstblend       = 0;
      DWORD        srcalpha       = 0;     // Separate Alpha Blend Eq: Src
      DWORD        dstalpha       = 0;     // Separate Alpha Blend Eq: Dst
      bool         alpha_test     = false; // Test Alpha?
      DWORD        alpha_ref      = 0;     // Value to test.
      bool         zwrite         = false; // Depth Mask

      int          last_vs_vec4   = 0; // Number of vectors in the last call to
                                       //   set vertex shader constant...

      int          draws          = 0; // Number of draw calls
      int          frames         = 0;
      bool         cegui_active   = false;
    } extern draw_state;


  extern std::set <UINT> active_samplers;

  class Texture {
  public:
    Texture (void) {
      crc32     = 0;
      size      = 0;
      refs      = 0;
      load_time = 0.0f;
      d3d9_tex  = nullptr;
    }

    uint32_t        crc32;
    size_t          size;
    int             refs;
    float           load_time;
    ISKTextureD3D9* d3d9_tex;
  };

  struct frame_texture_t {
    const uint32_t         crc32_corner = 0x6465f296;
    const uint32_t         crc32_side   = 0xace25896;

    IDirect3DBaseTexture9* tex_corner   = (IDirect3DBaseTexture9 *)1;
    IDirect3DBaseTexture9* tex_side     = (IDirect3DBaseTexture9 *)1;

    bool in_use                         = false;
  } extern cutscene_frame;

  struct pad_buttons_t {
    const uint32_t     crc32_ps3  = 0x8D7A5256;
    const uint32_t     crc32_xbox = 0x74F5352D;

    IDirect3DTexture9* tex_ps3    = nullptr;
    IDirect3DTexture9* tex_xbox   = nullptr;
  } extern pad_buttons;


  class TextureManager {
  public:
    void Init     (void);
    void Hook     (void);
    void Shutdown (void);

    void                     removeTexture   (ISKTextureD3D9* pTexD3D9);

    tzf::RenderFix::Texture* getTexture (uint32_t crc32);
    void                     addTexture (uint32_t crc32, tzf::RenderFix::Texture* pTex, size_t size);

    bool                     reloadTexture (uint32_t crc32);

    // Record a cached reference
    void                     refTexture  (tzf::RenderFix::Texture* pTex);

    // Similar, just call this to indicate a cache miss
    void                     missTexture (void) {
      InterlockedIncrement (&misses);
    }

    void                     reset (void);
    void                     purge (void); // WIP

    size_t                   numTextures (void) {
      return textures.size ();
    }
    int                      numInjectedTextures (void);

    int64_t                  cacheSizeTotal    (void);
    int64_t                  cacheSizeBasic    (void);
    int64_t                  cacheSizeInjected (void);

    int                      numMSAASurfs (void);

    void                     addInjected (size_t size) {
      InterlockedIncrement (&injected_count);
      InterlockedAdd64     (&injected_size, size);
    }

    std::string              osdStats  (void) { return osd_stats; }
    void                     updateOSD (void);


    float                    getTimeSaved (void) { return time_saved;                               }
    LONG64                   getByteSaved (void) { return InterlockedAdd64       (&bytes_saved, 0); }
    ULONG                    getHitCount  (void) { return InterlockedExchangeAdd (&hits,   0UL);    }
    ULONG                    getMissCount (void) { return InterlockedExchangeAdd (&misses, 0UL);    }


    void                     resetUsedTextures (void);
    void                     applyTexture      (IDirect3DBaseTexture9* tex);

    std::vector <IDirect3DBaseTexture9 *>
                             getUsedRenderTargets (void);
    uint32_t                 getRenderTargetCreationTime 
                                                  (IDirect3DBaseTexture9* rt);
    void                     trackRenderTarget    (IDirect3DBaseTexture9* rt);
    bool                     isRenderTarget       (IDirect3DBaseTexture9* rt);
    bool                     isUsedRenderTarget   (IDirect3DBaseTexture9* rt);

    void                     queueScreenshot      (wchar_t* wszFileName, bool hudless = true);
    bool                     wantsScreenshot      (void);
    HRESULT                  takeScreenshot       (IDirect3DSurface9* pSurf);

    std::vector<tzf_tex_thread_stats_s>
                             getThreadStats       (void);



    BOOL                     isTexturePowerOfTwo (UINT sampler)
    {
      return sampler_flags [sampler < 255 ? sampler : 255].power_of_two;
    }

    // Sttate of the active texture for each sampler,
    //   needed to correct some texture coordinate address
    //     problems in the base game.
    struct
    {
      BOOL power_of_two;
    } sampler_flags [256] = { 0 };


  private:
    struct {
      // In lieu of actually wrapping render targets with a COM interface, just add the creation time
      //   as the mapped parameter
      std::unordered_map <IDirect3DBaseTexture9 *, uint32_t> render_targets;
    } known;
    
    struct {
      std::unordered_set <IDirect3DBaseTexture9 *> render_targets;
    } used;

    std::unordered_map <uint32_t, tzf::RenderFix::Texture*> textures;
    float                                                   time_saved      = 0.0f;
    LONG64                                                  bytes_saved     = 0LL;

    ULONG                                                   hits            = 0UL;
    ULONG                                                   misses          = 0UL;

    LONG64                                                  basic_size      = 0LL;
    LONG64                                                  injected_size   = 0LL;
    ULONG                                                   injected_count  = 0UL;

    std::string                                             osd_stats       = "";
    bool                                                    want_screenshot = false;

    CRITICAL_SECTION                                        cs_cache;
  } extern tex_mgr;
}
}

#pragma comment (lib, "dxguid.lib")

const GUID IID_SKTextureD3D9 = { 0xace1f81b, 0x5f3f, 0x45f4, 0xbf, 0x9f, 0x1b, 0xaf, 0xdf, 0xba, 0x11, 0x9b };

interface ISKTextureD3D9 : public IDirect3DTexture9
{
public:
     ISKTextureD3D9 (IDirect3DTexture9 **ppTex, SIZE_T size, uint32_t crc32) {
         pTexOverride  = nullptr;
         can_free      = true;
         override_size = 0;
         last_used.QuadPart
                       = 0ULL;
         pTex          = *ppTex;
       *ppTex          =  this;
         tex_size      = size;
         tex_crc32     = crc32;
         must_block    = false;
         refs          =  1;
     };

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) {
      if (IsEqualGUID (riid, IID_SKTextureD3D9)) {
        return S_OK;
      }

#if 1
      if ( IsEqualGUID (riid, IID_IUnknown)              ||
           IsEqualGUID (riid, IID_IDirect3DTexture9)     ||
           IsEqualGUID (riid, IID_IDirect3DBaseTexture9)    )
      {
        AddRef ();
        *ppvObj = this;
        return S_OK;
      }

      return E_FAIL;
#else
      return pTex->QueryInterface (riid, ppvObj);
#endif
    }
    STDMETHOD_(ULONG,AddRef)(THIS) {
      ULONG ret = InterlockedIncrement (&refs);

      can_free = false;

      return ret;
    }
    STDMETHOD_(ULONG,Release)(THIS) {
      ULONG ret = InterlockedDecrement (&refs);

      if (ret == 1) {
        can_free = true;
      }

      if (ret == 0) {
        // Does not delete this immediately; defers the
        //   process until the next cached texture load.
        tzf::RenderFix::tex_mgr.removeTexture (this);
      }

      return ret;
    }

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) {
      tex_log->Log (L"[ Tex. Mgr ] ISKTextureD3D9::GetDevice (%ph)", ppDevice);
      return pTex->GetDevice (ppDevice);
    }
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPrivateData (%x, %ph, %lu, %x)",
                       refguid,
                         pData,
                           SizeOfData,
                             Flags );
      return pTex->SetPrivateData (refguid, pData, SizeOfData, Flags);
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPrivateData (%x, %ph, %lu)",
                       refguid,
                         pData,
                           *pSizeOfData );

      return pTex->GetPrivateData (refguid, pData, pSizeOfData);
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::FreePrivateData (%x)",
                       refguid );

      return pTex->FreePrivateData (refguid);
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPriority (%lu)",
                       PriorityNew );

      return pTex->SetPriority (PriorityNew);
    }
    STDMETHOD_(DWORD, GetPriority)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPriority ()" );

      return pTex->GetPriority ();
    }
    STDMETHOD_(void, PreLoad)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::PreLoad ()" );

      pTex->PreLoad ();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetType ()" );

      return pTex->GetType ();
    }
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetLOD (%lu)",
                       LODNew );

      return pTex->SetLOD (LODNew);
    }
    STDMETHOD_(DWORD, GetLOD)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLOD ()" );

      return pTex->GetLOD ();
    }
    STDMETHOD_(DWORD, GetLevelCount)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelCount ()" );

      return pTex->GetLevelCount ();
    }
    STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetAutoGenFilterType (%x)",
                       FilterType );

      return pTex->SetAutoGenFilterType (FilterType);
    }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetAutoGenFilterType ()" );

      return pTex->GetAutoGenFilterType ();
    }
    STDMETHOD_(void, GenerateMipSubLevels)(THIS) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GenerateMipSubLevels ()" );

      pTex->GenerateMipSubLevels ();
    }
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelDesc (%lu, %ph)",
                      Level,
                        pDesc );
      return pTex->GetLevelDesc (Level, pDesc);
    }
    STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetSurfaceLevel (%lu, %ph)",
                       Level,
                         ppSurfaceLevel );

      return pTex->GetSurfaceLevel (Level, ppSurfaceLevel);
    }
    STDMETHOD(LockRect)(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::LockRect (%lu, %ph, %ph, %x)",
                       Level,
                         pLockedRect,
                           pRect,
                             Flags );

      return pTex->LockRect (Level, pLockedRect, pRect, Flags);
    }
    STDMETHOD(UnlockRect)(THIS_ UINT Level) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::UnlockRect (%lu)", Level );

      return pTex->UnlockRect (Level);
    }
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect) {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetDirtyRect (...)" );

      return pTex->AddDirtyRect (pDirtyRect);
    }

    bool               can_free;      // Whether or not we can free this texture
    bool               must_block;    // Whether or not to draw using this texture before its
                                      //  override finishes streaming

    IDirect3DTexture9* pTex;          // The original texture data
    SSIZE_T            tex_size;      //   Original data size
    uint32_t           tex_crc32;     //   Original data checksum

    IDirect3DTexture9* pTexOverride;  // The overridden texture data (nullptr if unchanged)
    SSIZE_T            override_size; //   Override data size

    ULONG              refs;
    LARGE_INTEGER      last_used;     // The last time this texture was used (for rendering)
                                      //   different from the last time referenced, this is
                                      //     set when SetTexture (...) is called.
};

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileInMemoryEx_pfn)
(
  _In_        LPDIRECT3DDEVICE9  pDevice,
  _In_        LPCVOID            pSrcData,
  _In_        UINT               SrcDataSize,
  _In_        UINT               Width,
  _In_        UINT               Height,
  _In_        UINT               MipLevels,
  _In_        DWORD              Usage,
  _In_        D3DFORMAT          Format,
  _In_        D3DPOOL            Pool,
  _In_        DWORD              Filter,
  _In_        DWORD              MipFilter,
  _In_        D3DCOLOR           ColorKey,
  _Inout_opt_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_opt_   PALETTEENTRY       *pPalette,
  _Out_       LPDIRECT3DTEXTURE9 *ppTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXSaveTextureToFile_pfn)(
  _In_           LPCWSTR                 pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT    DestFormat,
  _In_           LPDIRECT3DBASETEXTURE9  pSrcTexture,
  _In_opt_ const PALETTEENTRY           *pSrcPalette
);

typedef HRESULT (WINAPI *D3DXSaveSurfaceToFile_pfn)
(
  _In_           LPCWSTR               pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT  DestFormat,
  _In_           LPDIRECT3DSURFACE9    pSrcSurface,
  _In_opt_ const PALETTEENTRY         *pSrcPalette,
  _In_opt_ const RECT                 *pSrcRect
);

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_pfn)
(
  IDirect3DDevice9   *This,
  UINT                Width,
  UINT                Height,
  UINT                Levels,
  DWORD               Usage,
  D3DFORMAT           Format,
  D3DPOOL             Pool,
  IDirect3DTexture9 **ppTexture,
  HANDLE             *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Lockable,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Discard,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *SetTexture_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  Sampler,
  _In_ IDirect3DBaseTexture9 *pTexture
);

typedef HRESULT (STDMETHODCALLTYPE *SetRenderTarget_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  RenderTargetIndex,
  _In_ IDirect3DSurface9     *pRenderTarget
);

typedef HRESULT (STDMETHODCALLTYPE *SetDepthStencilSurface_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ IDirect3DSurface9     *pNewZStencil
);


HRESULT
TZF_DumpTexture (D3DFORMAT fmt, uint32_t checksum, IDirect3DTexture9* pTex);

bool
TZF_IsTextureDumped (uint32_t checksum);

bool
TZF_DeleteDumpedTexture (D3DFORMAT fmt, uint32_t checksum);

enum tzf_load_method_t {
  Streaming,
  Blocking,
  DontCare
};

struct tzf_tex_record_s {
  unsigned int               archive = std::numeric_limits <unsigned int>::max ();
           int               fileno  = 0UL;
  enum     tzf_load_method_t method  = DontCare;
           size_t            size    = 0UL;
};

std::vector <std::wstring>
TZF_GetTextureArchives (void);

std::vector < std::pair < uint32_t, tzf_tex_record_s > >
TZF_GetInjectableTextures (void);

tzf_tex_record_s*
TZF_GetInjectableTexture (uint32_t checksum);

void
TZF_RefreshDataSources (void);


#endif /* __TZFIX__TEXTURES_H__ */