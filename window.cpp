#pragma once
#include "window.h"
#include "SDL_syswm.h"
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;

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
                float zOffset = event.wheel.y > 0 ? 1.0f : -1.0f;
                float scrollFactor = 1.0f;
                io.MouseWheel = zOffset;
                zOffset *= scrollFactor;

                // Define the minimum and maximum distances from the focus point
                const float minDistance = 5.0f;
                const float maxDistance = 100.0f;

                // Calculate the current distance from the focus point
                float currentDistance = XMVectorGetX(XMVector3Length(m_pCamera->m_Position - m_pCamera->m_Focus));

                // Calculate the desired new distance based on the scroll wheel input
                float newDistance = currentDistance + zOffset;

                // Clamp the new distance within the desired range
                float clampedDistance = std::clamp(newDistance, minDistance, maxDistance);

                // Calculate the direction vector from the focus point to the current camera position
                XMVECTOR direction = XMVector3Normalize(m_pCamera->m_Position - m_pCamera->m_Focus);

                // Calculate the new position by multiplying the direction vector by the clamped distance
                XMVECTOR newPosition = m_pCamera->m_Focus + direction * clampedDistance;

                // Update the camera's position
                m_pCamera->m_Position = newPosition;

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

                if (io.MouseDown[1])
                {
                    // Compute the mouse movement delta
                    float deltaX = static_cast<float>(event.motion.xrel);
                    float deltaY = static_cast<float>(event.motion.yrel);

                    // Adjust the camera rotation based on the mouse movement
                    float rotationSpeed = 0.01f;

                    // Calculate the camera-to-focus vector
                    XMVECTOR cameraToFocus = m_pCamera->m_Focus - m_pCamera->m_Position;

                    // Calculate the distance from the camera to the focus point
                    float distance = XMVectorGetX(XMVector3Length(cameraToFocus));

                    // Calculate the right and up vectors
                    XMVECTOR right = XMVector3Normalize(XMVector3Cross(m_pCamera->m_Up, cameraToFocus));
                    XMVECTOR up = XMVector3Normalize(XMVector3Cross(cameraToFocus, right));

                    // Rotate the camera around the focus point
                    XMMATRIX rotation = XMMatrixRotationAxis(up, deltaX * rotationSpeed)
                        * XMMatrixRotationAxis(right, deltaY * rotationSpeed);
                    cameraToFocus = XMVector3Transform(cameraToFocus, rotation);

                    // Update the camera position based on the new camera-to-focus vector
                    m_pCamera->m_Position = m_pCamera->m_Focus - cameraToFocus;

                    // Restore the distance from the focus point
                    XMVECTOR newCameraToFocus = m_pCamera->m_Focus - m_pCamera->m_Position;
                    newCameraToFocus = XMVector3Normalize(newCameraToFocus);
                    m_pCamera->m_Position = m_pCamera->m_Focus - distance * newCameraToFocus;
                }
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

void Window::setCamera(Camera* pCam) {
    m_pCamera = pCam;
}