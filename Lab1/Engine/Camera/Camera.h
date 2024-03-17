#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    Camera() {
        center = XMFLOAT3(0.0f, 0.0f, 0.0f);
        r = 5.0f;
        theta = XM_PIDIV4;
        phi = -XM_PIDIV4;
        updateViewMatrix();
    }

    void changePosition(float dphi, float dtheta, float dr) {
        phi -= dphi;
        theta += dtheta;
        theta = min(max(theta, -XM_PIDIV2), XM_PIDIV2);
        r += dr;
        if (r < 2.0f) {
            r = 2.0f;
        }
        updateViewMatrix();
    }

    XMMATRIX& getViewMatrix() {
        return viewMatrix;
    };
private:
    XMMATRIX viewMatrix;
    XMFLOAT3 center;
    float r;
    float theta;
    float phi;

    void updateViewMatrix() {
        XMFLOAT3 pos = XMFLOAT3(cosf(theta) * cosf(phi), sinf(theta), cosf(theta) * sinf(phi));
        pos.x = pos.x * r + center.x;
        pos.y = pos.y * r + center.y;
        pos.z = pos.z * r + center.z;
        float upTheta = theta + XM_PIDIV2;
        XMFLOAT3 up = XMFLOAT3(cosf(upTheta) * cosf(phi), sinf(upTheta), cosf(upTheta) * sinf(phi));

        viewMatrix = DirectX::XMMatrixLookAtLH(
            DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
            DirectX::XMVectorSet(center.x, center.y, center.z, 0.0f),
            DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
        );
    }
};

