#include "ConstantBuffer.h"
#include <stdexcept>

ConstantBuffer::ConstantBuffer(ID3D11Device* device, void* initData, size_t initDataSize)
{
    size_t cBufferSize;
    for (cBufferSize = initDataSize; cBufferSize % 16 != 0; cBufferSize++);

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = cBufferSize;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA initializeData = {};
    initializeData.pSysMem = &initData;

    if (FAILED(device->CreateBuffer(&bufferDesc, &initializeData, &buffer))) {
        throw std::runtime_error("Failed to create constant buffer");
    }
    
}

void ConstantBuffer::updateData(ID3D11DeviceContext* context, void* newData)
{
    context->UpdateSubresource(buffer, NULL, nullptr, &newData, NULL, NULL);
}

void ConstantBuffer::bindToVertexShader(ID3D11DeviceContext* context) {
    context->VSSetConstantBuffers(0, 1, &buffer);
}
void ConstantBuffer::bindToPixelShader(ID3D11DeviceContext* context) {
    context->PSSetConstantBuffers(0, 1, &buffer);
}