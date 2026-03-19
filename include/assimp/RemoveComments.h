


#pragma once
#ifndef AI_REMOVE_COMMENTS_H_INC
#define AI_REMOVE_COMMENTS_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/defs.h>

namespace Assimp {



class ASSIMP_API CommentRemover {
    
    CommentRemover() {}

public:

    
    
    
    
    
    
    static void RemoveLineComments(const char* szComment,
        char* szBuffer, char chReplacement = ' ');

    
    
    
    
    
    
    
    
    static void RemoveMultiLineComments(const char* szCommentStart,
        const char* szCommentEnd,char* szBuffer,
        char chReplacement = ' ');
};
} 

#endif 
