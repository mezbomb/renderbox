#pragma once

#include "stdafx.h"
#include "camera.h"

class Window
{
public:
    Window();
    ~Window();

    SDL_Window*  getWindow()  const { return m_pWindow; }
    SDL_Surface* getSurface() const { return m_pWindowSurface; }
    HWND         getHWND()    const { return m_hWnd; }
    const int    getHeight()  const { return m_height; }
    const int    getWidth()   const { return m_width; }
    bool         isWindowActive();

    void setCamera( Camera* );
    void update();

private:
    const int    m_width;
    const int    m_height;
    SDL_Window*  m_pWindow;
    SDL_Surface* m_pWindowSurface;
    SDL_Event    m_WindowEvent;
    HWND         m_hWnd;

    Camera*      m_pCamera;
};

