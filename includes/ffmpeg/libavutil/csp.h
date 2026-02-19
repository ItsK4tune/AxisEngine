

#ifndef AVUTIL_CSP_H
#define AVUTIL_CSP_H

#include "pixfmt.h"
#include "rational.h"






typedef struct AVLumaCoefficients {
    AVRational cr, cg, cb;
} AVLumaCoefficients;


typedef struct AVCIExy {
    AVRational x, y;
} AVCIExy;


typedef struct AVPrimaryCoefficients {
    AVCIExy r, g, b;
} AVPrimaryCoefficients;


typedef AVCIExy AVWhitepointCoefficients;


typedef struct AVColorPrimariesDesc {
    AVWhitepointCoefficients wp;
    AVPrimaryCoefficients prim;
} AVColorPrimariesDesc;


typedef double (*av_csp_trc_function)(double);


const AVLumaCoefficients *av_csp_luma_coeffs_from_avcsp(enum AVColorSpace csp);


const AVColorPrimariesDesc *av_csp_primaries_desc_from_id(enum AVColorPrimaries prm);


enum AVColorPrimaries av_csp_primaries_id_from_desc(const AVColorPrimariesDesc *prm);


double av_csp_approximate_trc_gamma(enum AVColorTransferCharacteristic trc);


av_csp_trc_function av_csp_trc_func_from_id(enum AVColorTransferCharacteristic trc);


av_csp_trc_function av_csp_trc_func_inv_from_id(enum AVColorTransferCharacteristic trc);


typedef void (*av_csp_eotf_function)(double Lw, double Lb, double c[3]);


av_csp_eotf_function av_csp_itu_eotf(enum AVColorTransferCharacteristic trc);


av_csp_eotf_function av_csp_itu_eotf_inv(enum AVColorTransferCharacteristic trc);



#endif 
