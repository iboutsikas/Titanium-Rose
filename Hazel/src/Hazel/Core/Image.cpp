#include "hzpch.h"
#include "Hazel/Core/Image.h"

#include "stb_image.h"

namespace Hazel
{
    Image::Image() :
        m_Width(0), m_Height(0), m_Channels(0), m_IsHdr(false)
    {

    }

    Ref<Image> Image::FromFile(std::string& filepath, int desiredChannels)
    {
        Ref<Image> ret { new Image() };
        int width = 0, height = 0, channels = 0;

        if (stbi_is_hdr(filepath.c_str()))
        {
            float* pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, desiredChannels);

            if (pixels != nullptr) {

                // This is a weird hackaround cuz we need RGBE hdrs but stbi won't load them
                if (channels == 3) {
                    channels = 4;
                    size_t newSize = width * height * 4 * sizeof(float);
                    float* newPixels = (float*)malloc(newSize);
                    
                    for (size_t h = 0; h < height; h++)
                    {
                        for (size_t w = 0; w < width; w++)
                        {
                            auto index = h * width + w;
                            newPixels[index * 4 + 0] = pixels[index * 3 + 0];
                            newPixels[index * 4 + 1] = pixels[index * 3 + 1];
                            newPixels[index * 4 + 2] = pixels[index * 3 + 2];
                            newPixels[index * 4 + 3] = 1.0f;
                        }
                    }
                    free(pixels);
                    ret->m_Pixels.reset(reinterpret_cast<uint8_t*>(newPixels));

                }
                else
                {
                    ret->m_Pixels.reset(reinterpret_cast<uint8_t*>(pixels));
                }

                ret->m_IsHdr = true;
            }
        }
        else
        {
            uint8_t* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, desiredChannels);
            if (pixels != nullptr)
            {
                ret->m_Pixels.reset(pixels);
                ret->m_IsHdr = false;
            }
        }

        if (ret->m_Pixels != nullptr)
        {
            ret->m_Width = width;
            ret->m_Height = height;
            /*
                We have only loaded desiredChannels in this case. Which can very well be < channels.
            */
            ret->m_Channels = desiredChannels > 0 ? desiredChannels : channels;
        }

        return ret;
    }
    uint32_t Image::BytesPerPixel() const
    {
        return m_Channels * (m_IsHdr ? sizeof(float) : sizeof(uint8_t));
    }
}

