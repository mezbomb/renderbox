#pragma once
#include "stdafx.h"

class Simulation
{
public:
    std::chrono::steady_clock::time_point m_StartTime;
    std::chrono::steady_clock::time_point m_EndTime;
    float m_ElapsedTime;
};