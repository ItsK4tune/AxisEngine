#include <resource/unit/font.h>
#include <core/logic/logger.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>

ITextureManager* Font::s_TextureManager = nullptr;

void Font::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

ITextureManager& Font::GetTextureManager()
{
    if (!s_TextureManager) {
        LOGGER_ERROR("Font") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in Font");
    }
    return *s_TextureManager;
}

Font::Font() {}

Font::~Font() {
    if (s_TextureManager) {
        for (auto const& [key, ch] : Characters) {
            s_TextureManager->DeleteTextures(1, &ch.TextureID);
        }
    }
}

bool Font::Load(const std::string& fontPath, unsigned int fontSize) {
    if (!s_TextureManager) return false;
    auto& tm = GetTextureManager();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        LOGGER_ERROR("Font") << "Could not init FreeType Library";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        LOGGER_ERROR("Font") << "Failed to load font: " << fontPath;
        FT_Done_FreeType(ft);
        return false;
    }

    m_FontSize = fontSize;
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    tm.PixelStorei(PixelStoreParam::UnpackAlignment, 1);

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            LOGGER_ERROR("Font") << "Failed to load Glyph";
            continue;
        }

        unsigned int Texture = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, Texture);
        tm.TexImage2D(
            TextureType::Texture2D,
            0,
            InternalFormat::R8,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            TextureFormat::Red,
            DataType::UnsignedByte,
            face->glyph->bitmap.buffer
        );

        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

        Character character = {
            Texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return true;
}

const Character& Font::GetCharacter(char c) const {
    return Characters.at(c);
}