#pragma once
#include "stdafx.h"
#include "window.h"
#include "renderengine.h"

int main(int argc, char* argv[])
{
    Window        window;
    RenderEngine* engine = RenderEngineFactory::CreateRenderEngine(ENGINE_TYPE::ENGINE_D3D12);
    engine->init(window);

    while (true) {
        engine->m_Simulation.m_EndTime = std::chrono::high_resolution_clock::now();

        float time =
            std::chrono::duration<float, std::milli>(engine->m_Simulation.m_EndTime - engine->m_Simulation.m_StartTime).count();
        if (time < (1000.0f / 60.0f))
        {
            continue;
        }

        engine->m_Simulation.m_StartTime = std::chrono::high_resolution_clock::now();
        engine->m_ElapsedTime += time;
        if ( false == window.isWindowActive()) { break; }
        else { engine->render(); }
    }
    engine->shutdown();

    return 0;
}