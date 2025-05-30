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

    const glm::mat4& projectionMatrix() const { return m_m4ProjectionMatrix; }

private:
    glm::mat4 m_m4ProjectionMatrix{ 1.f };
};

#endif

