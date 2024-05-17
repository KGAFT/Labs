#pragma once

#include <d3d11.h>
#include <string>
#include <stdexcept>

class IndexBuffer
{
    friend class Shader;

public:
    IndexBuffer(ID3D11Device* device, uint32_t* indices, uint32_t indicesCount, const char* bufferName = nullptr) : device(device),
        indexCount(indicesCount)
    {
        D3D11_BUFFER_DESC iBufferDesc;
        memset(&iBufferDesc, 0, sizeof(iBufferDesc));
        iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        iBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        iBufferDesc.ByteWidth = sizeof(DWORD) * indicesCount;

        D3D11_SUBRESOURCE_DATA indexData;
        indexData.pSysMem = indices;
        if (!FAILED(device->CreateBuffer(&iBufferDesc, &indexData, &buffer)))
        {
#if defined(_DEBUG)
            if (bufferName)
            {
                buffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(bufferName) * sizeof(char), bufferName);
            }
#endif
        }else
        {
            throw std::runtime_error("Failed to create index buffer");
        }
    }

private:
    ID3D11Buffer* buffer = nullptr;
    ID3D11Device* device;
    unsigned int indexCount;

public:
    ~IndexBuffer()
    {
        buffer->Release();
    }
};
