


#ifndef SVBDF_H_
#define SVBDF_H_

#include FT_BDF_H
#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_BDF  "bdf"

  typedef FT_Error
  (*FT_BDF_GetCharsetIdFunc)( FT_Face       face,
                              const char*  *acharset_encoding,
                              const char*  *acharset_registry );

  typedef FT_Error
  (*FT_BDF_GetPropertyFunc)( FT_Face           face,
                             const char*       prop_name,
                             BDF_PropertyRec  *aproperty );


  FT_DEFINE_SERVICE( BDF )
  {
    FT_BDF_GetCharsetIdFunc  get_charset_id;
    FT_BDF_GetPropertyFunc   get_property;
  };


#define FT_DEFINE_SERVICE_BDFRec( class_,                                \
                                  get_charset_id_,                       \
                                  get_property_ )                        \
  static const FT_Service_BDFRec  class_ =                               \
  {                                                                      \
    get_charset_id_, get_property_                                       \
  };

  


FT_END_HEADER


#endif 



