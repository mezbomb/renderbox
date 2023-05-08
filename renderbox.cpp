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
        if ( false == window.isWindowActive()) { break; }
        else { engine->render(); }
    }
    engine->shutdown();

    return 0;
}