// TextureLoader.cpp
#include "TextureLoader.h"
#include <stdexcept>

TextureLoader::TextureLoader(const std::string& filePath)
{
    // Flip the image vertically (optional, depending on your texture requirements)
    stbi_set_flip_vertically_on_load(1);

    // Load image data
    m_data = stbi_load(filePath.c_str(), &m_width, &m_height, &m_channels, 4); // Force 4 channels (RGBA)
    if (!m_data)
    {
        throw std::runtime_error("Failed to load image: " + filePath);
    }
}

TextureLoader::~TextureLoader()
{
    if (m_data)
    {
        stbi_image_free(m_data);
    }
}