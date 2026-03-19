


#pragma once
#ifndef AI_FILEIO_H_INC
#define AI_FILEIO_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aiFileIO;
struct aiFile;


typedef size_t          (*aiFileWriteProc) (C_STRUCT aiFile*,   const char*, size_t, size_t);
typedef size_t          (*aiFileReadProc)  (C_STRUCT aiFile*,   char*, size_t,size_t);
typedef size_t          (*aiFileTellProc)  (C_STRUCT aiFile*);
typedef void            (*aiFileFlushProc) (C_STRUCT aiFile*);
typedef C_ENUM aiReturn (*aiFileSeek)      (C_STRUCT aiFile*, size_t, C_ENUM aiOrigin);


typedef C_STRUCT aiFile* (*aiFileOpenProc)  (C_STRUCT aiFileIO*, const char*, const char*);
typedef void             (*aiFileCloseProc) (C_STRUCT aiFileIO*, C_STRUCT aiFile*);


typedef char* aiUserData;



struct aiFileIO
{
    
    aiFileOpenProc OpenProc;

    
    aiFileCloseProc CloseProc;

    
    aiUserData UserData;
};



struct aiFile {
    
    aiFileReadProc ReadProc;

    
    aiFileWriteProc WriteProc;

    
    aiFileTellProc TellProc;

    
    aiFileTellProc FileSizeProc;

    
    aiFileSeek SeekProc;

    
    aiFileFlushProc FlushProc;

    
    aiUserData UserData;
};

#ifdef __cplusplus
}
#endif
#endif 
