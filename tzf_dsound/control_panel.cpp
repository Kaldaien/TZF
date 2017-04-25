// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#define NOMINMAX

#include "imgui/imgui.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "config.h"
#include "render.h"
#include "textures.h"
#include "framerate.h"
#include "hook.h"
#include "sound.h"

#include <string>
#include <vector>
#include <algorithm>


__declspec (dllimport)
bool
SK_ImGui_ControlPanel (void);


bool show_special_k_cfg   = false;
bool show_texture_mod_dlg = false;
bool show_test_window     = false;

ImVec4 clear_col = ImColor(114, 144, 154);

struct {
  std::vector <const char*> array;
  int                       sel   = 0;
} gamepads;

void
__TZF_DefaultSetOverlayState (bool)
{
}

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <atlbase.h>

IAudioMeterInformation*
TZFix_GetAudioMeterInfo (void)
{
  CComPtr <IMMDeviceEnumerator> pDevEnum = nullptr;

  if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
    return nullptr;

  CComPtr <IMMDevice> pDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eConsole,
                                                 &pDevice )
              )
     ) return nullptr;

  IAudioMeterInformation* pMeterInfo = nullptr;

  HRESULT hr = pDevice->Activate ( __uuidof (IAudioMeterInformation),
                                     CLSCTX_ALL,
                                       nullptr,
                                         IID_PPV_ARGS_Helper (&pMeterInfo) );

  if (SUCCEEDED (hr))
    return pMeterInfo;

  return nullptr;
}

void
TZFix_PauseGame (bool pause)
{
  typedef void (__stdcall *SK_SteamAPI_SetOverlayState_pfn)(bool active);

  static SK_SteamAPI_SetOverlayState_pfn
    SK_SteamAPI_SetOverlayState =
      (SK_SteamAPI_SetOverlayState_pfn)

    TZF_ImportFunctionFromSpecialK (
      "SK_SteamAPI_SetOverlayState",
        &__TZF_DefaultSetOverlayState
    );

  SK_SteamAPI_SetOverlayState (pause);
}

bool reset_frame_history = false;
int  cursor_refs         = 0;

typedef DWORD (*SK_ImGui_Toggle_pfn)(void);
SK_ImGui_Toggle_pfn SK_ImGui_Toggle_Original = nullptr;

void
TZFix_ToggleConfigUI (void)
{
  SK_ImGui_Toggle_Original ();

  reset_frame_history = true;

  bool* visible =
    (bool *)SK_GetCommandProcessor ()->ProcessCommandLine ("ImGui.Visible").getVariable()->getValuePointer ();

  config.control_panel.visible = *visible;

  TZF_SaveConfig ();
}

extern
bool
TZFix_TextureModDlg (void);

void
TZFix_GamepadConfigDlg (void)
{
  if (gamepads.array.size () == 0)
  {
    if (GetFileAttributesA ("TZFix_Res\\Gamepads\\") != INVALID_FILE_ATTRIBUTES)
    {
      std::vector <std::string> gamepads_;

      WIN32_FIND_DATAA fd;
      HANDLE           hFind  = INVALID_HANDLE_VALUE;
      int              files  = 0;
      LARGE_INTEGER    liSize = { 0 };

      hFind = FindFirstFileA ("TZFix_Res\\Gamepads\\*", &fd);

      if (hFind != INVALID_HANDLE_VALUE)
      {
        do {
          if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
               fd.cFileName[0]    != '.' )
          {
            gamepads_.push_back (fd.cFileName);
          }
        } while (FindNextFileA (hFind, &fd) != 0);

        FindClose (hFind);
      }

            char current_gamepad [128] = { '\0' };
      snprintf ( current_gamepad, 127,
                   "%ws",
                     config.textures.gamepad.c_str () );

      for (int i = 0; i < gamepads_.size (); i++)
      {
        gamepads.array.push_back (
          _strdup ( gamepads_ [i].c_str () )
        );

        if (! stricmp (gamepads.array [i], current_gamepad))
          gamepads.sel = i;
      }
    }
  }

  if (ImGui::BeginPopupModal ("Gamepad Config", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    int orig_sel = gamepads.sel;

    if (ImGui::ListBox ("Gamepad\nIcons", &gamepads.sel, gamepads.array.data (), gamepads.array.size (), 3))
    {
      if (orig_sel != gamepads.sel)
      {
        wchar_t pad [128] = { L'\0' };

        swprintf (pad, L"%hs", gamepads.array [gamepads.sel]);

        config.textures.gamepad             = pad;
        tzf::RenderFix::need_reset.textures = true;
      }

      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup ();
  }
}

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

IMGUI_API
void
ImGui_ImplDX9_NewFrame (void);


void
TZFix_DrawConfigUI (void)
{
  static bool was_reset = false;

  ImGui_ImplDX9_NewFrame ();

  ImGuiIO& io =
    ImGui::GetIO ();

  static bool first_frame = true;

  extern HMODULE hInjectorDLL;

  typedef void (__stdcall *SK_ImGui_DrawEULA_pfn)(LPVOID reserved);

  static SK_ImGui_DrawEULA_pfn SK_ImGui_DrawEULA =
    (SK_ImGui_DrawEULA_pfn)GetProcAddress ( hInjectorDLL,
                                              "SK_ImGui_DrawEULA" );

  struct {
    bool show             = true;
    bool never_show_again = config.input.ui.never_show_eula;
  } static show_eula;
  
  if (show_eula.show && (! show_eula.never_show_again)) {
    SK_ImGui_DrawEULA (&show_eula);

    if (show_eula.never_show_again) config.input.ui.never_show_eula = true;
  }

  static int frame = 0;

  if (frame++ < 10)
    ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);

  if (io.DisplaySize.x != tzf::RenderFix::width ||
      io.DisplaySize.y != tzf::RenderFix::height)
  {
    frame = 0;

    io.DisplaySize.x = (float)tzf::RenderFix::width;
    io.DisplaySize.y = (float)tzf::RenderFix::height;

    ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);;
  }

  ImGui::SetNextWindowSizeConstraints (ImVec2 (500, 50),  ImVec2 ( ImGui::GetIO ().DisplaySize.x * 0.85f,
                                                                    ImGui::GetIO ().DisplaySize.y * 0.85f ) );

  if (was_reset) {
    ImGui::SetNextWindowSize (ImVec2 (500, 50), ImGuiSetCond_Always);
    was_reset = false;
  }

  bool show_config = true;

  ImGui::Begin ( "Tales of Zestiria \"Fix\" Control Panel",
                   &show_config,
                    ( ImGuiWindowFlags_AlwaysAutoResize |
                      ImGuiWindowFlags_ShowBorders      |
                      ImGuiWindowFlags_AlwaysUseWindowPadding )
               );

  ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

  if (ImGui::CollapsingHeader ("UI Scale"))
  {
    ImGui::TreePush    ("");

    ImGui::SliderFloat ("Scale (only 1.0 is officially supported)", &config.input.ui.scale, 1.0f, 3.0f);

    ImGui::TreePop     ();
  }

      io.FontGlobalScale = config.input.ui.scale;
  const  float font_size           =  ImGui::GetFont ()->FontSize                                     * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;


  if (ImGui::CollapsingHeader ("Framerate Control", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
#if 0
    int limiter =
      0;//config.framerate.replace_limiter ? 1 : 0;

    const char* szLabel = (limiter == 0 ? "Framerate Limiter  (choose something else!)" : "Framerate Limiter");

    ImGui::Combo (szLabel, &limiter, "Namco          (A.K.A. Stutterfest 2017)\0"
                                     "Special K      (Precision Timing For The Win!)\0\0" );
#endif

    static float values [120]  = { 0 };
    static int   values_offset =   0;

    values [values_offset] = 1000.0f * ImGui::GetIO ().DeltaTime;
    values_offset = (values_offset + 1) % IM_ARRAYSIZE (values);

    if (reset_frame_history) {
      const float fZero = 0.0f;
      memset (values, *(reinterpret_cast <const DWORD *> (&fZero)), sizeof (float) * 120);

      values_offset       = 0;
      reset_frame_history = false;
      was_reset           = true;
    }

    float sum = 0.0f;

    float min = FLT_MAX;
    float max = 0.0f;

    for (float val : values) {
      sum += val;

      if (val > max)
        max = val;

      if (val < min)
        min = val;
    }

    static char szAvg [512];


    float target_fps       = 1000.0f / tzf::FrameRateFix::GetTargetFrametime ();
    float target_frametime = tzf::FrameRateFix::GetTargetFrametime ();

    sprintf_s
          ( szAvg,
              512,
                u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
                u8"    Extreme frametimes:      %6.3f min, %6.3f max\n\n\n\n"
                u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
                  sum / 120.0f, target_frametime,
                    min, max, max - min,
                      1000.0f / (sum / 120.0f), (max - min) / (1000.0f / (sum / 120.0f)) );

    ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                              ImColor::HSV ( 0.31f - 0.31f *
                       std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                               0.73f,
                                                 0.93f ) );

    ImGui::PlotLines ( "",
                         values,
                           IM_ARRAYSIZE (values),
                             values_offset,
                               szAvg,
                                 0.0f,
                                   2.0f * target_frametime,
                                     ImVec2 (0, font_size * 7) );

    ImGui::SameLine     ();

    ImGui::PushItemWidth (210);
    ImGui::BeginChild   ( "Sub2", ImVec2 ( font_size * 30, font_size * 7 ), true );

    int cutscene = 0, battle = 0, world = 0;

    if (config.framerate.cutscene_target == 30) cutscene = 1; else cutscene = 0;

    if        (config.framerate.battle_target == 10)    battle = 5;
    else if   (config.framerate.battle_target == 12)    battle = 4;
    else if   (config.framerate.battle_target == 15)    battle = 3;
    else if   (config.framerate.battle_target == 20)    battle = 2;
    else if   (config.framerate.battle_target == 30)    battle = 1;
    else /*if (config.framerate.battle_target == 60)*/  battle = 0;

    if        (config.framerate.target == 10)           world = 5;
    else if   (config.framerate.target == 12)           world = 4;
    else if   (config.framerate.target == 15)           world = 3;
    else if   (config.framerate.target == 20)           world = 2;
    else if   (config.framerate.target == 30)           world = 1;
    else /*if (config.framerate.battle_target == 60)*/  world = 0;

    ImGui::Combo        ("Battle  ", &battle,   " 60 FPS\0 30 FPS\0 20 FPS\0 15 FPS\0 12 FPS\0 10 FPS\0\0");
    ImGui::Combo        ("Cutscene", &cutscene, " 60 FPS\0 30 FPS\0\0");
    ImGui::Combo        ("World   ", &world,    " 60 FPS\0 30 FPS\0 20 FPS\0 15 FPS\0 12 FPS\0 10 FPS\0\0");

    config.framerate.cutscene_target = (cutscene == 0) ? 60 : 30;

         if (world == 0) config.framerate.target = 60;
    else if (world == 1) config.framerate.target = 30;
    else if (world == 2) config.framerate.target = 20;
    else if (world == 3) config.framerate.target = 15;
    else if (world == 4) config.framerate.target = 12;
    else if (world == 5) config.framerate.target = 10;

         if (battle == 0) config.framerate.battle_target = 60;
    else if (battle == 1) config.framerate.battle_target = 30;
    else if (battle == 2) config.framerate.battle_target = 20;
    else if (battle == 3) config.framerate.battle_target = 15;
    else if (battle == 4) config.framerate.battle_target = 12;
    else if (battle == 5) config.framerate.battle_target = 10;

    ImGui::EndChild     ();
    ImGui::PopItemWidth ();

    //ImGui::Text ( "Application average %.3f ms/frame (%.1f FPS)",
                    //1000.0f / ImGui::GetIO ().Framerate,
                              //ImGui::GetIO ().Framerate );
  }

  if (ImGui::CollapsingHeader ("Textures", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Quality"))
    {
      if (ImGui::Checkbox ("Generate Mipmaps", &config.textures.remaster)) tzf::RenderFix::need_reset.graphics = true;

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates distant texture aliasing and shimmering caused by missing/incomplete mipmaps.");

      if (config.textures.remaster)
      {
        ImGui::TreePush ("");

        //if (ImGui::Checkbox ("Do Not Compress Generated Mipmaps", &config.textures.uncompressed)) tzf::RenderFix::need_reset.graphics = true;

        //if (ImGui::IsItemHovered ())
          //ImGui::SetTooltip ("Uses more VRAM, but avoids texture compression artifacts on generated mipmaps.");

        ImGui::Checkbox ("Show Loading Activity in OSD During Mipmap Generation", &config.textures.show_loading_text);
        ImGui::TreePop  ();
      }

      ImGui::SliderFloat ("Mipmap LOD Bias", &config.textures.lod_bias, -3.0f, 3.0f);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Controls texture sharpness;  -3 = Sharpest (WILL shimmer),  0 = Neutral,  3 = Blurry");
        ImGui::EndTooltip   ();
      }
      ImGui::TreePop    ();
    }

    if (ImGui::CollapsingHeader ("Performance", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
    {
      extern bool __remap_textures;

      ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 15.0f);
      ImGui::TreePush  ("");

      ImGui::BeginChild  ("Texture Details", ImVec2 ( font_size           * 70,
                                                      font_size_multiline * 6.1f ),
                                               true );

      ImGui::Columns   ( 3 );
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
        ImGui::Text    ( "       Savings" );
        ImGui::PopStyleColor  ();
      ImGui::Columns   ( 1 );

      ImGui::PushStyleColor
                       ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

      ImGui::Separator (   );

      ImGui::Columns   ( 3 );
        ImGui::Text    ( "%#6zu MiB Total",
                                                       tzf::RenderFix::tex_mgr.cacheSizeTotal () >> 20ULL ); ImGui::NextColumn (); 
        ImGui::TextColored
                       (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                         "%#5lu     Hits",             tzf::RenderFix::tex_mgr.getHitCount    ()          );  ImGui::NextColumn ();
        ImGui::Text    ( "Budget: %#7zu MiB  ",        tzf::RenderFix::pDevice->
                                                                       GetAvailableTextureMem ()  / 1048576UL );
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      ImColor  active   ( 1.0f,  1.0f,  1.0f, 1.0f);
      ImColor  inactive (0.75f, 0.75f, 0.75f, 1.0f);
      ImColor& color   = __remap_textures ? inactive : active;

      ImGui::Columns   ( 3 );
        ImGui::TextColored ( color,
                               "%#6zu MiB Base",
                                                       tzf::RenderFix::tex_mgr.cacheSizeBasic () >> 20ULL );  ImGui::NextColumn (); 
        if (ImGui::IsItemClicked ())
          __remap_textures = false;

        ImGui::TextColored
                       (ImVec4 (1.0f, 0.3f, 0.3f, 1.0f),
                         "%#5lu   Misses",             tzf::RenderFix::tex_mgr.getMissCount   ()          );  ImGui::NextColumn ();
        ImGui::Text    ( "Time:    %#6.04lf  s  ", tzf::RenderFix::tex_mgr.getTimeSaved       () / 1000.0f);
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

      color = __remap_textures ? active : inactive;

      ImGui::Columns   ( 3 );
        ImGui::TextColored ( color,
                               "%#6zu MiB Injected",
                                                       tzf::RenderFix::tex_mgr.cacheSizeInjected () >> 20ULL ); ImGui::NextColumn (); 

        if (ImGui::IsItemClicked ())
          __remap_textures = true;

        ImGui::TextColored (ImVec4 (0.555f, 0.555f, 1.0f, 1.0f),
                         "%.2f  Hit/Miss",          (double)tzf::RenderFix::tex_mgr.getHitCount  () / 
                                                    (double)tzf::RenderFix::tex_mgr.getMissCount ()          ); ImGui::NextColumn ();
        ImGui::Text    ( "Driver: %#7zu MiB  ",    tzf::RenderFix::tex_mgr.getByteSaved          () >> 20ULL );

      ImGui::PopStyleColor
                       (   );
      ImGui::Columns   ( 1 );

      ImGui::Separator (   );

#if 0
      ImGui::TreePush  ("");
      ImGui::Checkbox  ("Enable Texture QuickLoad", &config.textures.quick_load);
      
      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  (  );
          ImGui::TextColored ( ImVec4 (0.9f, 0.9f, 0.9f, 1.f), 
                                 "Only read the first texture level-of-detail from disk and split generation of the rest across all available CPU cores." );
          ImGui::Separator   (  );
          ImGui::TreePush    ("");
            ImGui::Bullet    (  ); ImGui::SameLine ();
            ImGui::TextColored ( ImColor (0.95f, 0.75f, 0.25f, 1.0f), 
                                   "Typically this reduces hitching and load-times, but some pop-in may "
                                   "become visible on lower-end CPUs." );
            ImGui::Bullet    (  ); ImGui::SameLine ();
            ImGui::TextColored  ( ImColor (0.95f, 0.75f, 0.25f, 1.0f),
                                   "Ideally, this should be used with texture compression disabled." );
            ImGui::SameLine  (  );
            ImGui::Text      ("[Quality/Generate Mipmaps]");
          ImGui::TreePop     (  );
        ImGui::EndTooltip    (  );
      }

      if (config.textures.quick_load) {
        ImGui::SameLine ();

        
        if (ImGui::SliderInt ("# of Threads", &config.textures.worker_threads, 3, 9))
        {
          // Only allow odd numbers of threads
          config.textures.worker_threads += 1 - (config.textures.worker_threads & 0x1);

          need_restart = true;
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Lower is actually better, the only reason to adjust this would be if you have an absurd number of CPU cores and pop-in bothers you ;)");
      }
      ImGui::TreePop      ( );
#endif

      ImGui::EndChild     ( );
      ImGui::PopStyleVar  ( );

#if 0
      if (ImGui::CollapsingHeader ("Thread Stats"))
      {
        std::vector <tzf_tex_thread_stats_s> stats =
          tzf::RenderFix::tex_mgr.getThreadStats ();

        int thread_id = 0;

        for ( auto it : stats ) {
          ImGui::Text ("Thread #%lu  -  %6lu jobs retired, %5lu MiB loaded  -  %.6f User / %.6f Kernel / %3.1f Idle",
                          thread_id++,
                            it.jobs_retired, it.bytes_loaded >> 20UL,
                              (double)ULARGE_INTEGER { it.runtime.user.dwLowDateTime,   it.runtime.user.dwHighDateTime   }.QuadPart / 10000000.0,
                              (double)ULARGE_INTEGER { it.runtime.kernel.dwLowDateTime, it.runtime.kernel.dwHighDateTime }.QuadPart / 10000000.0,
                              (double)ULARGE_INTEGER { it.runtime.idle.dwLowDateTime,   it.runtime.idle.dwHighDateTime   }.QuadPart / 10000000.0 );
        }
      }
#endif
      ImGui::TreePop      ( );
    }

    if (ImGui::Button ("  Texture Modding Tools  ")) {
      show_texture_mod_dlg = (! show_texture_mod_dlg);
    }

    ImGui::SameLine ();

    ImGui::SliderInt ("Texture Cache Size", &config.textures.max_cache_in_mib, 384, 2048, "%.0f MiB");

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Lower this if the reported budget (on the Performance tab) ever becomes negative.");

    ImGui::TreePop ();
  }

#if 0
  if (ImGui::CollapsingHeader ("Shader Options"))
  {
    ImGui::Checkbox ("Dump Shaders", &config.render.dump_shaders);
  }
#endif

  if (ImGui::CollapsingHeader ("Aspect Ratio"))
  {
    ImGui::TreePush ("");
    ImGui::Checkbox ("Use Aspect Ratio Correction Everywhere",     &config.render.aspect_correction);
    ImGui::TreePush ("");
    ImGui::Checkbox ("Clear non-16:9 Region in Menus",             &config.render.clear_blackbars);
    ImGui::TreePop  ();

    if (! config.render.aspect_correction)
    {
        ImGui::Checkbox ("Apply Correction (only) to Bink Videos", &config.render.blackbar_videos);
    }
    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Post-Processing"))
  {
    ImGui::TreePush ("");

    if ( ImGui::SliderFloat ("Post-Process Resolution Scale", &config.render.postproc_ratio, 0.0f, 2.0f) )
      tzf::RenderFix::need_reset.graphics = true;
    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Shadow Quality"))
  {
    ImGui::TreePush ("");

    struct shadow_imp_s
    {
      shadow_imp_s (int scale)
      {
        scale = std::abs (scale);

        if (scale > 3)
          scale = 3;

        radio    = scale;
        last_sel = radio;
      }

      int radio    = 0;
      int last_sel = 0;
    };

    static shadow_imp_s shadows     (config.render.shadow_rescale);
    static shadow_imp_s env_shadows (config.render.env_shadow_rescale);

    ImGui::Combo ("Character Shadow Resolution",     &shadows.radio,     "Normal\0Enhanced\0High\0Ultra\0\0");
    ImGui::Combo ("Environmental Shadow Resolution", &env_shadows.radio, "Normal\0High\0Ultra\0\0");

    if (env_shadows.radio != env_shadows.last_sel) {
      config.render.env_shadow_rescale    = env_shadows.radio;
      env_shadows.last_sel                = env_shadows.radio;
      tzf::RenderFix::need_reset.graphics = true;
    }

    if (shadows.radio != shadows.last_sel) {
      config.render.shadow_rescale        = -shadows.radio;
      shadows.last_sel                    =  shadows.radio;
      tzf::RenderFix::need_reset.graphics = true;
    }

    ImGui::TreePop ();
  }

  static bool need_restart = false;

  if (ImGui::CollapsingHeader ("Audio", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (tzf::SoundFix::wasapi_init) {
      ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 16.0f);
      ImGui::BeginChild  ("Audio Details", ImVec2 (0, 80), true);

        ImGui::Columns   (3);
        ImGui::Text      ("");                                                                     ImGui::NextColumn ();
        ImGui::Text      ("Sample Rate");                                                          ImGui::NextColumn ();
        ImGui::Text      ("Channel Setup");
        ImGui::Columns   (1);
        ImGui::Separator ();
        ImGui::Columns   (3);
        ImGui::Text      ( "Game (SoundCore)");                                                    ImGui::NextColumn ();
        ImGui::Text      ( "%6.2f kHz @ %lu-bit", (float)tzf::SoundFix::snd_core_fmt.nSamplesPerSec / 1000.0f,
                                                   tzf::SoundFix::snd_core_fmt.wBitsPerSample );   ImGui::NextColumn ();
        ImGui::Text      ( "%lu",                  tzf::SoundFix::snd_core_fmt.nChannels );
        ImGui::Columns   (1);
        ImGui::Separator ();
        ImGui::Columns   (3);
        ImGui::Text      ( "Device");                                                              ImGui::NextColumn ();
        ImGui::Text      ( "%6.2f kHz @ %lu-bit", (float)tzf::SoundFix::snd_device_fmt.nSamplesPerSec / 1000.0f,
                                                   tzf::SoundFix::snd_device_fmt.wBitsPerSample ); ImGui::NextColumn ();
        ImGui::Text      ( "%lu",                  tzf::SoundFix::snd_device_fmt.nChannels );
        ImGui::Columns   (1);

      ImGui::EndChild    ();
      ImGui::PopStyleVar ();
    }

    need_restart |= ImGui::Checkbox ("Enable Audio Sample Rate Fix", &config.audio.enable_fix);

    if (config.audio.enable_fix)
    {
      ImGui::TreePush ("");
      int sel;

      if   (config.audio.sample_hz == 48000)  sel = 1;
      else                                    sel = 0;

      need_restart |= ImGui::Combo ("Sample Rate", &sel, " 44.1 kHz\0 48.0 kHz\0\0", 2);

      if (sel == 0)  config.audio.sample_hz = 44100;
      else           config.audio.sample_hz = 48000;

      need_restart |= ImGui::Checkbox ("Use Compatibility Mode", &config.audio.compatibility);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("May reduce audio quality, but can help with some weird USB headsets and Windows 7 / Older.");
      ImGui::TreePop  ();
    }

    ImGui::TreePop ();
  }

  ImGui::PopItemWidth ();

  ImGui::Separator (   );
  ImGui::Columns   ( 2 );

  if (ImGui::Button ("   Gamepad Config   "))
    ImGui::OpenPopup ("Gamepad Config");

  TZFix_GamepadConfigDlg ();

  ImGui::SameLine      ();

  if (ImGui::Button ("   Special K Config   "))
    show_special_k_cfg = (! show_special_k_cfg);

  ImGui::SameLine ();

  if (ImGui::Selectable ("...", show_test_window))
    show_test_window = (! show_test_window);

  ImGui::SameLine (0.0f, 60.0f);

  if (ImGui::Selectable (" ... ", show_test_window))
    show_test_window = (! show_test_window);

  ImGui::NextColumn ();

  ImGui::Checkbox ("Apply Changes Immediately ", &config.render.auto_apply_changes);

  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
    ImGui::Text           ("Third-Party Software May Not Like This\n\n");
    ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
    ImGui::BulletText     ("You also may not when it takes 5 seconds to change some settings ;)");
    ImGui::PopStyleColor  (2);
    ImGui::EndTooltip     ();
  }

  bool extra_details = false;

  if (need_restart || tzf::RenderFix::need_reset.graphics || tzf::RenderFix::need_reset.textures)
     extra_details = true;

  if (extra_details) {
    ImGui::Columns    ( 1 );
    ImGui::Separator  (   );

    if (need_restart) {
      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.4f, 0.15f, 1.0f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    if (tzf::RenderFix::need_reset.graphics || tzf::RenderFix::need_reset.textures)
    {
      if (! config.render.auto_apply_changes)
      {
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Bullet          ( ); ImGui::SameLine ();
        
        ImGui::TextWrapped     ( "You have made changes to settings that require the game to re-load resources...");
        ImGui::Button          ( "  Apply Now  " );
        
        if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Click this before complaining that things look weird.");
        if (ImGui::IsItemClicked ()) tzf::RenderFix::TriggerReset ();
        
        ImGui::PopStyleColor   ( );
        ImGui::PopTextWrapPos  ( );
      }

      else
      {
        ImGui::TextColored           (ImVec4 (0.95f, 0.65f, 0.15f, 1.0f), "Applying Changes...");
        tzf::RenderFix::TriggerReset ();
      }
    }
  }

  ImGui::End ();

  if (show_special_k_cfg)  show_special_k_cfg = SK_ImGui_ControlPanel ();

  if (show_test_window) {
    ImGui::SetNextWindowPos (ImVec2 (650, 20), ImGuiSetCond_FirstUseEver);
    ImGui::ShowTestWindow   (&show_test_window);
  }

  if (show_texture_mod_dlg)
  {
    show_texture_mod_dlg = TZFix_TextureModDlg ();
  }

  if ( SUCCEEDED (
         tzf::RenderFix::pDevice->BeginScene ()
       )
     )
  {
    ImGui::Render                     ();
    tzf::RenderFix::pDevice->EndScene ();
  }

  if (! show_config)
  {
    TZFix_ToggleConfigUI ();
  }
}


typedef DWORD (*SK_ImGui_DrawFrame_pfn)(DWORD dwFlags, void* user);
SK_ImGui_DrawFrame_pfn SK_ImGui_DrawFrame_Original = nullptr;

DWORD
TZFix_ImGui_DrawFrame (DWORD dwFlags, void* user)
{
  TZFix_DrawConfigUI ();

  return 1;
}


void
TZFix_ImGui_Init (void)
{
  TZF_CreateDLLHook2 ( config.system.injector.c_str (),
                        "SK_ImGui_Toggle",
                        TZFix_ToggleConfigUI,
             (LPVOID *)&SK_ImGui_Toggle_Original );

  TZF_CreateDLLHook2 ( config.system.injector.c_str (),
                        "SK_ImGui_DrawFrame",
                     TZFix_ImGui_DrawFrame,
             (LPVOID *)&SK_ImGui_DrawFrame_Original );
}
