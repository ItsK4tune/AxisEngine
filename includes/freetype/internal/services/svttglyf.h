

#ifndef SVTTGLYF_H_
#define SVTTGLYF_H_

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_TT_GLYF  "tt-glyf"


  typedef FT_ULong
  (*TT_Glyf_GetLocationFunc)( FT_Face    face,
                              FT_UInt    gindex,
                              FT_ULong  *psize );

  FT_DEFINE_SERVICE( TTGlyf )
  {
    TT_Glyf_GetLocationFunc  get_location;
  };


#define FT_DEFINE_SERVICE_TTGLYFREC( class_, get_location_ )  \
  static const FT_Service_TTGlyfRec  class_ =                 \
  {                                                           \
    get_location_                                             \
  };

  


FT_END_HEADER

#endif 



