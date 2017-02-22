/**
 * This file is part of Tales of Berseria "Fix".
 *
 * Tales of Berseria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Berseria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Berseria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#define _CRT_SECURE_NO_WARNINGS

#define NOMINMAX

#include "DLL_VERSION.H"
#include "imgui\imgui.h"

#include <string>
#include <vector>
#include <algorithm>

#include "config.h"
#include "command.h"
#include "render.h"
#include "textures.h"

#include <atlbase.h>

extern std::wstring
SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);

void
TZF_DrawFileList (bool& can_scroll)
{
  const float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  ImGui::PushItemWidth (500.0f);

  struct enumerated_source_s
  {
    std::string            name       = "invalid";
    std::vector <uint32_t> checksums;

    struct {
      std::vector < std::pair < uint32_t, tzf_tex_record_s > >
                 records;
      uint64_t   size                 = 0ULL;
    } streaming, blocking;

    uint64_t     totalSize (void) { return streaming.size + blocking.size; };
  };

  static std::vector <enumerated_source_s>                        sources;
  static std::vector < std::pair < uint32_t, tzf_tex_record_s > > injectable;
  static std::vector < std::wstring >                             archives;
  static bool                                                     list_dirty = true;
  static int                                                      sel        = 0;

  auto EnumerateSource =
    [](int archive_no) ->
      enumerated_source_s
      {
        enumerated_source_s source;

        char szFileName [MAX_PATH] = { '\0' };

        if (archive_no != std::numeric_limits <unsigned int>::max ()) {
          sprintf (szFileName, "%ws", archives [archive_no].c_str ()); 
        }

        else strncpy (szFileName, "Regular Filesystem", MAX_PATH);

        source.name = szFileName;

        for ( auto it : injectable )
        {
          if (it.second.archive == archive_no)
          {
            switch (it.second.method)
            {
              case DontCare:
              case Streaming:
                source.streaming.records.push_back (std::make_pair (it.first, it.second));
                source.streaming.size += it.second.size;
                break;

              case Blocking:
                source.blocking.records.push_back (std::make_pair (it.first, it.second));
                source.blocking.size += it.second.size;
                break;
            }

            source.checksums.push_back (it.first);
          }
        }

        return source;
      };

  if (list_dirty)
  {
    injectable = TZF_GetInjectableTextures ();
    archives   = TZF_GetTextureArchives    ();

    sources.clear  ();

        sel = 0;
    int idx = 0;

    // First the .7z Data Sources
    for ( auto it : archives )
    {
      sources.push_back (EnumerateSource (idx++));
    }

    // Then the Straight Filesystem
    sources.push_back (EnumerateSource (std::numeric_limits <unsigned int>::max ()));

    list_dirty = false;
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border,                   ImVec4 (0.4f, 0.6f, 0.9f, 1.0f));

#define FILE_LIST_WIDTH  font_size * 20
#define FILE_LIST_HEIGHT font_size * (sources.size () + 3)

  ImGui::BeginChild ( "Source List",
                        ImVec2 ( FILE_LIST_WIDTH, FILE_LIST_HEIGHT ),
                          true,
                            ImGuiWindowFlags_AlwaysAutoResize );

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  auto DataSourceTooltip =
    [](int sel) ->
      void
      {
        ImGui::BeginTooltip ();
        ImGui::TextColored  (ImVec4 (0.9f, 0.7f, 0.3f, 1.f), "Data Source  (%s)", sources [sel].name.c_str ());
        ImGui::Separator    ();

        ImGui::Text ( "Total Size:                 %#5.2f MiB",
                        (double)sources [sel].totalSize () / (1024.0 * 1024.0) );
        ImGui::Text ( "Blocking Data:  %4lu File%c (%#5.2f MiB)",
                        sources [sel].blocking.records.size (),
                        sources [sel].blocking.records.size () != 1 ? 's' : ' ',
                        (double)sources [sel].blocking.size / (1024.0 * 1024.0) );
        ImGui::Text ( "Streaming Data: %4lu File%c (%#5.2f MiB)",
                        sources [sel].streaming.records.size (),
                        sources [sel].streaming.records.size () != 1 ? 's' : ' ',
                        (double)sources [sel].streaming.size / (1024.0 * 1024.0) );

        ImGui::EndTooltip    ();
      };

  if (sources.size ())
  {
    static      int last_sel = 0;
    static bool sel_changed  = false;
  
    if (sel != last_sel)
      sel_changed = true;
  
    last_sel = sel;
  
    for ( int line = 0; line < sources.size (); line++)
    {
      if (line == sel)
      {
        bool selected = true;
        ImGui::Selectable (sources [line].name.c_str (), &selected);
   
        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
          sel_changed = false;
        }
      }
   
      else
      {
        bool selected = false;

        if (ImGui::Selectable (sources [line].name.c_str (), &selected))
        {
          sel_changed = true;
          //tex_dbg_idx                 =  line;
          sel                         =  line;
          //debug_tex_id                =  textures_used_last_dump [line];
        }
      }

      if (ImGui::IsItemHovered ())
        DataSourceTooltip (line);
    }

    ImGui::EndChild      ();

    ImVec2 list_size = ImGui::GetItemRectSize ();

    ImGui::PopStyleColor ();
    ImGui::PopStyleVar   ();

    ImGui::SameLine      ();

    ImGui::BeginGroup    ();

    ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::BeginChild ( "Texture Selection",
                           ImVec2 (font_size * 30, list_size.y - font_size * 2),
                             true );

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

    for ( auto it : sources [sel].checksums )
    {
      tzf_tex_record_s* injectable =
        TZF_GetInjectableTexture (it);

      if (injectable != nullptr) {
                    
        ImGui::TextColored ( ImVec4 (0.9f, 0.6f, 0.3f, 1.0f), " %08x    ", it );
        ImGui::SameLine    (                                                  );

        bool streaming = 
          injectable->method == Streaming;

        ImGui::TextColored ( streaming ?
                               ImVec4 ( 0.2f,  0.90f, 0.3f, 1.0f ) :
                               ImVec4 ( 0.90f, 0.3f,  0.2f, 1.0f ),
                                 "  %s    ",
                                   streaming ? "Streaming" : 
                                               " Blocking" );
        ImGui::SameLine    (                               );

        ImGui::TextColored ( ImVec4 (1.f, 1.f, 1.f, 1.f), "%#5.2f MiB  ",
                            (double)injectable->size / (1024.0 * 1024.0) );
      }
    }

    ImGui::EndChild      (   );
    ImGui::PopStyleColor ( 1 );

    if (ImGui::Button ("  Refresh Data Sources  "))
    {
      TZF_RefreshDataSources ();
      list_dirty = true;
    }

    ImGui::SameLine ();

    if (ImGui::Button ("  Reload All Textures  "))
    {
      tzf::RenderFix::TriggerReset ();
    }

    ImGui::EndGroup ();
  }

  ImGui::PopItemWidth ();
}


extern std::unordered_map <uint32_t, tzf::RenderFix::shader_disasm_s> ps_disassembly;
extern std::unordered_map <uint32_t, tzf::RenderFix::shader_disasm_s> vs_disassembly;

enum class tzf_shader_class {
  Unknown = 0x00,
  Pixel   = 0x01,
  Vertex  = 0x02
};

void
TZF_LiveShaderClassView (tzf_shader_class shader_type, bool& can_scroll)
{
  ImGui::BeginGroup ();

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <std::string> contents;
    bool                      dirty      = true;
    uint32_t                  last_sel   =    0;
    int                            sel   =   -1;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
  };

  struct {
    shader_class_imp_s vs;
    shader_class_imp_s ps;
  } static list_base;

  shader_class_imp_s*
    list    = ( shader_type == tzf_shader_class::Pixel ? &list_base.ps :
                                                         &list_base.vs );

  tzf::RenderFix::shader_tracking_s*
    tracker = ( shader_type == tzf_shader_class::Pixel ? &tzf::RenderFix::tracked_ps :
                                                         &tzf::RenderFix::tracked_vs );

  std::vector <uint32_t>
    shaders   ( shader_type == tzf_shader_class::Pixel ? tzf::RenderFix::last_frame.pixel_shaders.begin  () :
                                                         tzf::RenderFix::last_frame.vertex_shaders.begin (),
                shader_type == tzf_shader_class::Pixel ? tzf::RenderFix::last_frame.pixel_shaders.end    () :
                                                         tzf::RenderFix::last_frame.vertex_shaders.end   () );

  std::unordered_map <uint32_t, tzf::RenderFix::shader_disasm_s>&
    disassembly = ( shader_type == tzf_shader_class::Pixel ? ps_disassembly :
                                                             vs_disassembly );

  const char*
    szShaderWord =  shader_type == tzf_shader_class::Pixel ? "Pixel" :
                                                             "Vertex";

  if (list->dirty)
  {
        list->sel = -1;
    int idx    =  0;
        list->contents.clear ();

    // The underlying list is unsorted for speed, but that's not at all
    //   intuitive to humans, so sort the thing when we have the RT view open.
    std::sort ( shaders.begin (),
                shaders.end   () );



    for ( auto it : shaders )
    {
      char szDesc [16] = { };

      sprintf (szDesc, "%08lx", (uintptr_t)it);

      list->contents.emplace_back (szDesc);

      if ((uint32_t)it == list->last_sel)
      {
        list->sel = idx;
        //tzf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
      }

      ++idx;
    }
  }

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( szShaderWord,
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize );

  if (ImGui::IsWindowHovered ())
  {
    can_scroll = false;

    ImGui::BeginTooltip ();
    ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
    ImGui::Separator    ();
    ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous shader");
    ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next shader");
    ImGui::EndTooltip   ();

         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  if (shaders.size ())
  {
    struct {
      int  last_sel    = 0;
      bool sel_changed = false;
    } static shader_state [3];

    int&  last_sel    = shader_state [(int)shader_type].last_sel;
    bool& sel_changed = shader_state [(int)shader_type].sel_changed;

    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    for ( int line = 0; line < shaders.size (); line++ )
    {
      if (line == list->sel)
      {
        bool selected    = true;

        ImGui::Selectable (list->contents [line].c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f);

          sel_changed    = false;
          list->last_sel = (uint32_t)shaders [list->sel];
          tracker->crc32 = (uint32_t)shaders [list->sel];
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (list->contents [line].c_str (), &selected))
        {
          sel_changed    = true;
          list->sel      =  line;
          list->last_sel = (uint32_t)shaders [list->sel];
          tracker->crc32 = (uint32_t)shaders [list->sel];
        }
      }
    }
  }

  ImGui::EndChild      ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ()) {
         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) list->sel--;
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) list->sel++;
  }

  if (tracker->crc32 != 0x00)
  {
    ImGui::BeginGroup ();
    ImGui::Checkbox ( shader_type == tzf_shader_class::Pixel ? "Cancel Draws Using Selected Pixel Shader" :
                                                               "Cancel Draws Using Selected Vertex Shader", 
                        &tracker->cancel_draws );  ImGui::SameLine ();

    if (tracker->cancel_draws)
      ImGui::TextDisabled ("%lu Skipped Draw%c Last Frame (%lu textures)", tracker->num_draws, tracker->num_draws != 1 ? 's' : ' ', tracker->used_textures.size () );
    else
      ImGui::TextDisabled ("%lu Draw%c Last Frame         (%lu textures)", tracker->num_draws, tracker->num_draws != 1 ? 's' : ' ', tracker->used_textures.size () );

    ImGui::Checkbox ( shader_type == tzf_shader_class::Pixel ? "Clamp Texture Coordinates For Selected Pixel Shader" :
                                                               "Clamp Texture Coordinates For Selected Vertex Shader",
                        &tracker->clamp_coords );

    ImGui::Separator      ();
    ImGui::EndGroup       ();

    if (ImGui::IsItemHoveredRect () && tracker->used_textures.size ())
    {
      ImGui::BeginTooltip ();

      D3DFORMAT fmt = D3DFMT_UNKNOWN;

      for ( auto it : tracker->used_textures )
      {
        ISKTextureD3D9* pTex = tzf::RenderFix::tex_mgr.getTexture (it)->d3d9_tex;
        
        if (pTex && pTex->pTex)
        {
          D3DSURFACE_DESC desc;
          if (SUCCEEDED (pTex->pTex->GetLevelDesc (0, &desc)))
          {
            fmt = desc.Format;
            ImGui::Image ( pTex->pTex, ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }

          ImGui::SameLine ();

          ImGui::BeginGroup ();
          ImGui::Text       ("Texture: %08lx", it);
          ImGui::Text       ("Format:  %ws",   SK_D3D9_FormatToStr (fmt).c_str ());
          ImGui::EndGroup   ();
        }
      }

      ImGui::EndTooltip ();
    }

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32].header.c_str ());

    ImGui::SameLine       ();
    ImGui::BeginGroup     ();
    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.666f, 0.666f, 0.666f, 1.0f));

    char szName    [192] = { '\0' };
    char szOrdinal [64 ] = { '\0' };
    char szOrdEl   [ 96] = { '\0' };

    int  el = 0;

    ImGui::PushItemWidth (font_size * 25);

    for ( auto&& it : tracker->constants )
    {
      if (it.struct_members.size ())
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::Text           (it.Name);
        ImGui::PopStyleColor  ();

        for ( auto&& it2 : it.struct_members )
        {
          snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                        it2.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                          it2.RegisterIndex );
          snprintf ( szOrdEl,  96,  "%s::%lu", // Uniquely identify parameters that share registers
                       szOrdinal, el++ );
          snprintf ( szName, 192, "%-24s :%s",
                       it2.Name, szOrdinal );

          if (it2.Type == D3DXPT_FLOAT && it2.Class == D3DXPC_VECTOR)
          {
            ImGui::Checkbox    (szName,  &it2.Override); ImGui::SameLine ();
            ImGui::InputFloat4 (szOrdEl,  it2.Data);
          }
          else {
            ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
          }
        }

        ImGui::Separator ();
      }

      else
      {
        snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                     it.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                        it.RegisterIndex );
        snprintf ( szOrdEl,  96,  "%s::%lu", // Uniquely identify parameters that share registers
                       szOrdinal, el++ );
        snprintf ( szName, 192, "%-24s :%s",
                     it.Name, szOrdinal );

        if (it.Type == D3DXPT_FLOAT && it.Class == D3DXPC_VECTOR)
        {
          ImGui::Checkbox    (szName,  &it.Override); ImGui::SameLine ();
          ImGui::InputFloat4 (szOrdEl,  it.Data);
        } else {
          ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
        }
      }
    }
    ImGui::PopItemWidth ();
    ImGui::TreePop      ();
    ImGui::EndGroup     ();

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.99f, 0.99f, 0.01f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32].code.c_str ());

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.5f, 0.95f, 0.5f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32].footer.c_str ());

    ImGui::PopStyleColor (4);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup      ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopStyleVar   ();
  ImGui::EndGroup      ();
}

void
TZF_LiveVertexStreamView (bool& can_scroll)
{
  ImGui::BeginGroup ();

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  struct vertex_stream_s
  {
    std::vector <std::string> contents;
    bool                      dirty      = true;
    uintptr_t                 last_sel   =    0;
    int                            sel   =   -1;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
  };

  struct {
    vertex_stream_s stream0;
  } static list_base;

  vertex_stream_s*
    list    = &list_base.stream0;

  tzf::RenderFix::vertex_buffer_tracking_s*
    tracker = &tzf::RenderFix::tracked_vb;

  std::vector <IDirect3DVertexBuffer9 *>
    buffers   ( tzf::RenderFix::last_frame.vertex_buffers.begin (),
                tzf::RenderFix::last_frame.vertex_buffers.end   () );

  if (list->dirty)
  {
        list->sel = -1;
    int idx    =  0;
        list->contents.clear ();

    // The underlying list is unsorted for speed, but that's not at all
    //   intuitive to humans, so sort the thing when we have the RT view open.
    std::sort ( buffers.begin (),
                buffers.end   () );



    for ( auto it : buffers )
    {
      char szDesc [16] = { };

      sprintf (szDesc, "%08lx", (uintptr_t)it);

      list->contents.emplace_back (szDesc);

      if ((uintptr_t)it == list->last_sel)
      {
        list->sel = idx;
        //tzf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
      }

      ++idx;
    }
  }

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( "Stream 0",
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize );

  if (ImGui::IsWindowHovered ())
  {
    can_scroll = false;

    ImGui::BeginTooltip ();
    ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected vertex buffer to debug a model");
    ImGui::Separator    ();
    ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous shader");
    ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next shader");
    ImGui::EndTooltip   ();

         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  if (buffers.size ())
  {
    struct {
      int  last_sel    = 0;
      bool sel_changed = false;
    } static stream [3];

    int&  last_sel    = stream [0].last_sel;
    bool& sel_changed = stream [0].sel_changed;

    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    for ( int line = 0; line < buffers.size (); line++ )
    {
      if (line == list->sel)
      {
        bool selected    = true;

        ImGui::Selectable (list->contents [line].c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f);

          sel_changed            = false;
          list->last_sel         = (uintptr_t)buffers [list->sel];
          tracker->vertex_buffer =            buffers [list->sel];
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (list->contents [line].c_str (), &selected))
        {
          sel_changed            = true;
          list->sel              =  line;
          list->last_sel         = (uintptr_t)buffers [list->sel];
          tracker->vertex_buffer =            buffers [list->sel];
        }
      }
    }
  }

  ImGui::EndChild      ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ()) {
         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) list->sel--;
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) list->sel++;
  }

  if (tracker->vertex_buffer != nullptr)
  {
    ImGui::Checkbox ("Cancel Draws Using Selected Vertex Buffer",  &tracker->cancel_draws);  ImGui::SameLine ();

    if (tracker->cancel_draws)
      ImGui::TextDisabled ("%lu Skipped Draw%c Last Frame", tracker->num_draws, tracker->num_draws != 1 ? 's' : ' ');
    else
      ImGui::TextDisabled ("%lu Draw%c Last Frame        ", tracker->num_draws, tracker->num_draws != 1 ? 's' : ' ');

    ImGui::Checkbox ("Draw Selected Vertex Buffer In Wireframe", &tracker->wireframe);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup      ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopStyleVar   ();
  ImGui::EndGroup      ();
}


bool
TZFix_TextureModDlg (void)
{
  const float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  bool show_dlg = true;

  ImGui::SetNextWindowSizeConstraints ( ImVec2 (256.0f, 384.0f), ImVec2 ( ImGui::GetIO ().DisplaySize.x * 0.75f, ImGui::GetIO ().DisplaySize.y * 0.75f ) );

  ImGui::Begin ( "Tales Engine Texture Mod Toolkit (v " TZF_VERSION_STR_A ")",
                   &show_dlg,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

  bool can_scroll = ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                 ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

  ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

  if (ImGui::CollapsingHeader ("Preliminary Documentation"))
  {
    ImGui::BeginChild ("ModDescription", ImVec2 (font_size * 66.0f, font_size * 25.0f), true);
      ImGui::TextColored    (ImVec4 (0.9f, 0.7f, 0.5f, 1.0f), "Texture Modding Overview"); ImGui::SameLine ();
      ImGui::Text           ("    (Full Writeup Pending)");

      ImGui::Separator      ();

      ImGui::TextWrapped    ("\nReplacement textures go in (TZFix_Res\\inject\\textures\\{blocking|streaming}\\<checksum>.dds)\n\n");

      ImGui::TreePush ("");
        ImGui::BulletText ("Blocking textures have a high performance penalty, but zero chance of visible pop-in.");
        ImGui::BulletText ("Streaming textures will replace the game's original texture whenever Disk/CPU loads finish.");
        ImGui::TreePush   ("");
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.6f, 0.9f, 0.2f, 1.0f));
          ImGui::BulletText     ("Use streaming whenever possible or performance will bite you in the ass.");
          ImGui::PopStyleColor  ();
        ImGui::TreePop    (  );
      ImGui::TreePop  ();

      ImGui::TextWrapped    ("\n\nLoading modified textures from separate files is inefficient; entire groups of textures may also be packaged into \".7z\" files (See TZFix_Res\\inject\\00_License.7z as an example, and use low/no compression ratio or you will kill the game's performance).\n");

      ImGui::Separator      ();

      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.6f, 0.3f, 1.0f));
      ImGui::TextWrapped    ( "\n\nA more detailed synopsis will follow in future versions, for now please refer to the GitHub release notes for Tales of Symphonia "
                              "\"Fix\" v 0.9.0 for a thorough description on authoring texture mods.\n\n" );
      ImGui::PopStyleColor  ();

      ImGui::Separator      ();

      ImGui::Bullet         (); ImGui::SameLine ();
      ImGui::TextWrapped    ( "If texture mods are enabled, you can click on the Injected and Base buttons on the texture cache "
                                "summary pannel to compare modified and unmodified." );
    ImGui::EndChild         ();
  }

  if (ImGui::CollapsingHeader("Injectable Data Sources", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    TZF_DrawFileList  (can_scroll);
  }

  if (ImGui::CollapsingHeader ("Live Texture View", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    static float last_ht    = 256.0f;
    static float last_width = 256.0f;

    static std::vector <std::string> list_contents;
    static bool                      list_dirty     = false;
    static int                       sel            =     0;

    extern std::vector <uint32_t> textures_used_last_dump;
    extern              uint32_t  tex_dbg_idx;
    extern              uint32_t  debug_tex_id;

    ImGui::BeginChild ("ToolHeadings", ImVec2 (font_size * 66.0f, font_size * 2.5f), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

    if (ImGui::Button ("  Refresh Textures  "))
    {
      SK_ICommandProcessor& command =
        *SK_GetCommandProcessor ();

      command.ProcessCommandLine ("Textures.Trace true");

      tzf::RenderFix::tex_mgr.updateOSD ();

      list_dirty = true;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Refreshes the set of texture checksums used in the last frame drawn.");

    ImGui::SameLine ();

    if (ImGui::Button (" Clear Debug "))
    {
      sel                         = -1;
      debug_tex_id                =  0;
      textures_used_last_dump.clear ();
      last_ht                     =  0;
      last_width                  =  0;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Enable On-Demand Texture Dumping",    &config.textures.on_demand_dump)) tzf::RenderFix::need_reset.graphics = true;
    
    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.7f, 0.3f, 1.f), "Enable dumping DXT compressed textures from VRAM.");
      ImGui::Separator    ();
      ImGui::Text         ("Drivers may not be able to manage texture memory as efficiently, and you should turn this option off when not modifying textures.\n\n");
      ImGui::BulletText   ("If this is your first time enabling this feature, the dump button will not work until you reload all textures in-game.");
      ImGui::EndTooltip   ();
    }

    ImGui::SameLine ();

    ImGui::Checkbox ("Highlight Selected Texture in Game",    &config.textures.highlight_debug_tex);

    ImGui::Separator ();
    ImGui::EndChild  ();

    if (list_dirty)
    {
           list_contents.clear ();
                sel = tex_dbg_idx;

      if (debug_tex_id == 0)
        last_ht = 0;


      // The underlying list is unsorted for speed, but that's not at all
      //   intuitive to humans, so sort the thing when we have the RT view open.
      std::sort ( textures_used_last_dump.begin (),
                  textures_used_last_dump.end   () );


      for ( auto it : textures_used_last_dump )
      {
        char szDesc [16] = { };

        sprintf (szDesc, "%08x", it);

        list_contents.push_back (szDesc);
      }
    }

    ImGui::BeginGroup ();

    ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

    ImGui::BeginChild ( "Item List",
                        ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                          true, ImGuiWindowFlags_AlwaysAutoResize );

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

   if (textures_used_last_dump.size ())
   {
     static      int last_sel = 0;
     static bool sel_changed  = false;

     if (sel != last_sel)
       sel_changed = true;

     last_sel = sel;

     for ( int line = 0; line < textures_used_last_dump.size (); line++)
     {
       if (line == sel)
       {
         bool selected = true;
         ImGui::Selectable (list_contents [line].c_str (), &selected);

         if (sel_changed)
         {
           ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
           sel_changed = false;
         }
       }

       else
       {
         bool selected = false;

         if (ImGui::Selectable (list_contents[line].c_str (), &selected))
         {
           sel_changed = true;
           tex_dbg_idx                 =  line;
           sel                         =  line;
           debug_tex_id                =  textures_used_last_dump [line];
         }
       }
     }
   }

   ImGui::EndChild ();

   if (ImGui::IsItemHovered ())
   {
     ImGui::BeginTooltip ();
     ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "The \"debug\" texture will appear black to make identifying textures to modify easier.");
     ImGui::Separator    ();
     ImGui::BulletText   ("Press Ctrl + Shift + [ to select the previous texture from this list");
     ImGui::BulletText   ("Press Ctrl + Shift + ] to select the next texture from this list");
     ImGui::EndTooltip   ();
   }

   ImGui::SameLine     ();
   ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

   last_ht    = std::max (last_ht,    16.0f);
   last_width = std::max (last_width, 16.0f);

   if (debug_tex_id != 0x00)
   {
     tzf::RenderFix::Texture* pTex =
       tzf::RenderFix::tex_mgr.getTexture (debug_tex_id);

     extern bool __remap_textures;
            bool has_alternate = (pTex != nullptr && pTex->d3d9_tex->pTexOverride != nullptr);

     if (pTex != nullptr)
     {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED (pTex->d3d9_tex->pTex->GetLevelDesc (0, &desc)))
        {
          ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                  ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                    (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                          ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);

          ImGui::PushStyleColor (ImGuiCol_Border, border_color);

          ImGui::BeginGroup     ();
          ImGui::BeginChild     ( "Item Selection",
                                  ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width + 24.0f),
                                (float)desc.Height + font_size * 10.0f),
                                    true,
                                      ImGuiWindowFlags_AlwaysAutoResize );

          if ((! config.textures.highlight_debug_tex) && has_alternate)
          {
            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("Click me to make this texture the visible version.");
            
            // Allow the user to toggle texture override by clicking the frame
            if (ImGui::IsItemClicked ())
              __remap_textures = false;
          }

          last_width  = (float)desc.Width;
          last_ht     = (float)desc.Height + font_size * 10.0f;


          int num_lods = pTex->d3d9_tex->pTex->GetLevelCount ();

          ImGui::Text ( "Dimensions:   %lux%lu (%lu %s)",
                          desc.Width, desc.Height,
                            num_lods, num_lods > 1 ? "LODs" : "LOD" );
          ImGui::Text ( "Format:       %ws",
                          SK_D3D9_FormatToStr (desc.Format).c_str () );
          ImGui::Text ( "Data Size:    %.2f MiB",
                          (double)pTex->d3d9_tex->tex_size / (1024.0f * 1024.0f) );
          ImGui::Text ( "Load Time:    %.6f Seconds",
                          pTex->load_time / 1000.0f );

          ImGui::Separator     ();

          if (! TZF_IsTextureDumped (debug_tex_id))
          {
            if ( ImGui::Button ("  Dump Texture to Disk  ") )
            {
              //if (! config.textures.quick_load)
                TZF_DumpTexture (desc.Format, debug_tex_id, pTex->d3d9_tex->pTex);
            }

            //if (config.textures.quick_load && ImGui::IsItemHovered ())
              //ImGui::SetTooltip ("Turn off Texture QuickLoad to use this feature.");
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ") )
            {
              TZF_DeleteDumpedTexture (desc.Format, debug_tex_id);
            }
          }

          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
          ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
          ImGui::Image           ( pTex->d3d9_tex->pTex,
                                     ImVec2 ((float)desc.Width, (float)desc.Height),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                                 );
          ImGui::EndChildFrame   ();
          ImGui::EndChild        ();
          ImGui::EndGroup        ();
          ImGui::PopStyleColor   (2);
        }
     }

     if (has_alternate)
     {
       ImGui::SameLine ();

        D3DSURFACE_DESC desc;

        if (SUCCEEDED (pTex->d3d9_tex->pTexOverride->GetLevelDesc (0, &desc)))
        {
          ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                  ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                    (__remap_textures) ? ImVec4 (0.3f,  1.0f,  0.3f, 1.0f) :
                                                         ImVec4 (0.5f,  0.5f,  0.5f, 1.0f);

          ImGui::PushStyleColor  (ImGuiCol_Border, border_color);

          ImGui::BeginGroup ();
          ImGui::BeginChild ( "Item Selection2",
                              ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width  + 24.0f),
                                                                    (float)desc.Height + font_size * 10.0f),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize );

          if (! config.textures.highlight_debug_tex)
          {
            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("Click me to make this texture the visible version.");

            // Allow the user to toggle texture override by clicking the frame
            if (ImGui::IsItemClicked ())
              __remap_textures = true;
          }


          last_width  = std::max (last_width, (float)desc.Width);
          last_ht     = std::max (last_ht,    (float)desc.Height + font_size * 10.0f);


          extern std::wstring
          SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


          bool injected  =
            (TZF_GetInjectableTexture (debug_tex_id) != nullptr),
               reloading = false;;

          int num_lods = pTex->d3d9_tex->pTexOverride->GetLevelCount ();

          ImGui::Text ( "Dimensions:   %lux%lu  (%lu %s)",
                          desc.Width, desc.Height,
                             num_lods, num_lods > 1 ? "LODs" : "LOD" );
          ImGui::Text ( "Format:       %ws",
                          SK_D3D9_FormatToStr (desc.Format).c_str () );
          ImGui::Text ( "Data Size:    %.2f MiB",
                          (double)pTex->d3d9_tex->override_size / (1024.0f * 1024.0f) );
          ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), injected ? "Injected Texture" : "Resampled Texture" );

          ImGui::Separator     ();


          if (injected)
          {
            if ( ImGui::Button ("  Reload This Texture  ") && tzf::RenderFix::tex_mgr.reloadTexture (debug_tex_id) )
            {
              reloading    = true;

              tzf::RenderFix::tex_mgr.updateOSD ();
            }
          }

          else {
            ImGui::Button ("  Resample This Texture  "); // NO-OP, but preserves alignment :P
          }

          if (! reloading)
          {
            ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
            ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
            ImGui::Image           ( pTex->d3d9_tex->pTexOverride,
                                       ImVec2 ((float)desc.Width, (float)desc.Height),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                                         ImColor (255,255,255,255), ImColor (255,255,255,128)
                                   );
            ImGui::EndChildFrame   ();
            ImGui::PopStyleColor   (1);
          }

          ImGui::EndChild        ();
          ImGui::EndGroup        ();
          ImGui::PopStyleColor   (1);
        }
      }
    }
    ImGui::EndGroup      ();
    ImGui::PopStyleColor (1);
    ImGui::PopStyleVar   (2);
  }

  if (ImGui::CollapsingHeader ("Live Render Target View"))
  {
    static float last_ht    = 256.0f;
    static float last_width = 256.0f;

    static std::vector <std::string> list_contents;
    static bool                      list_dirty     = true;
    static uintptr_t                 last_sel_ptr   =    0;
    static int                            sel       =   -1;

    std::vector <IDirect3DBaseTexture9*> render_textures =
      tzf::RenderFix::tex_mgr.getUsedRenderTargets ();

    tzf::RenderFix::tracked_rt.tracking_tex = 0;

    if (list_dirty)
    {
          sel = -1;
      int idx =  0;
          list_contents.clear ();

      // The underlying list is unsorted for speed, but that's not at all
      //   intuitive to humans, so sort the thing when we have the RT view open.
      std::sort ( render_textures.begin (),
                  render_textures.end   (),
        []( IDirect3DBaseTexture9 *a,
            IDirect3DBaseTexture9 *b )
        {
          return (uintptr_t)a < (uintptr_t)b;
        }
      );


      for ( auto it : render_textures )
      {
        char szDesc [16] = { };

        sprintf (szDesc, "%lx", (uintptr_t)it);

        list_contents.push_back (szDesc);

        if ((uintptr_t)it == last_sel_ptr) {
          sel = idx;
          tzf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
        }

        ++idx;
      }
    }

    ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

    ImGui::BeginChild ( "Item List2",
                        ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                          true, ImGuiWindowFlags_AlwaysAutoResize );

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

   if (render_textures.size ())
   {
     static      int last_sel = 0;
     static bool sel_changed  = false;

     if (sel != last_sel)
       sel_changed = true;

     last_sel = sel;

     for ( int line = 0; line < render_textures.size (); line++ )
     {
       D3DSURFACE_DESC desc;

       CComPtr <IDirect3DTexture9> pTex = nullptr;

       if (SUCCEEDED (render_textures [line]->QueryInterface (IID_PPV_ARGS (&pTex))))
       {
         if (SUCCEEDED (pTex->GetLevelDesc (0, &desc)))
         {
           if (line == sel)
           {
             bool selected = true;
             ImGui::Selectable (list_contents [line].c_str (), &selected);

             if (sel_changed)
             {
               ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
               sel_changed = false;
             }
           }

           else
           {
             bool selected = false;

             if (ImGui::Selectable (list_contents [line].c_str (), &selected))
             {
               sel_changed  = true;
               sel          =  line;
               last_sel_ptr = (uintptr_t)render_textures [sel];
               tzf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
             }
           }
         }
       }
     }
   }

   ImGui::EndChild ();

   ImGui::BeginGroup ();

   ImGui::PopStyleColor ();
   ImGui::PopStyleVar   ();

   CComPtr <IDirect3DTexture9> pTex = nullptr;

   if (render_textures.size () && sel >= 0)
     render_textures [sel]->QueryInterface (IID_PPV_ARGS (&pTex));

   if (pTex != nullptr)
   {
      D3DSURFACE_DESC desc;

      if (SUCCEEDED (pTex->GetLevelDesc (0, &desc)))
      {
        size_t shaders = std::max ( tzf::RenderFix::tracked_rt.pixel_shaders.size  (),
                                    tzf::RenderFix::tracked_rt.vertex_shaders.size () );

        // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
        float effective_width  = std::min (0.75f * ImGui::GetIO ().DisplaySize.x, (float)desc.Width  / 2.0f);
        float effective_height = std::min (0.75f * ImGui::GetIO ().DisplaySize.y, (float)desc.Height / 2.0f);

        ImGui::SameLine ();

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::BeginChild ( "Item Selection3",
                            ImVec2 ( std::max (font_size * 30.0f, effective_width  + 24.0f),
                                     std::max (256.0f,            effective_height + font_size * 4.0f + (float)shaders * font_size) ),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize );

        last_width  = effective_width;
        last_ht     = effective_height + font_size * 4.0f + (float)shaders * font_size;

        extern std::wstring
        SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


        ImGui::Text ( "Dimensions:   %lux%lu",
                        desc.Width, desc.Height/*,
                          pTex->d3d9_tex->GetLevelCount ()*/ );
        ImGui::Text ( "Format:       %ws",
                        SK_D3D9_FormatToStr (desc.Format).c_str () );

        ImGui::Separator     ();

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
        ImGui::BeginChildFrame (0, ImVec2 (effective_width + 8.0f, effective_height + 8.0f), ImGuiWindowFlags_ShowBorders);
        ImGui::Image           ( pTex,
                                   ImVec2 (effective_width, effective_height),
                                     ImVec2  (0,0),             ImVec2  (1,1),
                                     ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
        ImGui::EndChildFrame   ();

        if (shaders > 0)
        {
          ImGui::Columns (2);

          for ( auto it : tzf::RenderFix::tracked_rt.vertex_shaders )
            ImGui::Text ("Vertex Shader: %08x", it);

          ImGui::NextColumn ();

          for ( auto it : tzf::RenderFix::tracked_rt.pixel_shaders )
            ImGui::Text ("Pixel Shader: %08x", it);

          ImGui::Columns (1);
        }

        ImGui::EndChild        ();
        ImGui::PopStyleColor   (2);
      }
    }

    ImGui::EndGroup ();
  }

  if (ImGui::CollapsingHeader ("Live Shader View"))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Pixel Shaders"))
      TZF_LiveShaderClassView (tzf_shader_class::Pixel, can_scroll);

    if (ImGui::CollapsingHeader ("Vertex Shaders"))
      TZF_LiveShaderClassView (tzf_shader_class::Vertex, can_scroll);

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Live Vertex Buffer View"))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Stream 0"))
      TZF_LiveVertexStreamView (can_scroll);

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Misc. Settings"))
  {
    ImGui::TreePush ("");
    if (ImGui::Checkbox ("Dump ALL Shaders   (TZFix_Res\\dump\\shaders\\<ps|vs>_<checksum>.html)", &config.render.dump_shaders)) tzf::RenderFix::need_reset.graphics = true;
    if (ImGui::Checkbox ("Dump ALL Textures  (TZFix_Res\\dump\\textures\\<format>\\*.dds)",        &config.textures.dump))       tzf::RenderFix::need_reset.graphics = true;

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Enabling this will cause the game to run slower and waste disk space, only enable if you know what you are doing.");
      ImGui::EndTooltip   ();
    }

    ImGui::TreePop ();
  }

  ImGui::PopItemWidth ();

  if (can_scroll)
    ImGui::SetScrollY (ImGui::GetScrollY () + 5.0f * ImGui::GetFont ()->FontSize * -ImGui::GetIO ().MouseWheel);

  ImGui::End          ();

  return show_dlg;
}