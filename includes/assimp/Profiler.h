


#pragma once
#ifndef AI_INCLUDED_PROFILER_H
#define AI_INCLUDED_PROFILER_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <chrono>
#include <assimp/DefaultLogger.hpp>
#include <assimp/TinyFormatter.h>

#include <map>

namespace Assimp {
namespace Profiling {

using namespace Formatter;



class Profiler {
public:
    Profiler() = default;


    
    void BeginRegion(const std::string& region) {
        regions[region] = std::chrono::system_clock::now();
        ASSIMP_LOG_DEBUG("START `",region,"`");
    }


    
    void EndRegion(const std::string& region) {
        RegionMap::const_iterator it = regions.find(region);
        if (it == regions.end()) {
            return;
        }

        std::chrono::duration<double> elapsedSeconds = std::chrono::system_clock::now() - regions[region];
        ASSIMP_LOG_DEBUG("END   `",region,"`, dt= ", elapsedSeconds.count()," s");
    }

private:
    typedef std::map<std::string,std::chrono::time_point<std::chrono::system_clock>> RegionMap;
    RegionMap regions;
};

}
}

#endif 

