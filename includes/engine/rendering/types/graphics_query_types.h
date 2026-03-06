#pragma once

namespace Graphics {

    enum class QueryType {
        SamplesPassed,
        AnySamplesPassed,
        AnySamplesPassedConservative,
        TimeElapsed,
        Timestamp
    };
}
