


#ifndef FTGLOADR_H_
#define FTGLOADR_H_


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  
  typedef struct  FT_SubGlyphRec_
  {
    FT_Int     index;
    FT_UShort  flags;
    FT_Int     arg1;
    FT_Int     arg2;
    FT_Matrix  transform;

  } FT_SubGlyphRec;


  typedef struct  FT_GlyphLoadRec_
  {
    FT_Outline   outline;       
    FT_Vector*   extra_points;  
    FT_Vector*   extra_points2; 
    FT_UInt      num_subglyphs; 
    FT_SubGlyph  subglyphs;     

  } FT_GlyphLoadRec, *FT_GlyphLoad;


  typedef struct  FT_GlyphLoaderRec_
  {
    FT_Memory        memory;
    FT_UInt          max_points;
    FT_UInt          max_contours;
    FT_UInt          max_subglyphs;
    FT_Bool          use_extra;

    FT_GlyphLoadRec  base;
    FT_GlyphLoadRec  current;

    void*            other;            

  } FT_GlyphLoaderRec, *FT_GlyphLoader;


  
  FT_BASE( FT_Error )
  FT_GlyphLoader_New( FT_Memory        memory,
                      FT_GlyphLoader  *aloader );

  
  FT_BASE( FT_Error )
  FT_GlyphLoader_CreateExtra( FT_GlyphLoader  loader );

  
  FT_BASE( void )
  FT_GlyphLoader_Done( FT_GlyphLoader  loader );

  
  FT_BASE( void )
  FT_GlyphLoader_Reset( FT_GlyphLoader  loader );

  
  FT_BASE( void )
  FT_GlyphLoader_Rewind( FT_GlyphLoader  loader );

  
  
  FT_BASE( FT_Error )
  FT_GlyphLoader_CheckPoints( FT_GlyphLoader  loader,
                              FT_UInt         n_points,
                              FT_UInt         n_contours );


#define FT_GLYPHLOADER_CHECK_P( _loader, _count )       \
  ( (_count) == 0                                    || \
    ( (FT_UInt)(_loader)->base.outline.n_points    +    \
      (FT_UInt)(_loader)->current.outline.n_points +    \
      (FT_UInt)(_count) ) <= (_loader)->max_points   )

#define FT_GLYPHLOADER_CHECK_C( _loader, _count )         \
  ( (_count) == 0                                      || \
    ( (FT_UInt)(_loader)->base.outline.n_contours    +    \
      (FT_UInt)(_loader)->current.outline.n_contours +    \
      (FT_UInt)(_count) ) <= (_loader)->max_contours   )

#define FT_GLYPHLOADER_CHECK_POINTS( _loader, _points, _contours ) \
  ( ( FT_GLYPHLOADER_CHECK_P( _loader, _points )   &&              \
      FT_GLYPHLOADER_CHECK_C( _loader, _contours ) )               \
    ? 0                                                            \
    : FT_GlyphLoader_CheckPoints( (_loader),                       \
                                  (FT_UInt)(_points),              \
                                  (FT_UInt)(_contours) ) )


  
  
  FT_BASE( FT_Error )
  FT_GlyphLoader_CheckSubGlyphs( FT_GlyphLoader  loader,
                                 FT_UInt         n_subs );

  
  FT_BASE( void )
  FT_GlyphLoader_Prepare( FT_GlyphLoader  loader );

  
  FT_BASE( void )
  FT_GlyphLoader_Add( FT_GlyphLoader  loader );

 


FT_END_HEADER

#endif 



