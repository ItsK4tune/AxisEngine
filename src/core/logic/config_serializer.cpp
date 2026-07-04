#include <core/logic/config_serializer.h>
#include <core/logic/config_loader.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/logger.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

static std::string FloatStr(float val)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6) << val;
    return ss.str();
}

static std::string FormatGravity(const float gravity[3])
{
    return FloatStr(gravity[0]) + " " + FloatStr(gravity[1]) + " " + FloatStr(gravity[2]);
}

bool ConfigSerializer::Deserialize(const std::string& filepath, AppConfig& config, bool headless)
{
    if (!std::filesystem::exists(filepath))
    {
        return false;
    }

    auto roots = YAMLParser::Parse(filepath);
    if (roots.empty())
    {
        return false;
    }

    std::vector<YAMLNode> activeRoots;
    if (roots.size() == 1 && roots[0].key == "axis_config")
    {
        activeRoots = roots[0].children;
    }
    else
    {
        bool found = false;
        for (const auto& root : roots)
        {
            if (root.key == "axis_config")
            {
                activeRoots = root.children;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    for (const auto& node : activeRoots)
    {
        std::stringstream ss;
        ss << node.key << " " << node.value;
        ConfigLoader::LoadConfig(ss, config, headless);
    }

    return true;
}

bool ConfigSerializer::Serialize(const std::string& filepath, const AppConfig& cfg)
{
    std::ofstream f(filepath);
    if (!f.is_open())
        return false;

    f << "axis_config:\n";

    auto SerialWriteKV = [](std::ofstream& out, int indent, const std::string& key, const std::string& value) {
        out << std::string(indent, ' ') << key << ": " << value << "\n";
    };

    SerialWriteKV(f, 4, "WINDOW_WIDTH", std::to_string(cfg.window.width));
    SerialWriteKV(f, 4, "WINDOW_HEIGHT", std::to_string(cfg.window.height));

    auto WindowModeToStr = [](WindowMode mode) {
        switch (mode)
        {
            case WindowMode::Fullscreen:
                return "FULLSCREEN";
            case WindowMode::Borderless:
                return "BORDERLESS";
            case WindowMode::BorderlessFullscreen:
                return "BORDERLESS_FULLSCREEN";
            default:
                return "WINDOWED";
        }
    };
    SerialWriteKV(f, 4, "WINDOW_MODE", WindowModeToStr(cfg.window.windowMode));
    SerialWriteKV(f, 4, "VSYNC", cfg.window.vsync ? "1" : "0");

    SerialWriteKV(f, 4, "MSAA", std::to_string(cfg.graphics.msaaSamples));
    SerialWriteKV(f, 4, "RENDER_SCALE", FloatStr(cfg.graphics.renderScale));
    SerialWriteKV(f, 4, "ASYNC_RESOURCES", cfg.graphics.asyncResourceLoading ? "1" : "0");
    SerialWriteKV(f, 4, "STRICT_ASSET_LOADING", cfg.graphics.strictAssetLoading ? "1" : "0");

    SerialWriteKV(f, 4, "HDR_ENABLED", cfg.render.hdrEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "BLOOM_ENABLED", cfg.render.bloomEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "GAMMA", FloatStr(cfg.render.gamma));
    SerialWriteKV(f, 4, "EXPOSURE", FloatStr(cfg.render.exposure));
    SerialWriteKV(f, 4, "SKYBOX_INTENSITY", FloatStr(cfg.render.skyboxIntensity));
    SerialWriteKV(f, 4, "AMBIENT_INTENSITY", FloatStr(cfg.render.ambientIntensity));
    SerialWriteKV(f, 4, "UI_REFERENCE_SIZE", FloatStr(cfg.render.uiReferenceWidth) + " " + FloatStr(cfg.render.uiReferenceHeight));

    auto TonemappingToStr = [](TonemappingMode mode) {
        switch (mode)
        {
            case TonemappingMode::ACES:
                return "ACES";
            case TonemappingMode::Reinhard:
                return "REINHARD";
            default:
                return "NONE";
        }
    };
    SerialWriteKV(f, 4, "TONEMAPPING", TonemappingToStr(cfg.render.tonemappingMode));
    SerialWriteKV(f, 4, "SHADOWS_ENABLED", cfg.shadow.shadowsEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "SHADOW_RESOLUTION", std::to_string(cfg.shadow.shadowMapResolution));
    SerialWriteKV(f, 4, "SHADOW_BIAS", FloatStr(cfg.shadow.shadowBias));
    SerialWriteKV(f, 4, "SHADOW_SOFTNESS", std::to_string(cfg.shadow.shadowSoftness));

    SerialWriteKV(f, 4, "VOLUME", FloatStr(cfg.audio.masterVolume));

    SerialWriteKV(f, 4, "GRAVITY", FormatGravity(cfg.physics.gravity));
    SerialWriteKV(f, 4, "MAX_SUBSTEPS", std::to_string(cfg.physics.maxSubSteps));
    SerialWriteKV(f, 4, "PHYSICS_TICKRATE", FloatStr(cfg.physics.physicsTickRate));

    auto LightingModeToStr = [](LightingMode mode) {
        switch (mode)
        {
            case LightingMode::Bake:
                return "BAKE";
            case LightingMode::LightProbe:
                return "LIGHT_PROBE";
            case LightingMode::ReflectionProbes:
                return "REFLECTION_PROBES";
            default:
                return "REAL_TIME";
        }
    };
    SerialWriteKV(f, 4, "LIGHTING_MODE", LightingModeToStr(cfg.lightingMode));

    return true;
}
