

#pragma once
#ifndef AI_BASE64_HPP_INC
#define AI_BASE64_HPP_INC

#include <assimp/defs.h>

#include <stdint.h>
#include <vector>
#include <string>

namespace Assimp {
namespace Base64 {





ASSIMP_API void Encode(const uint8_t *in, size_t inLength, std::string &out);




ASSIMP_API void Encode(const std::vector<uint8_t> &in, std::string &out);




ASSIMP_API std::string Encode(const std::vector<uint8_t> &in);






ASSIMP_API size_t Decode(const char *in, size_t inLength, uint8_t *&out);





ASSIMP_API size_t Decode(const std::string &in, std::vector<uint8_t> &out);




ASSIMP_API std::vector<uint8_t> Decode(const std::string &in);

} 
} 

#endif 
