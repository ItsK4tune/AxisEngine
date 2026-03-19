


#ifndef WOFFTYPES_H_
#define WOFFTYPES_H_


#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  
  typedef struct  WOFF_HeaderRec_
  {
    FT_ULong   signature;
    FT_ULong   flavor;
    FT_ULong   length;
    FT_UShort  num_tables;
    FT_UShort  reserved;
    FT_ULong   totalSfntSize;
    FT_UShort  majorVersion;
    FT_UShort  minorVersion;
    FT_ULong   metaOffset;
    FT_ULong   metaLength;
    FT_ULong   metaOrigLength;
    FT_ULong   privOffset;
    FT_ULong   privLength;

  } WOFF_HeaderRec, *WOFF_Header;


  
  typedef struct  WOFF_TableRec_
  {
    FT_ULong  Tag;           
    FT_ULong  Offset;        
    FT_ULong  CompLength;    
    FT_ULong  OrigLength;    
    FT_ULong  CheckSum;      

    FT_ULong  OrigOffset;    
                             
  } WOFF_TableRec, *WOFF_Table;


  
  typedef struct  WOFF2_TtcFontRec_
  {
    FT_ULong    flavor;
    FT_UShort   num_tables;
    FT_UShort*  table_indices;

  } WOFF2_TtcFontRec, *WOFF2_TtcFont;


  
  typedef struct  WOFF2_HeaderRec_
  {
    FT_ULong   signature;
    FT_ULong   flavor;
    FT_ULong   length;
    FT_UShort  num_tables;
    FT_ULong   totalSfntSize;
    FT_ULong   totalCompressedSize;
    FT_ULong   metaOffset;
    FT_ULong   metaLength;
    FT_ULong   metaOrigLength;
    FT_ULong   privOffset;
    FT_ULong   privLength;

    FT_ULong   uncompressed_size;    
    FT_ULong   compressed_offset;    
    FT_ULong   header_version;       
    FT_UShort  num_fonts;            
    FT_ULong   actual_sfnt_size;     

    WOFF2_TtcFont  ttc_fonts;        

  } WOFF2_HeaderRec, *WOFF2_Header;


  
  typedef struct  WOFF2_TableRec_
  {
    FT_Byte   FlagByte;           
    FT_ULong  Tag;                
    FT_ULong  dst_length;         
    FT_ULong  TransformLength;    

    FT_ULong  flags;              
    FT_ULong  src_offset;         
    FT_ULong  src_length;         
    FT_ULong  dst_offset;         

  } WOFF2_TableRec, *WOFF2_Table;


  
  typedef struct  WOFF2_InfoRec_
  {
    FT_ULong   header_checksum;
    FT_UShort  num_glyphs;
    FT_UShort  num_hmetrics;
    FT_Short*  x_mins;

    WOFF2_Table  glyf_table;
    WOFF2_Table  loca_table;
    WOFF2_Table  head_table;

  } WOFF2_InfoRec, *WOFF2_Info;


  
  typedef struct  WOFF2_SubstreamRec_
  {
    FT_ULong  start;
    FT_ULong  offset;
    FT_ULong  size;

  } WOFF2_SubstreamRec, *WOFF2_Substream;


  
  typedef struct  WOFF2_PointRec_
  {
    FT_Int   x;
    FT_Int   y;
    FT_Bool  on_curve;

  } WOFF2_PointRec, *WOFF2_Point;


FT_END_HEADER

#endif 



