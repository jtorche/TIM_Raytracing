#pragma once

// ShaderMacro
#define C_CONTINUE_RECURSION 0

#ifdef __cplusplus
inline std::array<const char*, 64> getShaderMacros()
{
    std::array<const char*, 64> macros;
    macros[C_CONTINUE_RECURSION] = "CONTINUE_RECURSION";
    return macros;
}
#endif