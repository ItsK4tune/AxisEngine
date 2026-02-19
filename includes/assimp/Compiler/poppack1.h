










#ifndef AI_PUSHPACK_IS_DEFINED
#	error pushpack1.h must be included after poppack1.h
#endif


#if (defined(_MSC_VER) && !defined(__clang__)) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

#undef AI_PUSHPACK_IS_DEFINED
