/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012-2016                                                               *
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

#include <ghoul/opengl/texturemanager.h>

#include <ghoul/misc/crc32.h>
#include <ghoul/opengl/texture.h>

namespace ghoul {
namespace opengl {

TextureManager::TextureManagerError::TextureManagerError(std::string message)
    : RuntimeError(std::move(message), "TextureManager")
{}
    
TextureManager* TextureManager::_manager = nullptr;
    
void TextureManager::initialize() {
    ghoul_assert(_manager == nullptr, "Static manager must not have been initialized");
    _manager = new TextureManager;
}

void TextureManager::deinitialize() {
    ghoul_assert(_manager, "Static manager must have been intialized");
    delete _manager;
    _manager = nullptr;
}

TextureManager& TextureManager::ref() {
    ghoul_assert(_manager, "Static manager must have been initialized");
    return *_manager;
}

Texture* TextureManager::texture(unsigned int hashedName) {
    auto it = _textures.find(hashedName);
    if (it == _textures.end()) {
        throw TextureManagerError(
            "Could not find Texture for hash '" + std::to_string(hashedName) + "'"
        );
    }
    else
        return it->second.get();
}

Texture* TextureManager::texture(const std::string& name) {
    unsigned int hash = hashCRC32(name);
    try {
        return texture(hash);
    }
    catch (const TextureManagerError&) {
        throw TextureManagerError("Could not find Texture for '" + name + "'");
    }
}

unsigned int TextureManager::registerTexture(const std::string& name,
                                             std::unique_ptr<Texture> texture)
{
    unsigned int hashedName = hashCRC32(name);
    auto it = _textures.find(hashedName);
    if (it == _textures.end()) {
        _textures[hashedName] = std::move(texture);
        return hashedName;
    }
    else
        throw TextureManagerError("Name '" + name + "' was already registered");
}

std::unique_ptr<Texture> TextureManager::unregisterTexture(const std::string& name) {
    unsigned int hashedName = hashCRC32(name);
    return unregisterTexture(hashedName);
}

std::unique_ptr<Texture> TextureManager::unregisterTexture(unsigned int hashedName) {
    auto it = _textures.find(hashedName);
    if (it == _textures.end())
        return nullptr;
        
    auto tmp = std::move(it->second);
    _textures.erase(hashedName);
    return std::move(tmp);
}

unsigned int TextureManager::hashedNameForName(const std::string& name) const {
    return hashCRC32(name);
}

} // namespace opengl
} // namespace ghoul
