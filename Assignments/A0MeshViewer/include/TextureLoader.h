// TextureLoader.h
#ifndef TEXTURE_LOADER_CLASS
#define TEXTURE_LOADER_CLASS

#include <gimslib/contrib/stb/stb_image.h>
#include <string>
#include <cstdint>

class TextureLoader
{
public:
    // Constructor to load the image
    TextureLoader(const std::string& filePath);

    // Destructor to free the image data
    ~TextureLoader();

    // Getters for image properties
    uint8_t* getData() const { return m_data; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }

private:
    uint8_t* m_data = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};

#endif //TEXTURE_LOADER_CLASS