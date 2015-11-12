/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012-2015                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/font/font.h>

#include <ghoul/misc/assert.h>
#include <ghoul/logging/logmanager.h>

#include <algorithm>

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H

////Replacement for Visual Studio's _vscprintf function
//#if (_MSC_VER < 1400) //if older than visual studio 2005
//static int vscprintf (const char * format, va_list pargs)
//{
//    int retval;
//    va_list argcopy;
//    va_copy(argcopy, pargs);
//    retval = vsnprintf(NULL, 0, format, argcopy);
//    va_end(argcopy);
//    return retval;
//}
//#else
//#define vscprintf(f,a) _vscprintf(f,a)
//#endif


namespace {
    const std::string _loggerCat = "Font";
    const float PointConversionFactor = 64.f;  // Sizes in FT are given in 1/64th of pt
    const int DPI = 72;
}

namespace ghoul {
namespace fontrendering {
    
    
Font::Glyph::Glyph(wchar_t character,
                   size_t width,
                   size_t height,
                   int offsetX,
                   int offsetY,
                   float advanceX,
                   float advanceY,
                   glm::vec2 texCoordTopLeft,
                   glm::vec2 texCoordBottomRight,
                   glm::vec2 outlineTexCoordTopLeft,
                   glm::vec2 outlineTexCoordBottomRight
                   
                   )
    : _charcode(std::move(character))
    , _width(width)
    , _height(height)
    , _offsetX(offsetX)
    , _offsetY(offsetY)
    , _advanceX(advanceX)
    , _advanceY(advanceY)
    , _topLeft(std::move(texCoordTopLeft))
    , _bottomRight(std::move(texCoordBottomRight))
    , _outlineTopLeft(std::move(outlineTexCoordTopLeft))
    , _outlineBottomRight(std::move(outlineTexCoordBottomRight))
{
}
    
float Font::Glyph::kerning(wchar_t character) const {
    auto it = _kerning.find(character);
    if (it != _kerning.end())
        return it->second;
    else
        return 0.f;
}
    
int Font::Glyph::offsetX() const {
    return _offsetX;
}

int Font::Glyph::offsetY() const {
    return _offsetY;
}
    
size_t Font::Glyph::width() const {
    return _width;
}
    
size_t Font::Glyph::height() const {
    return _height;
}
   
Font::Font(std::string filename, float pointSize, opengl::TextureAtlas& atlas, bool outline, float outlineThickness)
    : _atlas(atlas)
    , _name(std::move(filename))
    , _pointSize(pointSize)
    , _height(0.f)
    , _outline(outline)
    , _outlineThickness(outlineThickness)
{
    ghoul_assert(_pointSize > 0.f, "Need positive point size");
    ghoul_assert(!_name.empty(), "Empty file name not allowed");
}
    
Font::~Font() {
        
}
    
bool Font::operator==(const Font& rhs) {
    return (
        (_name == rhs._name) &&
        (_pointSize == rhs._pointSize) &&
        (_glyphs == rhs._glyphs) &&
        (&_atlas == &rhs._atlas) &&
        (_outline = rhs._outline) &&
        (_outlineThickness == rhs._outlineThickness)
    );
}
    
bool Font::initialize() {
    // Get font metrics at high resolution for increased accuracy
    static const float HighFaceResolutionFactor = 100.f;
    FT_Library library;
    FT_Face face;

    bool success = loadFace(_pointSize * HighFaceResolutionFactor, library, face);
    if (!success)
        return false;
    
    
    FT_Size_Metrics metrics = face->size->metrics;
    _height = (metrics.height >> 6) / HighFaceResolutionFactor;
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    
    /* -1 is a special glyph */
    glyph(-1);
    
    return true;
}
    
Font::Glyph* Font::glyph(wchar_t character) {
    // Check if charcode has been already loaded
    for (size_t i = 0; i < _glyphs.size(); ++i) {
        Glyph* glyph = _glyphs[i];
        
        if (glyph->_charcode == character)
            return glyph;
    }
    
    // charcode -1 is special: it is used for line drawing (overline, underline,
    // strikethrough) and background.
    if (character == static_cast<wchar_t>(-1)) {
        size_t width = _atlas.size().x;
        size_t height = _atlas.size().y;
        glm::ivec4 region = _atlas.newRegion(5, 5);
        if (region.x < 0)
            return nullptr;

        // The last *4 for the depth is not a danger here as _atlas.setRegion only
        // extracts as much data as is needed for the used texture atlas
        std::array<unsigned char, 4*4*4> data;
        data.fill(std::numeric_limits<unsigned char>::max());
        
        _atlas.setRegionData(region.x, region.y, 4, 4, data.data());
        
        Glyph* glyph = new Glyph(static_cast<wchar_t>(-1));
        glyph->_topLeft = glm::vec2(
            (region.x+2)/static_cast<float>(width),
            (region.y+2)/static_cast<float>(height)
        );
        glyph->_bottomRight = glm::vec2(
            (region.x+3)/static_cast<float>(width),
            (region.y+3)/static_cast<float>(height)
        );

        _glyphs.push_back(glyph);

        return glyph;
    }
    
    // Glyph has not been already loaded
    size_t nGlyphNotLoaded = loadGlyphs({character});
    if (nGlyphNotLoaded == 0)
        return _glyphs.back();
    else {
        LERROR(nGlyphNotLoaded << " glyphs could not be loaded");
        return nullptr;
    }
}
    
std::string Font::name() const {
    return _name;
}
    
float Font::pointSize() const {
    return _pointSize;
}

float Font::height() const {
    return _height;
}
    
opengl::TextureAtlas& Font::atlas() {
    return _atlas;
}
    
bool Font::outline() const {
    return _outline;
}
    
size_t Font::loadGlyphs(const std::vector<wchar_t>& glyphs) {
    size_t missed = 0;
    
    unsigned int atlasWidth  = _atlas.size().x;
    unsigned int atlasHeight = _atlas.size().y;
    unsigned int atlasDepth  = _atlas.size().z;
    
    FT_Library library;
    FT_Face face;
    bool success = loadFace(_pointSize, library, face);
    if (!success)
        return glyphs.size();

    for (size_t i = 0; i < glyphs.size(); ++i) {
        
        // Search through the loaded glyphs to avoid duplicates
        wchar_t charcode = glyphs[i];
        auto it = std::find_if(
            _glyphs.begin(),
            _glyphs.end(),
            [charcode](Glyph* glyph) { return glyph->_charcode == charcode; }
        );
        if (it != _glyphs.end())
            continue;
        
        // First generate the font without outline and store it in the font atlas
        // only if an outline is request, repeat the process for the outline
        int ft_glyph_top, ft_glyph_left;
        glm::vec2 topLeft, bottomRight;
        glm::vec2 outlineTopLeft, outlineBottomRight;
        unsigned int w = 0, h = 0;
        
        FT_Int32 flags = 0;
        flags |= FT_LOAD_FORCE_AUTOHINT;
        
        FT_UInt glyphIndex = FT_Get_Char_Index(face, glyphs[i]);
        if (glyphIndex == 0) {
            LERROR("Glyph was not present in the FreeType face");
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return glyphs.size() - i;
        }
        
        if (_outline) {
            FT_Int32 outlineFlags = flags;
            //            flags |= FT_LOAD_NO_BITMAP;
            
            FT_Error error = FT_Load_Glyph(face, glyphIndex, outlineFlags);
            if (error) {
                LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
                FT_Done_Face(face);
                FT_Done_FreeType(library);
                return glyphs.size() - i;
            }
            
            
            FT_Stroker stroker;
            error = FT_Stroker_New(library, &stroker);
            if (error) {
                LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
                
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return glyphs.size() - i;
            }
            
            FT_Stroker_Set(stroker,
                           static_cast<int>(_outlineThickness * PointConversionFactor),
                           //                           static_cast<int>(OutlineThickness * HighResolution),
                           FT_STROKER_LINECAP_ROUND,
                           FT_STROKER_LINEJOIN_ROUND,
                           0);
            
            FT_Glyph outlineGlyph;
            error = FT_Get_Glyph(face->glyph, &outlineGlyph);
            if (error) {
                LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return glyphs.size() - i;
            }
            
            error = FT_Glyph_Stroke(&outlineGlyph, stroker, 1);
            //            error = FT_Glyph_StrokeBorder(&outlineGlyph, stroker, false, true);
            if (error) {
                LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return glyphs.size() - i;
            }
            
            error = FT_Glyph_To_Bitmap(&outlineGlyph, FT_RENDER_MODE_NORMAL, 0, 1);
            if (error) {
                LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
                FT_Done_Face(face);
                FT_Stroker_Done(stroker);
                FT_Done_FreeType(library);
                return 0;
            }
            FT_BitmapGlyph outlineBitmap;
            outlineBitmap = (FT_BitmapGlyph)outlineGlyph;
            
            ft_glyph_top    = outlineBitmap->top;
            ft_glyph_left   = outlineBitmap->left;
            FT_Stroker_Done(stroker);
            
            // We want each glyph to be separated by at least one black pixel
            // (for example for shader used in demo-subpixel.c)
            w = outlineBitmap->bitmap.width/atlasDepth;
            h = outlineBitmap->bitmap.rows;
            
            glm::ivec4 region = _atlas.newRegion(w + 1, h + 1);
            if (region.x < 0) {
                missed++;
                LERROR("Texture atlas is full");
                continue;
            }
            int x = region.x;
            int y = region.y;
            _atlas.setRegionData(x, y, w, h, outlineBitmap->bitmap.buffer);
            
            outlineTopLeft = glm::vec2(
                                       x/static_cast<float>(atlasWidth),
                                       y/static_cast<float>(atlasHeight)
                                       );
            outlineBottomRight = glm::vec2(
                                           (x + w)/static_cast<float>(atlasWidth),
                                           (y + h)/static_cast<float>(atlasHeight)
                                           );
        }
        
        FT_Error error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_FORCE_AUTOHINT);
        if (error) {
            LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return glyphs.size() - i;
        }
        
        FT_Glyph insideGlyph;
        error = FT_Get_Glyph(face->glyph, &insideGlyph);
        if (error) {
            LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return glyphs.size() - i;
        }
        
        error = FT_Glyph_To_Bitmap(&insideGlyph, FT_RENDER_MODE_NORMAL, nullptr, true);
        if (error) {
            LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return glyphs.size() - i;
        }
        
        
        FT_BitmapGlyph insideBitmap = (FT_BitmapGlyph)insideGlyph;
        
        ft_glyph_top = insideBitmap->top;
        ft_glyph_left = insideBitmap->left;
        
        w = std::max(w, insideBitmap->bitmap.width/atlasDepth);
        h = std::max(h, insideBitmap->bitmap.rows);
        
        
        glm::ivec4 region = _atlas.newRegion(w + 1, h + 1);
        if (region.x < 0) {
            missed++;
            LERROR("Texture atlas is full");
            continue;
        }
        int x = region.x;
        int y = region.y;
        
        if (_outline) {
            std::vector<unsigned char> buffer(w * h * sizeof(char), 0);
            int widthOffset = w - insideBitmap->bitmap.width;
            int heightOffset = h - insideBitmap->bitmap.rows;
            
            int k, l;
            for (unsigned int j = 0; j < h; ++j) {
                for (unsigned int i = 0; i < w; ++i) {
                    k = i - widthOffset;
                    l = j - heightOffset;
                    buffer[(i + j*w)] =
                    (k >= static_cast<int>(insideBitmap->bitmap.width) || l >= static_cast<int>(insideBitmap->bitmap.rows) || k < 0 || l < 0) ?
                    0 : insideBitmap->bitmap.buffer[k + insideBitmap->bitmap.width*l];
                    
                }
            }
            
            _atlas.setRegionData(x, y, w, h, buffer.data());
            
            x += widthOffset / 2.f;
            y += heightOffset / 2.f;
        }
        else {
            _atlas.setRegionData(x, y, w, h, insideBitmap->bitmap.buffer);
            
        }
        
        topLeft = glm::vec2(
                            x/static_cast<float>(atlasWidth),
                            y/static_cast<float>(atlasHeight)
                            );
        bottomRight = glm::vec2(
                                (x + w)/static_cast<float>(atlasWidth),
                                (y + h)/static_cast<float>(atlasHeight)
                                );
        

        
        
        // Discard hinting to get advance
        FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        
        Glyph* glyph = new Glyph(
            glyphs[i],
            w,
            h,
            ft_glyph_left,
            ft_glyph_top,
            face->glyph->advance.x / PointConversionFactor,
            face->glyph->advance.y / PointConversionFactor,
            topLeft,
            bottomRight,
            outlineTopLeft,
            outlineBottomRight
        );
        
        _glyphs.push_back(glyph);
    }
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    _atlas.upload();
    generateKerning();
    return missed;
}
    
void Font::generateKerning() {
    FT_Library library;
    FT_Face face;
    
    bool success = loadFace(_pointSize, library, face);
    if (!success)
        return;
    
    /* For each glyph couple combination, check if kerning is necessary */
    /* Starts at index 1 since 0 is for the special backgroudn glyph */
    for (size_t i = 1; i < _glyphs.size(); ++i) {
        Glyph* glyph = _glyphs[i];
        FT_UInt glyphIndex = FT_Get_Char_Index(face, glyph->_charcode);
        glyph->_kerning.clear();
        
        for (size_t j = 1; j < _glyphs.size(); ++j) {
            Glyph* prevGlyph = _glyphs[j];
            FT_UInt prevIndex = FT_Get_Char_Index(face, prevGlyph->_charcode);
            FT_Vector kerning;
            FT_Get_Kerning(face, prevIndex, glyphIndex, FT_KERNING_UNFITTED, &kerning);
            if (kerning.x != 0) {
                glyph->_kerning[prevGlyph->_charcode] = kerning.x / (PointConversionFactor*PointConversionFactor);
            }
        }
    }
    
    FT_Done_Face( face );
    FT_Done_FreeType( library );
}
    
bool Font::loadFace(float size, FT_Library& library, FT_Face& face) {
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
        return false;
    }

    // Load face
    error = FT_New_Face(library, _name.c_str(), 0, &face);
    if (error) {
        LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
        FT_Done_FreeType(library);
        return false;
    }
    
    // Select charmap
    error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (error) {
        LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    // Set char size
    error = FT_Set_Char_Size(face, (int)(size * PointConversionFactor), 0, DPI , DPI);
    if (error) {
        LERROR("FT_Error: " << FT_Errors[error].code << " (" << FT_Errors[error].message << ")");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }
    
    return true;
}

    
} // namespace fontrendering
} // namespace ghoul