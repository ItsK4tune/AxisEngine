



#ifndef SVTTCMAP_H_
#define SVTTCMAP_H_

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_TT_CMAP  "tt-cmaps"


  
  typedef struct  TT_CMapInfo_
  {
    FT_ULong  language;
    FT_Long   format;

  } TT_CMapInfo;


  typedef FT_Error
  (*TT_CMap_Info_GetFunc)( FT_CharMap    charmap,
                           TT_CMapInfo  *cmap_info );


  FT_DEFINE_SERVICE( TTCMaps )
  {
    TT_CMap_Info_GetFunc  get_cmap_info;
  };


#define FT_DEFINE_SERVICE_TTCMAPSREC( class_, get_cmap_info_ )  \
  static const FT_Service_TTCMapsRec  class_ =                  \
  {                                                             \
    get_cmap_info_                                              \
  };

  


FT_END_HEADER

#endif 



