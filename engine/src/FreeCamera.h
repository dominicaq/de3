#pragma once

#include "io/InputManager.h"
#include "components/GameObject.h"

#include <iostream>

class FreeCamera : public Script {
public:
    float defaultSpeed = 2.5f;
    float boostSpeed = 10.0f;
    float sensitivity = 0.1f;

private:
    float cameraSpeed = defaultSpeed;
    bool m_flashLightToggle = false;

public:
    void start() override {
        Input.setCursorMode(InputManager::CursorMode::DISABLED);
    }

    void update(const float& deltaTime) override {
        // Get the camera's position and Euler angles from the GameObject
        glm::vec3 position = gameObject->getPosition();
        glm::vec3 eulerAngles = gameObject->getEuler();
        glm::vec3 front = gameObject->getForward();
        glm::vec3 right = gameObject->getRight();
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        float dt = cameraSpeed * deltaTime;

        // Lock cursor
        if (Input.isKeyPressed(InputKeys::Q)) {
            Input.setCursorMode(InputManager::CursorMode::NORMAL);
        }
        if (Input.isKeyPressed(InputKeys::E)) {
            Input.setCursorMode(InputManager::CursorMode::DISABLED);
        }

        // Handle movement based on input
        if (Input.isKeyDown(InputKeys::W)) {
            position += dt * front;
        }
        if (Input.isKeyDown(InputKeys::S)) {
            position -= dt * front;
        }
        if (Input.isKeyDown(InputKeys::A)) {
            position -= right * dt;
        }
        if (Input.isKeyDown(InputKeys::D)) {
            position += right * dt;
        }
        if (Input.isKeyDown(InputKeys::SPACE)) {
            position += up * dt * 2.0f;
        }
        if (Input.isKeyDown(InputKeys::CTRL)) {
            position -= up * dt * 2.0f;
        }

        // Speed
        if (Input.isKeyDown(InputKeys::SHIFT)) {
            cameraSpeed = boostSpeed;
        } else {
            cameraSpeed = defaultSpeed;
        }

        // Flash light
        // if (Input.isKeyPressed(InputKeys::F)) {
        //     m_flashLightToggle = !m_flashLightToggle;
        //     auto& light = gameObject->getComponent<Light>();
        //     light.isActive = m_flashLightToggle;
        // }

        // Update the camera's position in the GameObject
        gameObject->setPosition(position);

        // Mouse input for look rotation
        if (Input.isCursorDisabled()) {
            float xOffset = -Input.getMouseXOffset() * sensitivity;
            float yOffset = Input.getMouseYOffset() * sensitivity;

            // Adjust camera angles
            eulerAngles.y += xOffset;  // Yaw (left/right)
            eulerAngles.x += yOffset;  // Pitch (up/down)

            // Constrain the pitch to avoid gimbal lock
            if (eulerAngles.x > 89.0f) {
                eulerAngles.x = 89.0f;
            }
            if (eulerAngles.x < -89.0f) {
                eulerAngles.x = -89.0f;
            }

            // Update the Euler angles in the GameObject
            gameObject->setEuler(eulerAngles);
        }
    }
};
