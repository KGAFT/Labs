#pragma once

#include <d3d11.h>

class VertexBuffer
{
    friend class Shader;
private:
    VertexBuffer(ID3D11Buffer* buffer, ID3D11InputLayout* bufferLayout, ID3D11Device* device, ID3DBlob* shaderBlob,
        unsigned int vertexSize) : buffer(buffer), bufferLayout(bufferLayout),
        device(device), shaderBlob(shaderBlob),
        vertexSize(vertexSize) {}
private:
    ID3D11Buffer* buffer;
    ID3D11InputLayout* bufferLayout;
    ID3D11Device* device;
    ID3DBlob* shaderBlob;
    unsigned int vertexSize;
public:
    ~VertexBuffer() {
        bufferLayout->Release();
        buffer->Release();
    }
};

