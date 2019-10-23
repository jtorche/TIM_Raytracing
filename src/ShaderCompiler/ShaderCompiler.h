#include "timCore/type.h"
#include "timCore/hash.h"
#include "timCore/flat_hash_map.h"
#include "ShaderFlags.h"
#include <array>

namespace tim
{
    using Blob = ::std::vector<ubyte>;

    class ShaderCompiler
    {
    public:
        ShaderCompiler(const char * _folder, const std::array<const char*, 64>& _macros);
        ~ShaderCompiler() = default;

        void clearCache();

        const Blob& getShaderMicrocode(FxNameHash, ShaderFlags);

    private:
        std::string buildDefineCommand(ShaderFlags) const;
        std::string buildShaderKeyString(FxNameHash, ShaderFlags) const;

    private:
        std::string m_folder;
        std::array<const char*, 64> m_macros = { { nullptr } };
        
        struct FXFile
        {
            std::string m_fxName;
            ska::flat_hash_map<u64, Blob> m_bytecode;
        };
        ska::flat_hash_map<FxNameHash, FXFile> m_shaders;
    };
}