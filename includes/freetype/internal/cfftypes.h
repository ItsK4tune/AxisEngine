


#ifndef CFFTYPES_H_
#define CFFTYPES_H_


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_SERVICE_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  
  typedef struct  CFF_IndexRec_
  {
    FT_Stream  stream;
    FT_ULong   start;
    FT_UInt    hdr_size;
    FT_UInt    count;
    FT_Byte    off_size;
    FT_ULong   data_offset;
    FT_ULong   data_size;

    FT_ULong*  offsets;
    FT_Byte*   bytes;

  } CFF_IndexRec, *CFF_Index;


  typedef struct  CFF_EncodingRec_
  {
    FT_UInt     format;
    FT_ULong    offset;

    FT_UInt     count;
    FT_UShort   sids [256];  
    FT_UShort   codes[256];

  } CFF_EncodingRec, *CFF_Encoding;


  typedef struct  CFF_CharsetRec_
  {

    FT_UInt     format;
    FT_ULong    offset;

    FT_UShort*  sids;
    FT_UShort*  cids;       
                            
    FT_UInt     max_cid;
    FT_UInt     num_glyphs;

  } CFF_CharsetRec, *CFF_Charset;


  

  typedef struct  CFF_VarData_
  {
#if 0
    FT_UInt  itemCount;       
    FT_UInt  shortDeltaCount; 
#endif

    FT_UInt   regionIdxCount; 
    FT_UInt*  regionIndices;  
                              
  } CFF_VarData;


  
  typedef struct  CFF_AxisCoords_
  {
    FT_Fixed  startCoord;
    FT_Fixed  peakCoord;      
    FT_Fixed  endCoord;

  } CFF_AxisCoords;


  typedef struct  CFF_VarRegion_
  {
    CFF_AxisCoords*  axisList;      

  } CFF_VarRegion;


  typedef struct  CFF_VStoreRec_
  {
    FT_UInt         dataCount;
    CFF_VarData*    varData;        
                                    
    FT_UShort       axisCount;
    FT_UInt         regionCount;    
    CFF_VarRegion*  varRegionList;

  } CFF_VStoreRec, *CFF_VStore;


  
  typedef struct CFF_FontRec_*  CFF_Font;


  
  
  
  
  
  
  
  
  
  
  
  typedef struct  CFF_BlendRec_
  {
    FT_Bool    builtBV;        
    FT_Bool    usedBV;         
    CFF_Font   font;           
    FT_UInt    lastVsindex;    
    FT_UInt    lenNDV;         
    FT_Fixed*  lastNDV;        
    FT_UInt    lenBV;          
    FT_Int32*  BV;             

  } CFF_BlendRec, *CFF_Blend;


  typedef struct  CFF_FontRecDictRec_
  {
    FT_UInt    version;
    FT_UInt    notice;
    FT_UInt    copyright;
    FT_UInt    full_name;
    FT_UInt    family_name;
    FT_UInt    weight;
    FT_Bool    is_fixed_pitch;
    FT_Fixed   italic_angle;
    FT_Fixed   underline_position;
    FT_Fixed   underline_thickness;
    FT_Int     paint_type;
    FT_Int     charstring_type;
    FT_Matrix  font_matrix;
    FT_Bool    has_font_matrix;
    FT_ULong   units_per_em;  
    FT_Vector  font_offset;
    FT_ULong   unique_id;
    FT_BBox    font_bbox;
    FT_Pos     stroke_width;
    FT_ULong   charset_offset;
    FT_ULong   encoding_offset;
    FT_ULong   charstrings_offset;
    FT_ULong   private_offset;
    FT_ULong   private_size;
    FT_Long    synthetic_base;
    FT_UInt    embedded_postscript;

    
    FT_UInt    cid_registry;
    FT_UInt    cid_ordering;
    FT_Long    cid_supplement;

    FT_Long    cid_font_version;
    FT_Long    cid_font_revision;
    FT_Long    cid_font_type;
    FT_ULong   cid_count;
    FT_ULong   cid_uid_base;
    FT_ULong   cid_fd_array_offset;
    FT_ULong   cid_fd_select_offset;
    FT_UInt    cid_font_name;

    
    
    
    FT_UShort  num_designs;
    FT_UShort  num_axes;

    
    FT_ULong   vstore_offset;
    FT_UInt    maxstack;

  } CFF_FontRecDictRec, *CFF_FontRecDict;


  
  typedef struct CFF_SubFontRec_*  CFF_SubFont;


  typedef struct  CFF_PrivateRec_
  {
    FT_Byte   num_blue_values;
    FT_Byte   num_other_blues;
    FT_Byte   num_family_blues;
    FT_Byte   num_family_other_blues;

    FT_Pos    blue_values[14];
    FT_Pos    other_blues[10];
    FT_Pos    family_blues[14];
    FT_Pos    family_other_blues[10];

    FT_Fixed  blue_scale;
    FT_Pos    blue_shift;
    FT_Pos    blue_fuzz;
    FT_Pos    standard_width;
    FT_Pos    standard_height;

    FT_Byte   num_snap_widths;
    FT_Byte   num_snap_heights;
    FT_Pos    snap_widths[13];
    FT_Pos    snap_heights[13];
    FT_Bool   force_bold;
    FT_Fixed  force_bold_threshold;
    FT_Int    lenIV;
    FT_Int    language_group;
    FT_Fixed  expansion_factor;
    FT_Long   initial_random_seed;
    FT_ULong  local_subrs_offset;
    FT_Pos    default_width;
    FT_Pos    nominal_width;

    
    FT_UInt      vsindex;
    CFF_SubFont  subfont;

  } CFF_PrivateRec, *CFF_Private;


  typedef struct  CFF_FDSelectRec_
  {
    FT_Byte   format;
    FT_UInt   range_count;

    
    FT_Byte*  data;
    FT_UInt   data_size;

    
    FT_UInt   cache_first;
    FT_UInt   cache_count;
    FT_Byte   cache_fd;

  } CFF_FDSelectRec, *CFF_FDSelect;


  
  
  typedef struct  CFF_SubFontRec_
  {
    CFF_FontRecDictRec  font_dict;
    CFF_PrivateRec      private_dict;

    
    CFF_BlendRec  blend;      
    FT_UInt       lenNDV;     
    FT_Fixed*     NDV;        

    
    
    
    
    
    
    

    FT_Byte*  blend_stack;    
    FT_Byte*  blend_top;      
    FT_UInt   blend_used;     
    FT_UInt   blend_alloc;    

    CFF_IndexRec  local_subrs_index;
    FT_Byte**     local_subrs; 
                               

    FT_UInt32  random;

  } CFF_SubFontRec;


#define CFF_MAX_CID_FONTS  256


  typedef struct  CFF_FontRec_
  {
    FT_Library       library;
    FT_Stream        stream;
    FT_Memory        memory;        
    FT_ULong         base_offset;   
    FT_UInt          num_faces;
    FT_UInt          num_glyphs;

    FT_Byte          version_major;
    FT_Byte          version_minor;
    FT_Byte          header_size;

    FT_UInt          top_dict_length;   

    FT_Bool          cff2;

    CFF_IndexRec     name_index;
    CFF_IndexRec     top_dict_index;
    CFF_IndexRec     global_subrs_index;

    CFF_EncodingRec  encoding;
    CFF_CharsetRec   charset;

    CFF_IndexRec     charstrings_index;
    CFF_IndexRec     font_dict_index;
    CFF_IndexRec     private_index;
    CFF_IndexRec     local_subrs_index;

    FT_String*       font_name;

    
    FT_Byte**        global_subrs;

    
    FT_UInt          num_strings;
    FT_Byte**        strings;
    FT_Byte*         string_pool;
    FT_ULong         string_pool_size;

    CFF_SubFontRec   top_font;
    FT_UInt          num_subfonts;
    CFF_SubFont      subfonts[CFF_MAX_CID_FONTS];

    CFF_FDSelectRec  fd_select;

    
    PSHinter_Service  pshinter;

    
    FT_Service_PsCMaps  psnames;

    
    const void*  cffload;

    
    PS_FontInfoRec*  font_info;   

    
    FT_String*       registry;
    FT_String*       ordering;

    
    FT_Generic       cf2_instance;

    
    CFF_VStoreRec    vstore;        

    
    PS_FontExtraRec*  font_extra;

  } CFF_FontRec;


FT_END_HEADER

#endif 



