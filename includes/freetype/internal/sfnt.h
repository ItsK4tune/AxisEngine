


#ifndef SFNT_H_
#define SFNT_H_


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_TRUETYPE_TYPES_H
#include FT_INTERNAL_WOFF_TYPES_H


FT_BEGIN_HEADER


  
  typedef FT_Error
  (*TT_Init_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  
  typedef FT_Error
  (*TT_Load_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  
  typedef void
  (*TT_Done_Face_Func)( TT_Face  face );


  
  typedef FT_Error
  (*TT_Load_Any_Func)( TT_Face    face,
                       FT_ULong   tag,
                       FT_Long    offset,
                       FT_Byte   *buffer,
                       FT_ULong*  length );


  
  typedef FT_Error
  (*TT_Find_SBit_Image_Func)( TT_Face          face,
                              FT_UInt          glyph_index,
                              FT_ULong         strike_index,
                              TT_SBit_Range   *arange,
                              TT_SBit_Strike  *astrike,
                              FT_ULong        *aglyph_offset );


  
  typedef FT_Error
  (*TT_Load_SBit_Metrics_Func)( FT_Stream        stream,
                                TT_SBit_Range    range,
                                TT_SBit_Metrics  metrics );


  
  typedef FT_Error
  (*TT_Load_SBit_Image_Func)( TT_Face              face,
                              FT_ULong             strike_index,
                              FT_UInt              glyph_index,
                              FT_UInt              load_flags,
                              FT_Stream            stream,
                              FT_Bitmap           *amap,
                              TT_SBit_MetricsRec  *ametrics );


  
  typedef FT_Error
  (*TT_Set_SBit_Strike_Func)( TT_Face          face,
                              FT_Size_Request  req,
                              FT_ULong*        astrike_index );


  
  typedef FT_Error
  (*TT_Load_Strike_Metrics_Func)( TT_Face           face,
                                  FT_ULong          strike_index,
                                  FT_Size_Metrics*  metrics );


  
  typedef FT_Error
  (*TT_Get_PS_Name_Func)( TT_Face      face,
                          FT_UInt      idx,
                          FT_String**  PSname );


  
  typedef FT_Error
  (*TT_Load_Metrics_Func)( TT_Face    face,
                           FT_Stream  stream,
                           FT_Bool    vertical );


  
  typedef void
  (*TT_Get_Metrics_Func)( TT_Face     face,
                          FT_Bool     vertical,
                          FT_UInt     gindex,
                          FT_Short*   abearing,
                          FT_UShort*  aadvance );


  
  typedef FT_Error
  (*TT_Set_Palette_Func)( TT_Face  face,
                          FT_UInt  idx );


  
  typedef FT_Bool
  (*TT_Get_Colr_Layer_Func)( TT_Face            face,
                             FT_UInt            base_glyph,
                             FT_UInt           *aglyph_index,
                             FT_UInt           *acolor_index,
                             FT_LayerIterator*  iterator );


  
  typedef FT_Error
  (*TT_Blend_Colr_Func)( TT_Face       face,
                         FT_UInt       color_index,
                         FT_GlyphSlot  base_glyph,
                         FT_GlyphSlot  new_glyph );


  
  typedef FT_Error
  (*TT_Get_Name_Func)( TT_Face      face,
                       FT_UShort    nameid,
                       FT_String**  name );


  
  typedef FT_Bool
  (*TT_Get_Name_ID_Func)( TT_Face    face,
                          FT_UShort  nameid,
                          FT_Int    *win,
                          FT_Int    *apple );


  
  typedef FT_Error
  (*TT_Load_Table_Func)( TT_Face    face,
                         FT_Stream  stream );


  
  typedef void
  (*TT_Free_Table_Func)( TT_Face  face );


  
  typedef FT_Int
  (*TT_Face_GetKerningFunc)( TT_Face  face,
                             FT_UInt  left_glyph,
                             FT_UInt  right_glyph );


  
  typedef struct  SFNT_Interface_
  {
    TT_Loader_GotoTableFunc      goto_table;

    TT_Init_Face_Func            init_face;
    TT_Load_Face_Func            load_face;
    TT_Done_Face_Func            done_face;
    FT_Module_Requester          get_interface;

    TT_Load_Any_Func             load_any;

    
    
    TT_Load_Table_Func           load_head;
    TT_Load_Metrics_Func         load_hhea;
    TT_Load_Table_Func           load_cmap;
    TT_Load_Table_Func           load_maxp;
    TT_Load_Table_Func           load_os2;
    TT_Load_Table_Func           load_post;

    TT_Load_Table_Func           load_name;
    TT_Free_Table_Func           free_name;

    
    TT_Load_Table_Func           load_kern;

    TT_Load_Table_Func           load_gasp;
    TT_Load_Table_Func           load_pclt;

    
    
    TT_Load_Table_Func           load_bhed;

    TT_Load_SBit_Image_Func      load_sbit_image;

    
    TT_Get_PS_Name_Func          get_psname;
    TT_Free_Table_Func           free_psnames;

    

    
    TT_Face_GetKerningFunc       get_kerning;

    

    
    
    TT_Load_Table_Func           load_font_dir;
    TT_Load_Metrics_Func         load_hmtx;

    TT_Load_Table_Func           load_eblc;
    TT_Free_Table_Func           free_eblc;

    TT_Set_SBit_Strike_Func      set_sbit_strike;
    TT_Load_Strike_Metrics_Func  load_strike_metrics;

    TT_Load_Table_Func           load_cpal;
    TT_Load_Table_Func           load_colr;
    TT_Free_Table_Func           free_cpal;
    TT_Free_Table_Func           free_colr;
    TT_Set_Palette_Func          set_palette;
    TT_Get_Colr_Layer_Func       get_colr_layer;
    TT_Blend_Colr_Func           colr_blend;

    TT_Get_Metrics_Func          get_metrics;

    TT_Get_Name_Func             get_name;
    TT_Get_Name_ID_Func          get_name_id;

  } SFNT_Interface;


  
  typedef SFNT_Interface*   SFNT_Service;


#define FT_DEFINE_SFNT_INTERFACE(        \
          class_,                        \
          goto_table_,                   \
          init_face_,                    \
          load_face_,                    \
          done_face_,                    \
          get_interface_,                \
          load_any_,                     \
          load_head_,                    \
          load_hhea_,                    \
          load_cmap_,                    \
          load_maxp_,                    \
          load_os2_,                     \
          load_post_,                    \
          load_name_,                    \
          free_name_,                    \
          load_kern_,                    \
          load_gasp_,                    \
          load_pclt_,                    \
          load_bhed_,                    \
          load_sbit_image_,              \
          get_psname_,                   \
          free_psnames_,                 \
          get_kerning_,                  \
          load_font_dir_,                \
          load_hmtx_,                    \
          load_eblc_,                    \
          free_eblc_,                    \
          set_sbit_strike_,              \
          load_strike_metrics_,          \
          load_cpal_,                    \
          load_colr_,                    \
          free_cpal_,                    \
          free_colr_,                    \
          set_palette_,                  \
          get_colr_layer_,               \
          colr_blend_,                   \
          get_metrics_,                  \
          get_name_,                     \
          get_name_id_ )                 \
  static const SFNT_Interface  class_ =  \
  {                                      \
    goto_table_,                         \
    init_face_,                          \
    load_face_,                          \
    done_face_,                          \
    get_interface_,                      \
    load_any_,                           \
    load_head_,                          \
    load_hhea_,                          \
    load_cmap_,                          \
    load_maxp_,                          \
    load_os2_,                           \
    load_post_,                          \
    load_name_,                          \
    free_name_,                          \
    load_kern_,                          \
    load_gasp_,                          \
    load_pclt_,                          \
    load_bhed_,                          \
    load_sbit_image_,                    \
    get_psname_,                         \
    free_psnames_,                       \
    get_kerning_,                        \
    load_font_dir_,                      \
    load_hmtx_,                          \
    load_eblc_,                          \
    free_eblc_,                          \
    set_sbit_strike_,                    \
    load_strike_metrics_,                \
    load_cpal_,                          \
    load_colr_,                          \
    free_cpal_,                          \
    free_colr_,                          \
    set_palette_,                        \
    get_colr_layer_,                     \
    colr_blend_,                         \
    get_metrics_,                        \
    get_name_,                           \
    get_name_id_                         \
  };


FT_END_HEADER

#endif 



