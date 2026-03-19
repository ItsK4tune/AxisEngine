



#pragma once
#ifndef INCLUDED_AI_BASEIMPORTER_H
#define INCLUDED_AI_BASEIMPORTER_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include "Exceptional.h"

#include <assimp/types.h>
#include <assimp/ProgressHandler.hpp>
#include <exception>
#include <set>
#include <vector>
#include <memory>

struct aiScene;
struct aiImporterDesc;

namespace Assimp {


class Importer;
class IOSystem;
class BaseProcess;
class SharedPostProcessInfo;
class IOStream;


#define AI_MAKE_MAGIC(string) ((uint32_t)((string[0] << 24) + \
                                          (string[1] << 16) + (string[2] << 8) + string[3]))

using UByteBuffer = std::vector<uint8_t>;
using ByteBuffer = std::vector<int8_t>;



class ASSIMP_API BaseImporter {
    friend class Importer;

public:
    
    BaseImporter() AI_NO_EXCEPT;

    
    virtual ~BaseImporter() = default;

    
    
    virtual bool CanRead(
            const std::string &pFile,
            IOSystem *pIOHandler,
            bool checkSig) const = 0;

    
    
    aiScene *ReadFile(
            Importer *pImp,
            const std::string &pFile,
            IOSystem *pIOHandler);

    
    
    const std::string &GetErrorText() const {
        return m_ErrorText;
    }

    
    
    const std::exception_ptr& GetException() const {
        return m_Exception;
    }

    
    
    virtual void SetupProperties(
            const Importer *pImp);

    
    
    virtual const aiImporterDesc *GetInfo() const = 0;

    
    void SetFileScale(double scale) {
        fileScale = scale;
    }

    
    
    void GetExtensionList(std::set<std::string> &extensions);

protected:
    double importerScale = 1.0;
    double fileScale = 1.0;

    
    
    virtual void InternReadFile(
            const std::string &pFile,
            aiScene *pScene,
            IOSystem *pIOHandler) = 0;

public: 
    
    
    static bool SearchFileHeaderForToken(
            IOSystem *pIOSystem,
            const std::string &file,
            const char **tokens,
            std::size_t numTokens,
            unsigned int searchBytes = 200,
            bool tokensSol = false,
            bool noGraphBeforeTokens = false);

    
    
    static bool SimpleExtensionCheck(
            const std::string &pFile,
            const char *ext0,
            const char *ext1 = nullptr,
            const char *ext2 = nullptr,
            const char *ext3 = nullptr);

    
    
    static bool HasExtension(
            const std::string &pFile,
            const std::set<std::string> &extensions);

    
    
    static std::string GetExtension(
            const std::string &pFile);

    
    
    static bool CheckMagicToken(
            IOSystem *pIOHandler,
            const std::string &pFile,
            const void *magic,
            std::size_t num,
            unsigned int offset = 0,
            unsigned int size = 4);

    
    
    static void ConvertToUTF8(
            std::vector<char> &data);

    
    
    static void ConvertUTF8toISO8859_1(
            std::string &data);

    
    
    enum TextFileMode {
        ALLOW_EMPTY,
        FORBID_EMPTY
    };

    
    
    static void TextFileToBuffer(
            IOStream *stream,
            std::vector<char> &data,
            TextFileMode mode = FORBID_EMPTY);

    
    
    template <typename T>
    AI_FORCE_INLINE static void CopyVector(
            std::vector<T> &vec,
            T *&out,
            unsigned int &outLength) {
        outLength = unsigned(vec.size());
        if (outLength) {
            out = new T[outLength];
            std::swap_ranges(vec.begin(), vec.end(), out);
        }
    }

    
    
    template <typename T>
    AI_FORCE_INLINE static void CopyVector(
            std::vector<std::unique_ptr<T> > &vec,
            T **&out,
            unsigned int &outLength) {
        outLength = unsigned(vec.size());
        if (outLength) {
            out = new T*[outLength];
            T** outPtr = out;
            std::for_each(vec.begin(), vec.end(), [&outPtr](std::unique_ptr<T>& uPtr){*outPtr = uPtr.release(); ++outPtr; });
        }
    }

private:
    
    void UpdateImporterScale(Importer *pImp);

protected:
    
    std::string m_ErrorText;
    
    std::exception_ptr m_Exception;
    
    ProgressHandler *m_progress;
};

} 

#endif 
