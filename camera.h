#pragma once
#include "stdafx.h"

class Camera {
public:
    Camera();
    ~Camera();

    DirectX::XMVECTOR m_Position;
    DirectX::XMVECTOR m_Focus;
    DirectX::XMVECTOR m_Up;

private:

};
