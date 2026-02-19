


#ifndef SVPROP_H_
#define SVPROP_H_


FT_BEGIN_HEADER


#define FT_SERVICE_ID_PROPERTIES  "properties"


  typedef FT_Error
  (*FT_Properties_SetFunc)( FT_Module    module,
                            const char*  property_name,
                            const void*  value,
                            FT_Bool      value_is_string );

  typedef FT_Error
  (*FT_Properties_GetFunc)( FT_Module    module,
                            const char*  property_name,
                            void*        value );


  FT_DEFINE_SERVICE( Properties )
  {
    FT_Properties_SetFunc  set_property;
    FT_Properties_GetFunc  get_property;
  };


#define FT_DEFINE_SERVICE_PROPERTIESREC( class_,          \
                                         set_property_,   \
                                         get_property_ )  \
  static const FT_Service_PropertiesRec  class_ =         \
  {                                                       \
    set_property_,                                        \
    get_property_                                         \
  };

  


FT_END_HEADER


#endif 



