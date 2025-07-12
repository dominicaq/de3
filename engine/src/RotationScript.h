#pragma once

#include "components/GameObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class RotationScript : public Script {
public:
    glm::vec3 rotationSpeed = glm::vec3(0.0f, 1.0f, 0.0f); // Rotation speed in radians per second (X, Y, Z)

private:
    glm::vec3 m_currentRotation = glm::vec3(0.0f);

    void start() override {
        // Initialize current rotation from the object's existing rotation
        m_currentRotation = glm::eulerAngles(gameObject->getRotation());
    }

    void update(const float& deltaTime) override {
        // Update rotation angles
        m_currentRotation += rotationSpeed * deltaTime;

        // Normalize angles to prevent floating point precision issues
        for (int i = 0; i < 3; i++) {
            if (m_currentRotation[i] > glm::two_pi<float>()) {
                m_currentRotation[i] -= glm::two_pi<float>();
            }
            else if (m_currentRotation[i] < -glm::two_pi<float>()) {
                m_currentRotation[i] += glm::two_pi<float>();
            }
        }

        // Convert to quaternion and apply
        glm::quat rotationQuat = glm::quat(m_currentRotation);
        gameObject->setRotation(rotationQuat);
    }
};
