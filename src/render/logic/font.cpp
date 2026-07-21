#include <resource/unit/font.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

ITextureManager* Font::s_TextureManager = nullptr;

void Font::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

void Font::ClearTextureManager()
{
    s_TextureManager = nullptr;
}

ITextureManager& Font::GetTextureManager()
{
    if (!s_TextureManager)
    {
        LOGGER_ERROR("Font") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in Font");
    }
    return *s_TextureManager;
}

Font::Font() = default;

Font::~Font()
{
    if (s_TextureManager && m_AtlasTextureID != 0)
        s_TextureManager->DeleteTextures(1, &m_AtlasTextureID);
}

bool Font::Load(const std::string& fontPath, unsigned int fontSize)
{
    if (!s_TextureManager)
        return false;
    auto& tm = GetTextureManager();

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        LOGGER_ERROR("Font") << "Could not init FreeType Library";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
        LOGGER_ERROR("Font") << "Failed to load font: " << fontPath;
        FT_Done_FreeType(ft);
        return false;
    }

    m_FontSize = fontSize;
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    tm.PixelStorei(PixelStoreParam::UnpackAlignment, 1);

    if (m_AtlasTextureID != 0)
        tm.DeleteTextures(1, &m_AtlasTextureID);
    m_AtlasTextureID = 0;
    m_Characters.clear();

    struct GlyphBitmap
    {
        unsigned int codepoint = 0;
        int width = 0;
        int height = 0;
        int bearingX = 0;
        int bearingY = 0;
        unsigned int advance = 0;
        int atlasX = 0;
        int atlasY = 0;
        std::vector<unsigned char> pixels;
    };

    const std::array<std::pair<unsigned int, unsigned int>, 4> ranges = {
        {{0x0020, 0x007E}, {0x00A0, 0x00FF}, {0x0100, 0x017F}, {0x1E00, 0x1EFF}}};
    std::vector<GlyphBitmap> glyphs;
    size_t totalPixelArea = 0;
    int widestGlyph = 1;

    for (const auto& range : ranges)
    {
        for (unsigned int codepoint = range.first; codepoint <= range.second; ++codepoint)
        {
            if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER))
                continue;

            GlyphBitmap glyph;
            glyph.codepoint = codepoint;
            glyph.width = static_cast<int>(face->glyph->bitmap.width);
            glyph.height = static_cast<int>(face->glyph->bitmap.rows);
            glyph.bearingX = face->glyph->bitmap_left;
            glyph.bearingY = face->glyph->bitmap_top;
            glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);
            if (glyph.width > 0 && glyph.height > 0)
            {
                glyph.pixels.resize(static_cast<size_t>(glyph.width) * glyph.height);
                const int pitch = face->glyph->bitmap.pitch;
                for (int row = 0; row < glyph.height; ++row)
                {
                    const int sourceRow = pitch >= 0 ? row : glyph.height - 1 - row;
                    std::memcpy(glyph.pixels.data() + static_cast<size_t>(row) * glyph.width,
                                face->glyph->bitmap.buffer + static_cast<ptrdiff_t>(sourceRow) * std::abs(pitch),
                                glyph.width);
                }
                totalPixelArea += static_cast<size_t>(glyph.width + 2) * (glyph.height + 2);
                widestGlyph = (std::max)(widestGlyph, glyph.width);
            }
            glyphs.push_back(std::move(glyph));
        }
    }

    auto nextPowerOfTwo = [](int value) {
        int result = 1;
        while (result < value)
            result <<= 1;
        return result;
    };

    constexpr int padding = 1;
    int atlasWidth = nextPowerOfTwo((std::max)(512, static_cast<int>(std::sqrt(totalPixelArea * 1.35))));
    atlasWidth = (std::max)(atlasWidth, nextPowerOfTwo(widestGlyph + padding * 2));
    atlasWidth = (std::min)(atlasWidth, 16384);
    int requiredHeight = 0;
    for (;;)
    {
        int cursorX = padding;
        int cursorY = padding;
        int rowHeight = 0;
        for (auto& glyph : glyphs)
        {
            if (glyph.width == 0 || glyph.height == 0)
                continue;
            if (cursorX + glyph.width + padding > atlasWidth)
            {
                cursorX = padding;
                cursorY += rowHeight + padding;
                rowHeight = 0;
            }
            glyph.atlasX = cursorX;
            glyph.atlasY = cursorY;
            cursorX += glyph.width + padding;
            rowHeight = (std::max)(rowHeight, glyph.height);
        }
        requiredHeight = cursorY + rowHeight + padding;
        if (requiredHeight <= 16384 || atlasWidth >= 16384)
            break;
        atlasWidth *= 2;
    }

    if (requiredHeight > 16384)
    {
        LOGGER_ERROR("Font") << "Font atlas exceeds 16384 pixels: " << fontPath;
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return false;
    }

    const int atlasHeight = nextPowerOfTwo((std::max)(1, requiredHeight));
    std::vector<unsigned char> atlasPixels(static_cast<size_t>(atlasWidth) * atlasHeight, 0);
    for (const auto& glyph : glyphs)
    {
        if (glyph.pixels.empty())
            continue;
        for (int row = 0; row < glyph.height; ++row)
        {
            std::memcpy(atlasPixels.data() + static_cast<size_t>(glyph.atlasY + row) * atlasWidth + glyph.atlasX,
                        glyph.pixels.data() + static_cast<size_t>(row) * glyph.width, glyph.width);
        }
    }

    m_AtlasTextureID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_AtlasTextureID);
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::R8, atlasWidth, atlasHeight, 0, TextureFormat::Red,
                  DataType::UnsignedByte, atlasPixels.data());

    for (const auto& glyph : glyphs)
    {
        Character character;
        character.textureID = m_AtlasTextureID;
        character.size = {glyph.width, glyph.height};
        character.bearing = {glyph.bearingX, glyph.bearingY};
        character.advance = glyph.advance;
        character.uvMin = {static_cast<float>(glyph.atlasX) / atlasWidth,
                           static_cast<float>(glyph.atlasY) / atlasHeight};
        character.uvMax = {static_cast<float>(glyph.atlasX + glyph.width) / atlasWidth,
                           static_cast<float>(glyph.atlasY + glyph.height) / atlasHeight};
        m_Characters.emplace(glyph.codepoint, character);
    }

    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
    tm.BindTexture(TextureType::Texture2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return !m_Characters.empty();
}

const Character& Font::GetCharacter(unsigned int codepoint) const
{
    auto it = m_Characters.find(codepoint);
    if (it != m_Characters.end())
        return it->second;
    if (const auto fallback = m_Characters.find('?'); fallback != m_Characters.end())
        return fallback->second;
    return m_Characters.at(' ');
}
