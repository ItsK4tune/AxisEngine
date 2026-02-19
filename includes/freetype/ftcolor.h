


#ifndef FTCOLOR_H_
#define FTCOLOR_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  


  
  typedef struct  FT_Color_
  {
    FT_Byte  blue;
    FT_Byte  green;
    FT_Byte  red;
    FT_Byte  alpha;

  } FT_Color;


  
#define FT_PALETTE_FOR_LIGHT_BACKGROUND  0x01
#define FT_PALETTE_FOR_DARK_BACKGROUND   0x02


  
  typedef struct  FT_Palette_Data_ {
    FT_UShort         num_palettes;
    const FT_UShort*  palette_name_ids;
    const FT_UShort*  palette_flags;

    FT_UShort         num_palette_entries;
    const FT_UShort*  palette_entry_name_ids;

  } FT_Palette_Data;


  
  FT_EXPORT( FT_Error )
  FT_Palette_Data_Get( FT_Face           face,
                       FT_Palette_Data  *apalette );


  
  FT_EXPORT( FT_Error )
  FT_Palette_Select( FT_Face     face,
                     FT_UShort   palette_index,
                     FT_Color*  *apalette );


  
  FT_EXPORT( FT_Error )
  FT_Palette_Set_Foreground_Color( FT_Face   face,
                                   FT_Color  foreground_color );

  


FT_END_HEADER

#endif 



