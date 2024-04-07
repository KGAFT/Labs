#pragma once

#include <d3d11.h>

class IndexBuffer
{
    friend class Shader;
private:
    IndexBuffer(ID3D11Buffer* buffer, ID3D11Device* device, unsigned int indexCount) : buffer(buffer), device(device),
        indexCount(indexCount) {}
private:
    ID3D11Buffer* buffer;
    ID3D11Device* device;
    unsigned int indexCount;
public:
    ~IndexBuffer() {
        buffer->Release();
    }
};

