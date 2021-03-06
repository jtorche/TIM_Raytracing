#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Renderer/raytracingPass.h"
#include "Renderer/postprocessPass.h"
#include "Renderer/SimpleCamera.h"
#include "Renderer/shaderMacros.h"
#include "Renderer/TextureManager.h"

#include <iostream>

using namespace tim;

IRenderer * g_renderer = nullptr;
SimpleCamera camera;
uvec2 frameResolution = { 640, 420 };
bool g_rebuildBvh = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    static bool forward = false;
    static bool backward = false;
    static bool left = false;
    static bool right = false;

    if (key == GLFW_KEY_F10 && action == GLFW_PRESS)
        g_renderer->InvalidateShaders();

    if (key == GLFW_KEY_W)
        forward = action == GLFW_PRESS || action == GLFW_REPEAT;
    if (key == GLFW_KEY_S)
        backward = action == GLFW_PRESS || action == GLFW_REPEAT;

    if (key == GLFW_KEY_A)
        left = action == GLFW_PRESS || action == GLFW_REPEAT;
    if (key == GLFW_KEY_D)
        right = action == GLFW_PRESS || action == GLFW_REPEAT;

    camera.setDirection(left, right, forward, backward);

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        g_rebuildBvh = true;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    static bool leftButton = false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        leftButton = true;
    else
        leftButton = false;

    camera.setMouseButton(leftButton);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double prevX = 0;
    static double prevY = 0;

    camera.setMouseDelta(float(prevX - xpos), float(prevY - ypos));

    prevX = xpos;
    prevY = ypos;
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    static int s_width = 0;
    static int s_height = 0;

    if (g_renderer && (s_width != width || s_height != height))
    {
        s_width = width;
        s_height = height;
        frameResolution = uvec2{ u32(width), u32(height) };
        g_renderer->Resize(s_width, s_height);
    }
}

int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(frameResolution.x, frameResolution.y, "RayTracing", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

    HWND winHandle = glfwGetWin32Window(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    g_renderer = createRenderer();
    ResourceAllocator resourceAllocator(g_renderer);

    ShaderCompiler shaderCompiler("../src/Shaders/", getShaderMacros());
    g_renderer->Init(shaderCompiler , &winHandle, frameResolution.x, frameResolution.y, false);

    IRenderContext* context = g_renderer->CreateRenderContext(RenderContextType::Graphics);
    
    {
        TextureManager textureManager(g_renderer);
        textureManager.loadTexture("../data/image/ibl_brdf_lut.png");
        RayTracingPass rtPass(g_renderer, context, resourceAllocator, textureManager);
        PostprocessPass postprocessPass(g_renderer, context);

        double prevTime = glfwGetTime();
        double frameTime = 0.01;
        u32 frameIndex = 0;
        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Poll for and process events */
            glfwPollEvents();
            camera.update(float(frameTime));

            if (g_rebuildBvh)
            {
                g_rebuildBvh = false;
                u32 maxDepth = 5; u32 maxObjPerNode = 8;
                std::cout << "Rebuild bvh, max depth : ";
                std::cin >> maxDepth;
                std::cout << "Rebuild bvh, max obj per node : ";
                std::cin >> maxObjPerNode;

                rtPass.rebuildBvh(maxDepth, maxObjPerNode);
            }

            g_renderer->BeginFrame();
            ImageHandle backbuffer = g_renderer->GetBackBuffer();

            context->BeginRender();
            context->ClearImage(backbuffer, Color{ 0, 0, 0, 0 });

            postprocessPass.setFrameBufferSize(frameResolution);
            rtPass.setFrameBufferSize(frameResolution);

            ImageCreateInfo imgInfo(ImageFormat::RGBA16F, frameResolution.x, frameResolution.y, 1, ImageType::Image2D, MemoryType::Default);
            ImageHandle outputColorBuffer = resourceAllocator.allocTexture(imgInfo);
            rtPass.draw(outputColorBuffer, camera);

            postprocessPass.linearToSrgb(outputColorBuffer, backbuffer);
            resourceAllocator.releaseTexture(outputColorBuffer);

            context->EndRender();

            g_renderer->Execute(context);

            g_renderer->EndFrame();
            g_renderer->Present();

            if (frameIndex % 100 == 0)
                std::cout << "FPS : " << u32(0.5f + 1.f / float(frameTime)) << std::endl;

            double curTime = glfwGetTime();
            frameTime = curTime - prevTime;
            prevTime = curTime;
            frameIndex++;

            //double sleepTime = std::max<double>(0, (1.0 / 60) - frameTime);
            //Sleep(u32(1000.0 * sleepTime));
        }

        g_renderer->WaitForIdle();
    }

    resourceAllocator.clear();
    g_renderer->Deinit();
	destroyRenderer(g_renderer);

	glfwTerminate();
	return 0;
}