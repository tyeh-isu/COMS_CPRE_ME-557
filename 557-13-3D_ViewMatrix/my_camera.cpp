#include "my_camera.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

// std
#include <cassert>
#include <limits>


void MyCamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) 
{
    // Based on the equation from
    // https://www.songho.ca/opengl/gl_projectionmatrix.html
    //
    // Follow Vulkan's convention that X is right, Y is down and Z is into the screen

    m_m4ProjectionMatrix = glm::mat4{ 1.0f };

    //
    // glm matrix layout
    // 
    // [0,0] [1,0] [2,0] [3,0]
    // [0,1] [1,1] [2,1] [3,1]
    // [0,2] [1,2] [2,2] [3,2]
    // [0,3] [1,3] [2,3] [3,3]
    // 
    m_m4ProjectionMatrix[0][0] = 2.f / (right - left);
    m_m4ProjectionMatrix[1][1] = 2.f / (top - bottom);
    m_m4ProjectionMatrix[2][2] = -2.0f / (far - near);
    m_m4ProjectionMatrix[3][0] = -(right + left) / (right - left);
    m_m4ProjectionMatrix[3][1] = -(top + bottom) / (top - bottom);
    m_m4ProjectionMatrix[3][2] = -(far + near) / (far - near);
}

void MyCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far)
{
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

    // Use OpenGL matrix but follow Vulkan's convention that Y is down
    const float tanHalfFovy = tan(fovy / 2.f);

    // Based on the equation from
    // https://www.songho.ca/opengl/gl_projectionmatrix.html
    //
    // Follow Vulkan's convention that X is right, Y is down and Z is into the screen


    const float right = near * tanHalfFovy * aspect;  // Right is positive
    const float left = -1.0f * right;                 // Left is minus X (because near is a negative value)

    const float bottom = near * tanHalfFovy;          // bottom is positive (because Y is down)
    const float top = -1.0f * bottom;                 // top is negative

    //
    // Use this matrix such that it will become X - right, Y - up, Z - out of the screen
    //
    //
    // glm matrix layout
    // 
    // [0,0] [1,0] [2,0] [3,0]
    // [0,1] [1,1] [2,1] [3,1]
    // [0,2] [1,2] [2,2] [3,2]
    // [0,3] [1,3] [2,3] [3,3]
    // 
    m_m4ProjectionMatrix = glm::mat4{ 0.0f };
    m_m4ProjectionMatrix[0][0] = 2.0f * near / (right - left);
    m_m4ProjectionMatrix[2][0] = (right + left) / (right - left);
    m_m4ProjectionMatrix[1][1] = 2.0f * near / (top - bottom);
    m_m4ProjectionMatrix[2][1] = (top + bottom) / (top - bottom);
    m_m4ProjectionMatrix[2][2] = -1.0f * (far + near) / (far - near);
    m_m4ProjectionMatrix[3][2] = -2.0f * far * near / (far - near);
    m_m4ProjectionMatrix[2][3] = -1.0f;
}

void MyCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up)
{
    //
    // glm matrix layout
    // 
    // [0,0] [1,0] [2,0] [3,0]
    // [0,1] [1,1] [2,1] [3,1]
    // [0,2] [1,2] [2,2] [3,2]
    // [0,3] [1,3] [2,3] [3,3]
    // 
    // If using Y-down coordinate system
    const glm::vec3 w{ glm::normalize(direction)}; // Z is out of the screen
    const glm::vec3 u{ glm::normalize(glm::cross(up, w)) };
    const glm::vec3 v{ glm::cross(w, u) };

    // View to world
    m_m4ViewMatrix = glm::mat4{ 1.f };
    m_m4ViewMatrix[0][0] = u.x;
    m_m4ViewMatrix[1][0] = u.y;
    m_m4ViewMatrix[2][0] = u.z;
    m_m4ViewMatrix[0][1] = v.x;
    m_m4ViewMatrix[1][1] = v.y;
    m_m4ViewMatrix[2][1] = v.z;
    m_m4ViewMatrix[0][2] = w.x;
    m_m4ViewMatrix[1][2] = w.y;
    m_m4ViewMatrix[2][2] = w.z;
    m_m4ViewMatrix[3][0] = position.x;
    m_m4ViewMatrix[3][1] = position.y;
    m_m4ViewMatrix[3][2] = position.z;

    // World to view
    m_m4ViewMatrix = glm::inverse(m_m4ViewMatrix);
}

void MyCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up)
{
    // Z direction is out of the screen so we need to flip the view direction
    setViewDirection(position, position - target, up);
}

