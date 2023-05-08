#pragma once
#include "stdafx.h"



struct ModelViewProjection
{
    DirectX::XMMATRIX projectionMatrix;
    DirectX::XMMATRIX modelMatrix;
    DirectX::XMMATRIX viewMatrix;
};

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    WCHAR* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

// Helper function for resolving the full path of assets.
std::wstring GetAssetFullPath(LPCWSTR assetName)
{
    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    std::wstring assetPath = assetsPath;
    return assetPath + assetName;
}





