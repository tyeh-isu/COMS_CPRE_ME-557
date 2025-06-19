#include "my_game_object.h"

glm::mat4 TransformComponent::mat4() 
{
	// Matrix corresponds to tranlate scale * Rz * Rx * Ry * transform (row, pitch, yaw)
	// Rotation conventation uses Tait-Bryan angles with axis order Y(1), X(2), Z(3)
	
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);

	// This is the order of the mulipilcation
	// Rz * Rx * Ry
	return glm::mat4
	{
		{
			scale.x * (c1 * c3 + s1 * s2 * s3),
			scale.x * (c2 * s3),
			scale.x * (c1 * s2 * s3 - c3 * s1),
			0.0f,
		},
		{
			scale.y * (c3 * s1 * s2 - c1 * s3),
			scale.y * (c2 * c3),
			scale.y * (c1 * c3 * s2 + s1 * s3),
			0.0f,
		},
		{
			scale.z * (c2 * s1),
			scale.z * (-s2),
			scale.z * (c1 * c2),
			0.0f,
		},
		{translation.x, translation.y, translation.z, 1.0f}
	};

	// This is the order of the mulipilcation
	// Ry * Rx * Rz
	/*return glm::mat4{
		{
			scale.x * (c1 * c3 - s1 * s2 * s3),
			scale.x * (c1 * s3 - c3 * s1 * s2),  
			scale.x * (c2 * s1) * -1.0f,
			0.0f,
		},
		{
			scale.y * (c2 * s3) * -1.0f,
			scale.y * (c2 * c3),
			scale.y * (s2), 
			0.0f,
		},
		{
			scale.z * (c1 * s2 * s3 + c3 * s1),
			scale.z * (s1 * s3 - c1 * c3 * s2),
			scale.z * (c1 * c2),
			0.0f,
		},
		{translation.x, translation.y, translation.z, 1.0f} };*/
}

glm::mat3 TransformComponent::normalMatrix() 
{
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 invScale = 1.0f / scale;

    return glm::mat3{
        {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1),
        },
        {
            invScale.y * (c3 * s1 * s2 - c1 * s3),
            invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3),
        },
        {
            invScale.z * (c2 * s1),
            invScale.z * (-s2),
            invScale.z * (c1 * c2),
        },
    };
}

MyGameObject MyGameObject::makePointLight(float intensity, float radius, glm::vec3 color) 
{
    MyGameObject gameObj = MyGameObject::createGameObject();
    gameObj.color = color;
    gameObj.transform.scale.x = radius;
    gameObj.pointLight = std::make_unique<PointLightComponent>();
    gameObj.pointLight->lightIntensity = intensity;
    return gameObj;
}

