#pragma once

// ShaderMacro
#define C_FIRST_RECURSION_STEP 0
#define C_CONTINUE_RECURSION 1
#define C_USE_TRAVERSE_TLAS 2
#define C_TRACING_STEP 3
#define C_USE_LPF 4
#define C_USE_LPF_MASK 5

#ifdef __cplusplus
inline std::array<const char*, 64> getShaderMacros()
{
    std::array<const char*, 64> macros;
    macros[C_FIRST_RECURSION_STEP] = "FIRST_RECURSION_STEP";
    macros[C_CONTINUE_RECURSION] = "CONTINUE_RECURSION";
    macros[C_USE_TRAVERSE_TLAS] = "USE_TRAVERSE_TLAS";
    macros[C_TRACING_STEP] = "TRACING_STEP";
    macros[C_USE_LPF] = "USE_LPF";
    macros[C_USE_LPF_MASK] = "USE_LPF_MASK";
    return macros;
}
#endif