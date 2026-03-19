



  


#ifndef FTHASH_H_
#define FTHASH_H_


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  typedef union  FT_Hashkey_
  {
    FT_Int       num;
    const char*  str;

  } FT_Hashkey;


  typedef struct  FT_HashnodeRec_
  {
    FT_Hashkey  key;
    size_t      data;

  } FT_HashnodeRec;

  typedef struct FT_HashnodeRec_  *FT_Hashnode;


  typedef FT_ULong
  (*FT_Hash_LookupFunc)( FT_Hashkey*  key );

  typedef FT_Bool
  (*FT_Hash_CompareFunc)( FT_Hashkey*  a,
                          FT_Hashkey*  b );


  typedef struct  FT_HashRec_
  {
    FT_UInt  limit;
    FT_UInt  size;
    FT_UInt  used;

    FT_Hash_LookupFunc   lookup;
    FT_Hash_CompareFunc  compare;

    FT_Hashnode*  table;

  } FT_HashRec;

  typedef struct FT_HashRec_  *FT_Hash;


  FT_Error
  ft_hash_str_init( FT_Hash    hash,
                    FT_Memory  memory );

  FT_Error
  ft_hash_num_init( FT_Hash    hash,
                    FT_Memory  memory );

  void
  ft_hash_str_free( FT_Hash    hash,
                    FT_Memory  memory );

#define ft_hash_num_free  ft_hash_str_free

  FT_Error
  ft_hash_str_insert( const char*  key,
                      size_t       data,
                      FT_Hash      hash,
                      FT_Memory    memory );

  FT_Error
  ft_hash_num_insert( FT_Int     num,
                      size_t     data,
                      FT_Hash    hash,
                      FT_Memory  memory );

  size_t*
  ft_hash_str_lookup( const char*  key,
                      FT_Hash      hash );

  size_t*
  ft_hash_num_lookup( FT_Int   num,
                      FT_Hash  hash );


FT_END_HEADER


#endif 



