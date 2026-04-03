#pragma once

class IIBLService
{
public:
    virtual ~IIBLService() = default;

    virtual unsigned int GetIrradianceMap() const = 0;
    virtual unsigned int GetPrefilterMap() const = 0;
    virtual unsigned int GetBrdfLUT() const = 0;
};
