#pragma once

#include <DirectXMath.h>
#include "../../Window/WindowInputSystem.h"
using namespace DirectX;

class Camera : public IWindowKeyCallback, public IWindowMouseCallback
{
public:
    Camera() {
        focus = XMFLOAT3(0.0f, 0.0f, 0.0f);
        r = 5.0f;
        theta = -XM_PIDIV4;
        phi = XM_PIDIV4;
        keys.push_back({ 'w', true, KEY_DOWN });
        keys.push_back({ 's', true, KEY_DOWN });
        keys.push_back({ 'a', true, KEY_DOWN });
        keys.push_back({ 'd', true, KEY_DOWN });
        keys.push_back({ 'c', true, KEY_DOWN });
        keys.push_back({ VK_SPACE, false, KEY_DOWN });

        position = XMFLOAT3(focus.x - cosf(theta) * cosf(phi) * r,
            focus.y - sinf(theta) * r,
            focus.z - cosf(theta) * sinf(phi) * r);
        updateViewMatrix();
    }
private:
    XMMATRIX viewMatrix;
    XMFLOAT3 focus;
    XMFLOAT3 position;
    float r;
    float theta;
    float phi;
    std::vector<WindowKey> keys;
    bool rotating = false;
    float oldPosX = 0;
    float oldPosY = 0;
public:
    void changePosition(float dx, float dy, float dz) {
        focus = XMFLOAT3(focus.x + dx * cosf(phi) - dz * sinf(phi), focus.y + dy, focus.z + dx * sinf(phi) + dz * cosf(phi));
        position = XMFLOAT3(position.x + dx * cosf(phi) - dz * sinf(phi), position.y + dy, position.z + dx * sinf(phi) + dz * cosf(phi));

        updateViewMatrix();
    }
    void zoom(float dr) {
        r += dr;
        if (r < 1.0f) {
            r = 1.0f;
        }
        position = XMFLOAT3(focus.x - cosf(theta) * cosf(phi) * r,
            focus.y - sinf(theta) * r,
            focus.z - cosf(theta) * sinf(phi) * r);

        updateViewMatrix();
    }
    XMFLOAT3 getPosition() {
        return position;
    }

    void rotate(float dphi, float dtheta) {
        phi -= dphi;
        theta -= dtheta;
        theta = min(max(theta, -XM_PIDIV2), XM_PIDIV2);
        focus = XMFLOAT3(cosf(theta) * cosf(phi) * r + position.x,
            sinf(theta) * r + position.y,
            cosf(theta) * sinf(phi) * r + position.z);

        updateViewMatrix();
    }

    XMMATRIX& getViewMatrix() {
        return viewMatrix;
    }

    void mouseMove(uint32_t x, uint32_t y) override {
        if (rotating) {
            rotate((x-oldPosX)/100, (y-oldPosY)/100);
        }
        oldPosX = x;
        oldPosY = y;
    }
    void mouseKey(uint32_t key) override {
        rotating = key == WM_LBUTTONDOWN ? true : key == WM_LBUTTONUP ? false : rotating;
    }

    void keyEvent(WindowKey key) override {
        float sens = 0.11f;
        float dx = 0,  dy = 0, dz = 0;
        switch (key.key) {
        case 'w':
            dx = sens;
            break;
        case 's':
            dx = -1*sens;
            break;
        case 'a':
            dz = sens;
            break;
        case 'd':
            dz = -1*sens;
            break;
        case 'c':
            dy = -1*sens;
            break;
        case VK_SPACE:
            dy = sens;
        default:
            break;
        }
        changePosition(dx, dy, dz);
    }
    WindowKey* getKeys(uint32_t* pKeysAmountOut) override {
        *pKeysAmountOut = keys.size();
        return keys.data();
    }

private:
    void updateViewMatrix() {
        float upTheta = theta + XM_PIDIV2;
        XMFLOAT3 up = XMFLOAT3(cosf(upTheta) * cosf(phi), sinf(upTheta), cosf(upTheta) * sinf(phi));

        viewMatrix = DirectX::XMMatrixLookAtLH(
            DirectX::XMVectorSet(position.x, position.y, position.z, 0.0f),
            DirectX::XMVectorSet(focus.x, focus.y, focus.z, 0.0f),
            DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
        );
    }
};

