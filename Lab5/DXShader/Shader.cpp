#include "Shader.h"
#include <map>
#include <iostream>
#include "D3DInclude.h"

Shader* Shader::loadShader(ID3D11Device* device, ShaderCreateInfo* pCreateInfos, uint32_t shaderAmount)
{
    std::map<ShaderType, ID3DBlob*> shadersBinaries;
    ID3DBlob* errorBlob = nullptr;
    ID3DBlob* tempBlob = nullptr;
    uint32_t compileFlags = 0;
    uint32_t vertexShaderIndex = 0;
    uint32_t pixelShaderIndex = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    D3DInclude includeObj;
    for (uint32_t i = 0; i < shaderAmount; i++)
    {
        if (FAILED(
            D3DCompileFromFile(pCreateInfos[i].pathToShader, nullptr, &includeObj, "main", pCreateInfos[i].shaderType ==
                VERTEX_SHADER ? "vs_5_0" : "ps_5_0", compileFlags, NULL, &tempBlob,
                &errorBlob)))
        {
            if (errorBlob != nullptr)
            {
                for (size_t i = 0; i < errorBlob->GetBufferSize(); i += sizeof(char))
                {
                    std::cerr << *(((char*)errorBlob->GetBufferPointer()) + i);
                }
                std::cerr << std::endl;
            }
            throw std::runtime_error("Failed to compile shader");
        }
        if (tempBlob == nullptr)
        {
            throw std::runtime_error("Failed to compile shader");
        }
        if (pCreateInfos[i].shaderType == VERTEX_SHADER)
        {
            vertexShaderIndex = i;
        }
        else if (pCreateInfos[i].shaderType == PIXEL_SHADER)
        {
            pixelShaderIndex == i;
        }
        shadersBinaries[pCreateInfos[i].shaderType] = tempBlob;
        tempBlob = nullptr;
    }
    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    if (FAILED(
        device->CreateVertexShader(shadersBinaries[VERTEX_SHADER]->GetBufferPointer(), shadersBinaries[VERTEX_SHADER]->
            GetBufferSize(), nullptr,
            &vertexShader)))
    {
        throw std::runtime_error("Failed to make vertex shader");
    }
    if (FAILED(
        device->CreatePixelShader(shadersBinaries[PIXEL_SHADER]->GetBufferPointer(), shadersBinaries[PIXEL_SHADER]->
            GetBufferSize(), nullptr,
            &pixelShader)))
    {
        throw std::runtime_error("Failed to make pixel shader");
    }
#if defined(_DEBUG)
    if (pCreateInfos[vertexShaderIndex].shaderName)
    {
        vertexShader->SetPrivateData(WKPDID_D3DDebugObjectName,
                                     strlen(pCreateInfos[vertexShaderIndex].shaderName) * sizeof(char),
                                     pCreateInfos[vertexShaderIndex].shaderName);
    }
    if (pCreateInfos[pixelShaderIndex].shaderName)
    {
        pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName,
                                    strlen(pCreateInfos[pixelShaderIndex].shaderName) * sizeof(wchar_t),
                                    pCreateInfos[pixelShaderIndex].shaderName);
    }
#endif
    return new Shader(vertexShader, pixelShader, shadersBinaries[VERTEX_SHADER], shadersBinaries[PIXEL_SHADER]);
}

Shader::Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader,
               ID3DBlob* vertexShaderData, ID3DBlob* pixelShaderData) : vertexShader(vertexShader),
                                                                        pixelShader(pixelShader),
                                                                        vertexShaderData(vertexShaderData),
                                                                        pixelShaderData(pixelShaderData)
{
}

void Shader::makeInputLayout(ID3D11Device* device, ShaderVertexInput* pInputs, uint32_t inputsAmount)
{
    if (shaderInputs.empty())
    {
        shaderInputs.resize(inputsAmount);
        uint32_t offsetCounter = 0;
        for (uint32_t i = 0; i < inputsAmount; i++)
        {
            shaderInputs[i] = {
                pInputs[i].shaderVariableName, pInputs[i].variableIndex, pInputs[i].inputFormat, 0, offsetCounter,
                D3D11_INPUT_PER_VERTEX_DATA, 0
            };
            offsetCounter += pInputs[i].vertexSize;
        }

        if (FAILED(
            device->CreateInputLayout(shaderInputs.data(), shaderInputs.size(), vertexShaderData->GetBufferPointer(),
                vertexShaderData->GetBufferSize(), &inputLayout)))
        {
            throw std::runtime_error("Failed to create buffer layout");
        }
    }
}



void Shader::bind(ID3D11DeviceContext* deviceContext)
{
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
}

void Shader::draw(ID3D11DeviceContext* context, IndexBuffer* indexBuffer, VertexBuffer* vertexBuffer)
{
    UINT stride = vertexBuffer->vertexSize;
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, &vertexBuffer->buffer,
                                &stride, &offset);
    context->IASetInputLayout(inputLayout);
    context->IASetIndexBuffer(indexBuffer->buffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(indexBuffer->indexCount, 0, 0);
}

Shader::~Shader()
{
    if(inputLayout)
    {
        inputLayout->Release();

    }
    pixelShader->Release();
    vertexShader->Release();
    vertexShaderData->Release();
    pixelShaderData->Release();
}
