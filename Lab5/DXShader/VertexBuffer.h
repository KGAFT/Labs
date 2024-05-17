#pragma once

#include <d3d11.h>
#include <string>
#include <stdexcept>

class VertexBuffer
{
    friend class Shader;


public:
    VertexBuffer(ID3D11Device* device, size_t dataSize, size_t stepSize, void* verticesList,
                 const char* bufferName = nullptr) : device(device), vertexSize(stepSize)
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = dataSize;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = verticesList;


        if (FAILED(device->CreateBuffer(&bufferDesc, &initData, &buffer)))
        {
            throw std::runtime_error("Failed to create buffer");
        }

#if defined(_DEBUG)
        if (bufferName)
        {
            std::string layoutName = bufferName;
            layoutName += " layout";
            buffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(bufferName) * sizeof(char), bufferName);
        }
#endif
    }

private:
    ID3D11Buffer* buffer;
    ID3D11Device* device;
    unsigned int vertexSize;

public:
    ~VertexBuffer()
    {
        buffer->Release();
    }
};
