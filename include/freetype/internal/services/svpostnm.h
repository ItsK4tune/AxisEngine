


#ifndef SVPOSTNM_H_
#define SVPOSTNM_H_

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER

  

#define FT_SERVICE_ID_POSTSCRIPT_FONT_NAME  "postscript-font-name"


  typedef const char*
  (*FT_PsName_GetFunc)( FT_Face  face );


  FT_DEFINE_SERVICE( PsFontName )
  {
    FT_PsName_GetFunc  get_ps_font_name;
  };


#define FT_DEFINE_SERVICE_PSFONTNAMEREC( class_, get_ps_font_name_ ) \
  static const FT_Service_PsFontNameRec  class_ =                    \
  {                                                                  \
    get_ps_font_name_                                                \
  };

  


FT_END_HEADER


#endif 



