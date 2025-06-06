#ifndef __MY_CAMERA_H__
#define __MY_CAMERA_H__

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class MyCamera 
{
public:
    void setOrthographicProjection(
        float left, float right, float top, float bottom, float near, float far);

    void setPerspectiveProjection(float fovy, float aspect, float near, float far);

    void setViewDirection(glm::vec3 position, glm::vec3 dirction, glm::vec3 up = glm::vec3{ 0.0f, 1.0f, 0.0f });
    void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, 1.0f, 0.0f });
    void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

    const glm::mat4& projectionMatrix()  const { return m_m4ProjectionMatrix; }
    const glm::mat4& viewMatrix()        const { return m_m4ViewMatrix; }

private:
    glm::mat4 m_m4ProjectionMatrix{ 1.f };
    glm::mat4 m_m4ViewMatrix{ 1.f };
};

#endif

