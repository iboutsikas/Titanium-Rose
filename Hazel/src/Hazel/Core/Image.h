#pragma once

#include "Hazel/Core/Core.h"

namespace Hazel
{
    class Image
    {
    public:

        /// <summary>
        /// Will attempt to load the contents of the file into an Image. If channels is 0, it will load all 
        /// of the channels in the image
        /// </summary>
        /// <param name="filepath">The filepath to the image file</param>
        /// <param name="desiredChannels">How many channels to load out of the file.</param>
        /// <returns></returns>
        static Ref<Image> FromFile(std::string& filepath, int desiredChannels = 0);

        inline uint32_t GetWidth()      const { return m_Width; }
        inline uint32_t GetHeight()     const { return m_Height; }
        inline uint32_t GetChannels()   const { return m_Channels; }
        inline bool     IsHdr()         const { return m_IsHdr; }

        uint32_t BytesPerPixel() const;

        template<typename T>
        const T* Bytes() const
        {
            return reinterpret_cast<const T*>(m_Pixels.get());
        }

    private:
        Image();

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Channels;
        bool m_IsHdr;

        Scope<uint8_t> m_Pixels;
    };

}

