

#ifndef AVUTIL_CPU_H
#define AVUTIL_CPU_H

#include <stddef.h>
#include "version.h"

#define AV_CPU_FLAG_FORCE    0x80000000 

    
#define AV_CPU_FLAG_MMX          0x0001 
#define AV_CPU_FLAG_MMXEXT       0x0002 
#define AV_CPU_FLAG_MMX2         0x0002 
#define AV_CPU_FLAG_3DNOW        0x0004 
#define AV_CPU_FLAG_SSE          0x0008 
#define AV_CPU_FLAG_SSE2         0x0010 
#define AV_CPU_FLAG_SSE2SLOW 0x40000000 
                                        
#define AV_CPU_FLAG_3DNOWEXT     0x0020 
#define AV_CPU_FLAG_SSE3         0x0040 
#define AV_CPU_FLAG_SSE3SLOW 0x20000000 
                                        
#define AV_CPU_FLAG_SSSE3        0x0080 
#define AV_CPU_FLAG_SSSE3SLOW 0x4000000 
#define AV_CPU_FLAG_ATOM     0x10000000 
#define AV_CPU_FLAG_SSE4         0x0100 
#define AV_CPU_FLAG_SSE42        0x0200 
#define AV_CPU_FLAG_AESNI       0x80000 
#define AV_CPU_FLAG_AVX          0x4000 
#define AV_CPU_FLAG_AVXSLOW   0x8000000 
#define AV_CPU_FLAG_XOP          0x0400 
#define AV_CPU_FLAG_FMA4         0x0800 
#define AV_CPU_FLAG_CMOV         0x1000 
#define AV_CPU_FLAG_AVX2         0x8000 
#define AV_CPU_FLAG_FMA3        0x10000 
#define AV_CPU_FLAG_BMI1        0x20000 
#define AV_CPU_FLAG_BMI2        0x40000 
#define AV_CPU_FLAG_AVX512     0x100000 
#define AV_CPU_FLAG_AVX512ICL  0x200000 
#define AV_CPU_FLAG_SLOW_GATHER  0x2000000 

#define AV_CPU_FLAG_ALTIVEC      0x0001 
#define AV_CPU_FLAG_VSX          0x0002 
#define AV_CPU_FLAG_POWER8       0x0004 

#define AV_CPU_FLAG_ARMV5TE      (1 << 0)
#define AV_CPU_FLAG_ARMV6        (1 << 1)
#define AV_CPU_FLAG_ARMV6T2      (1 << 2)
#define AV_CPU_FLAG_VFP          (1 << 3)
#define AV_CPU_FLAG_VFPV3        (1 << 4)
#define AV_CPU_FLAG_NEON         (1 << 5)
#define AV_CPU_FLAG_ARMV8        (1 << 6)
#define AV_CPU_FLAG_VFP_VM       (1 << 7) 
#define AV_CPU_FLAG_DOTPROD      (1 << 8)
#define AV_CPU_FLAG_I8MM         (1 << 9)
#define AV_CPU_FLAG_SVE          (1 <<10)
#define AV_CPU_FLAG_SVE2         (1 <<11)
#define AV_CPU_FLAG_SETEND       (1 <<16)

#define AV_CPU_FLAG_MMI          (1 << 0)
#define AV_CPU_FLAG_MSA          (1 << 1)


#define AV_CPU_FLAG_LSX          (1 << 0)
#define AV_CPU_FLAG_LASX         (1 << 1)


#define AV_CPU_FLAG_RVI          (1 << 0) 
#if FF_API_RISCV_FD_ZBA
#define AV_CPU_FLAG_RVF          (1 << 1) 
#define AV_CPU_FLAG_RVD          (1 << 2) 
#endif
#define AV_CPU_FLAG_RVV_I32      (1 << 3) 
#define AV_CPU_FLAG_RVV_F32      (1 << 4) 
#define AV_CPU_FLAG_RVV_I64      (1 << 5) 
#define AV_CPU_FLAG_RVV_F64      (1 << 6) 
#define AV_CPU_FLAG_RVB_BASIC    (1 << 7) 
#if FF_API_RISCV_FD_ZBA
#define AV_CPU_FLAG_RVB_ADDR     (1 << 8) 
#endif
#define AV_CPU_FLAG_RV_ZVBB      (1 << 9) 
#define AV_CPU_FLAG_RV_MISALIGNED (1 <<10) 
#define AV_CPU_FLAG_RVB          (1 <<11) 


#define AV_CPU_FLAG_SIMD128      (1 << 0)


int av_get_cpu_flags(void);


void av_force_cpu_flags(int flags);


int av_parse_cpu_caps(unsigned *flags, const char *s);


int av_cpu_count(void);


void av_cpu_force_count(int count);


size_t av_cpu_max_align(void);

#endif 
