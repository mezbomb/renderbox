#pragma once
#include "window.h"
#include "SDL_syswm.h"

Window::Window() :
    m_width(1280),
    m_height(720),
    m_pWindow(nullptr),
    m_pWindowSurface(nullptr),
    m_WindowEvent(SDL_Event()),
    m_hWnd(nullptr){

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Window Initialization Failure!", SDL_GetError(), nullptr);
    }

    m_pWindow = SDL_CreateWindow("SDL Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_width,
        m_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (nullptr == m_pWindow) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Window Initialization Failure!", SDL_GetError(), nullptr);
    }
    else {
        m_pWindowSurface = SDL_GetWindowSurface(m_pWindow);
        SDL_SysWMinfo info = {};
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(m_pWindow, &info);
        m_hWnd = info.info.win.window;
        int test = 0;
    }
}

Window::~Window() {
    SDL_DestroyWindow(m_pWindow);
    SDL_Quit();
}

void Window::update() {
    if (SDL_UpdateWindowSurface(m_pWindow) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Window Update Failure!", SDL_GetError(), nullptr);
    }
}

extern bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event* event);

bool Window::isWindowActive() {
    ImGuiIO& io = ImGui::GetIO();

    // Poll events from SDL
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.

        if ( false && io.WantCaptureKeyboard || io.WantCaptureMouse) {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
        else {
            switch (event.type)
            {
            case SDL_QUIT: { return false; }
            case SDL_MOUSEWHEEL:
            {
                io.MouseWheel = event.wheel.y > 0 ? 1 : -1;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                io.MouseDown[event.button.button] = true;
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                io.MouseDown[event.button.button] = false;
                break;
            }
            case SDL_MOUSEMOTION:
            {
                io.MousePos = ImVec2(event.motion.x, event.motion.y);
                break;
            }
            case SDL_TEXTINPUT:
            {
                io.AddInputCharactersUTF8(event.text.text);
                break;
            }
            //case SDL_KEYDOWN:
            //case SDL_KEYUP:
            //{
            //    int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
            //    io.KeysDown[key] = (event.type == SDL_KEYDOWN);
            //    io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            //    io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            //    io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            //    io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
            //    break;
            //}
            default:
                break;
            }
        }
    }
    return true;
}