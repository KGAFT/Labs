#pragma once

#include <fstream>
#include <d3dcompiler.h>

class D3DInclude : public ID3DInclude {
public:
    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);

    HRESULT __stdcall Close(LPCVOID pData);
};
