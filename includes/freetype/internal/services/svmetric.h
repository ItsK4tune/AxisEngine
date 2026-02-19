


#ifndef SVMETRIC_H_
#define SVMETRIC_H_

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


  

#define FT_SERVICE_ID_METRICS_VARIATIONS  "metrics-variations"


  

  typedef FT_Error
  (*FT_HAdvance_Adjust_Func)( FT_Face  face,
                              FT_UInt  gindex,
                              FT_Int  *avalue );

  typedef FT_Error
  (*FT_LSB_Adjust_Func)( FT_Face  face,
                         FT_UInt  gindex,
                         FT_Int  *avalue );

  typedef FT_Error
  (*FT_RSB_Adjust_Func)( FT_Face  face,
                         FT_UInt  gindex,
                         FT_Int  *avalue );

  

  typedef FT_Error
  (*FT_VAdvance_Adjust_Func)( FT_Face  face,
                              FT_UInt  gindex,
                              FT_Int  *avalue );

  typedef FT_Error
  (*FT_TSB_Adjust_Func)( FT_Face  face,
                         FT_UInt  gindex,
                         FT_Int  *avalue );

  typedef FT_Error
  (*FT_BSB_Adjust_Func)( FT_Face  face,
                         FT_UInt  gindex,
                         FT_Int  *avalue );

  typedef FT_Error
  (*FT_VOrg_Adjust_Func)( FT_Face  face,
                          FT_UInt  gindex,
                          FT_Int  *avalue );

  

  typedef void
  (*FT_Metrics_Adjust_Func)( FT_Face  face );


  FT_DEFINE_SERVICE( MetricsVariations )
  {
    FT_HAdvance_Adjust_Func  hadvance_adjust;
    FT_LSB_Adjust_Func       lsb_adjust;
    FT_RSB_Adjust_Func       rsb_adjust;

    FT_VAdvance_Adjust_Func  vadvance_adjust;
    FT_TSB_Adjust_Func       tsb_adjust;
    FT_BSB_Adjust_Func       bsb_adjust;
    FT_VOrg_Adjust_Func      vorg_adjust;

    FT_Metrics_Adjust_Func   metrics_adjust;
  };


#define FT_DEFINE_SERVICE_METRICSVARIATIONSREC( class_,            \
                                                hadvance_adjust_,  \
                                                lsb_adjust_,       \
                                                rsb_adjust_,       \
                                                vadvance_adjust_,  \
                                                tsb_adjust_,       \
                                                bsb_adjust_,       \
                                                vorg_adjust_,      \
                                                metrics_adjust_  ) \
  static const FT_Service_MetricsVariationsRec  class_ =           \
  {                                                                \
    hadvance_adjust_,                                              \
    lsb_adjust_,                                                   \
    rsb_adjust_,                                                   \
    vadvance_adjust_,                                              \
    tsb_adjust_,                                                   \
    bsb_adjust_,                                                   \
    vorg_adjust_,                                                  \
    metrics_adjust_                                                \
  };

  


FT_END_HEADER

#endif 



