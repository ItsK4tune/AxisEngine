#include <axis_plugin.h>
#include <axis_sdk.h>

int main()
{
    AppBuilder providers;
    ISystemRegistry* systems = nullptr;
    IComponentCodecRegistry* codecs = nullptr;
    ILocalizationService* localization = nullptr;
    return providers.GetCapabilities().customWindow || systems || codecs || localization ? 1 : 0;
}
