#include "ShaderCompiler.h"
#include "timCore/hash.h"
#include "timCore/Common.h"
#include <filesystem>
#include <iostream>
#include <windows.h>

namespace tim
{
    ShaderCompiler::ShaderCompiler(const char* _folder, const std::array<const char*, 64>& _macros) : m_folder{ _folder }, m_macros { _macros }
    {
        namespace fs = std::filesystem;

        fs::create_directory(m_folder + "cache");

        for (const auto& entry : fs::directory_iterator(_folder))
        {
            if (entry.is_regular_file() && entry.path().has_extension())
            {
                if (entry.path().extension() == ".comp")
                {
                    std::cout << "FX: "<< entry.path().filename() << "\n";
                    m_shaders[TIM_HASH32_STR(entry.path().filename().c_str())] = { entry.path().u8string() };
                }
            }
        }
    }

    void ShaderCompiler::clearCache()
    {
        for (auto& fxFile : m_shaders)
            fxFile.second.m_bytecode.clear();
    }

    const Blob& ShaderCompiler::getShaderMicrocode(FxNameHash _fx, ShaderFlags _flags)
    {
        namespace fs = std::filesystem;

        auto itFx = m_shaders.find(_fx);
        TIM_ASSERT(itFx != m_shaders.end());

        auto it = itFx->second.m_bytecode.find(_flags.m_flags);
        if (it != itFx->second.m_bytecode.end())
            return it->second;
        else
        {
            std::string outputFile = m_folder + "cache/" + buildShaderKeyString(_fx, _flags);
            std::string cmdArg = itFx->second.m_fxName + " " + buildDefineCommand(_flags) + " -O -o " + outputFile;

            fs::remove(outputFile);

            std::cout << "\nCompiling " << itFx->second.m_fxName << ".\n";

            bool retry = false;
            do
            {
                system(("C:\\VulkanSDK\\1.2.131.2\\Bin\\glslc.exe " + cmdArg).c_str());
                if (!fs::exists(outputFile))
                {
                    std::cout << "\nCompilation failed, would you retry ?\n";
                    std::cin >> retry;
                    std::cin.clear();
                }
                else
                    retry = false;

            } while (retry);

            TIM_ASSERT(fs::exists(outputFile));

            u64 size = fs::file_size(outputFile);
            Blob fileContent(size);

            FILE * filp = fopen(outputFile.c_str(), "rb");
            TIM_ASSERT(filp != nullptr);
            size_t bytes_read = fread(fileContent.data(), sizeof(char), size, filp);
            TIM_ASSERT(size == bytes_read);
            fclose(filp); 

            itFx->second.m_bytecode.emplace(_flags.m_flags, std::move(fileContent));

            return itFx->second.m_bytecode.at(_flags.m_flags);
        }
    }

    std::string ShaderCompiler::buildDefineCommand(ShaderFlags _flags) const
    {
        std::string res;
        for (u32 i = 0; i < ShaderFlags::MaxFlags; ++i)
        {
            if (_flags.test(i) && m_macros[i])
                res += std::string("-D") + m_macros[i];
        }

        return res;
    }

    std::string ShaderCompiler::buildShaderKeyString(FxNameHash _fx, ShaderFlags _flags) const
    {
        char buffer[64];
        sprintf_s(buffer, "%u_%llu.bin", _fx, _flags.m_flags);
        return std::string(buffer);
    }
}