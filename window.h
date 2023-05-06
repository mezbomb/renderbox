#pragma once

#include "stdafx.h"

class Window
{
public:
    Window();
    ~Window();

    SDL_Window*  getWindow()  const  { return m_pWindow; }
    SDL_Surface* getSurface() const  { return m_pWindowSurface; }
    HWND         getHWND()    const  { return m_hWnd; }
    bool         isWindowActive();

    void update();

private:
    const int    m_width;
    const int    m_height;
    SDL_Window*  m_pWindow;
    SDL_Surface* m_pWindowSurface;
    SDL_Event    m_WindowEvent;
    HWND         m_hWnd;
};

