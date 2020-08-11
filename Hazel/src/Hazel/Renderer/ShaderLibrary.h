#pragma once

#include "Hazel/Renderer/Shader.h"
#include <map>
namespace Hazel
{
    class ShaderLibrary
    {
    public:
        ShaderLibrary(uint32_t frameLatency = 3);

        void Add(const std::string& name, const Ref<Shader>& shader);
        void Add(const Ref<Shader>& shader);
        Ref<Shader> Load(const std::string& filepath);
        Ref<Shader> Load(const std::string& name, const std::string& filepath);

        Ref<Shader> Get(const std::string& name);

        template<typename T,
            typename = std::enable_if_t<std::is_base_of_v<Hazel::Shader, T>>>
            Ref<T> GetAs(const std::string& name)
        {
            return std::dynamic_pointer_cast<T>(Get(name));
        }


        bool Exists(const std::string& name) const;
        void Update();

        std::map<std::string, Ref<Shader>>::iterator begin() { return m_Shaders.begin(); }
        std::map<std::string, Ref<Shader>>::iterator end() { return m_Shaders.end(); }

        std::map<std::string, Ref<Shader>>::const_iterator begin() const { return m_Shaders.begin(); }
        std::map<std::string, Ref<Shader>>::const_iterator end()	const { return m_Shaders.end(); }

    private:
        std::map<std::string, Ref<Shader>> m_Shaders;
        uint32_t m_FrameLatency;
        uint32_t m_FrameCount;
    };

}


