#pragma once

// ShaderMacro
#define C_FIRST_RECURSION_STEP 0
#define C_CONTINUE_RECURSION 1

#ifdef __cplusplus
inline std::array<const char*, 64> getShaderMacros()
{
    std::array<const char*, 64> macros;
    macros[C_FIRST_RECURSION_STEP] = "FIRST_RECURSION_STEP";
    macros[C_CONTINUE_RECURSION] = "CONTINUE_RECURSION";
    return macros;
}
#endif