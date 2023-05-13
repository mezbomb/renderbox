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
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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

bool Window::isWindowActive() {
    SDL_PollEvent(&m_WindowEvent);
    return (m_WindowEvent.type == SDL_QUIT) ? false : true;
}