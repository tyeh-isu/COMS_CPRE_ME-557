#include "my_camera.h"
#include "glm/ext/matrix_clip_space.hpp"

// std
#include <cassert>
#include <limits>
#include <iostream>

void printMatrix(glm::mat4& mat)
{
    std::cout << mat[0][0] << " " << mat[0][1] << " " << mat[0][2] << " " << mat[0][3] << std::endl;
    std::cout << mat[1][0] << " " << mat[1][1] << " " << mat[1][2] << " " << mat[1][3] << std::endl;
    std::cout << mat[2][0] << " " << mat[2][1] << " " << mat[2][2] << " " << mat[2][3] << std::endl;
    std::cout << mat[3][0] << " " << mat[3][1] << " " << mat[3][2] << " " << mat[3][3] << std::endl;
}

void MyCamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) 
{
    m_m4ProjectionMatrix = glm::orthoRH(left, right, bottom, top, near, far);
}

void MyCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far)
{
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    
    m_m4ProjectionMatrix = glm::perspective(fovy, aspect, near, far);

    // *-1 to make it right hand coordinatge
    // it is because Vulkan NDC space points downward by default everything will get flipped
    m_m4ProjectionMatrix[1][1] *= -1.0f;

    /*
    // Otherwise we can use the matrix below to make the projection matrix
    // like we had before that is still use right hand coordinate system,
    // but X-axix to the right, Y-axis down and Z-axis into the screen
    const float tanHalfFovy = tan(fovy / 2.f);
    projectionMatrix = glm::mat4{ 0.0f };
    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = -1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);

    printMatrix(projectionMatrix);*/
}

