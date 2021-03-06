cmake_minimum_required(VERSION 3.0)

project(RayTracingProject)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
add_definitions(-DNOMINMAX)

link_directories("extern/glfw/lib/win64/")
link_directories("extern/V-EZ/")
link_directories("extern/FreeImage/")

#Core
file(GLOB_RECURSE CORE_HDRS src/timCore/*.h)
#file(GLOB_RECURSE CORE_HDRS src/timCore/*.hpp)
file(GLOB_RECURSE CORE_SRCS src/timCore/*.cpp)
ADD_LIBRARY(timCore ${CORE_SRCS} ${CORE_HDRS})
target_include_directories(timCore PRIVATE "src/")
set_property(TARGET timCore PROPERTY CXX_STANDARD 17)

#ShaderCompiler
file(GLOB_RECURSE SC_HDRS src/ShaderCompiler/*.h)
file(GLOB_RECURSE SC_SRCS src/ShaderCompiler/*.cpp)
ADD_LIBRARY(shaderCompiler ${SC_SRCS} ${SC_HDRS})
target_include_directories(shaderCompiler PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(shaderCompiler PRIVATE "src/")
target_link_libraries(shaderCompiler timCore)
set_property(TARGET shaderCompiler PROPERTY CXX_STANDARD 17)

#Shaders
file(GLOB_RECURSE SC_FXH src/Shaders/*.glsl)
file(GLOB_RECURSE SC_COMP src/Shaders/*.comp)
ADD_LIBRARY(shaders src/dummy.cpp ${SC_FXH} ${SC_COMP})

#VezRenderer
find_package(Vulkan REQUIRED)
file(GLOB_RECURSE VEZRENDERER_HDRS src/rtDevice/*.h)
#file(GLOB_RECURSE VEZRENDERER_HDRS src/rtDevice/*.hpp)
file(GLOB_RECURSE VEZRENDERER_SRCS src/rtDevice/*.cpp)

ADD_LIBRARY(VezRenderer ${VEZRENDERER_SRCS} ${VEZRENDERER_HDRS})
target_include_directories(VezRenderer PRIVATE "extern/V-EZ")
target_include_directories(VezRenderer PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(VezRenderer PRIVATE "src/")
target_link_libraries(VezRenderer timCore shaderCompiler ${Vulkan_LIBRARY})
target_link_libraries(VezRenderer debug VEZd)
target_link_libraries(VezRenderer optimized VEZ)
set_property(TARGET VezRenderer PROPERTY CXX_STANDARD 17)

# Main exe
file(GLOB RENDERER_HDRS src/*.h)
file(GLOB RENDERER_SRCS src/*.cpp)
file(GLOB_RECURSE MAIN_HDRS src/Renderer/*.cpp src/Renderer/*.hpp)
file(GLOB_RECURSE MAIN_SRCS src/Renderer/*.h)

ADD_EXECUTABLE(Program ${MAIN_SRCS} ${RENDERER_HDRS} ${RENDERER_SRCS} ${MAIN_HDRS})
target_include_directories(Program PRIVATE "extern/glfw/include")
target_include_directories(Program PRIVATE "src/")
target_include_directories(Program PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(Program PRIVATE "extern/FreeImage")
target_link_libraries(Program FreeImage glfw VezRenderer timCore)
set_property(TARGET Program PROPERTY CXX_STANDARD 17)
