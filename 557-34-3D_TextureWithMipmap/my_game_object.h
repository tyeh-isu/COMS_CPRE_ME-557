#ifndef __MY_GAMEOBJECT_H__
#define __MY_GAMEOBJECT_H__

#include "my_model.h"
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>
#include <unordered_map>

struct TransformComponent
{
	glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };

	// Matrix corresponds to the overall transformation - scale * Rz * Rx * Ry * transform (row, pitch, yaw)
	// Rotation conventation uses Tait-Bryan angles with axis order Y(1), X(2), Z(3)
    // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	glm::mat4 mat4();

	glm::mat3 normalMatrix();
};

struct PointLightComponent 
{
	float lightIntensity = 1.0f;
};

class MyGameObject
{
public:
	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, MyGameObject>;

	static MyGameObject createGameObject()
	{
		static id_t currentID = 0;
		return MyGameObject{ currentID++ };
	}

	static MyGameObject makePointLight(
		float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

	MyGameObject(const MyGameObject&) = delete;
	MyGameObject& operator=(const MyGameObject&) = delete;
	MyGameObject(MyGameObject&&) = default;
	MyGameObject& operator=(MyGameObject&&) = default;

	id_t                     getID() const { return m_iID; }
	glm::vec3                color{};
	TransformComponent       transform{};

	// Optional pointer components
	// meaning the game object could be either empty object - for camera
	// or a light object
	std::shared_ptr<MyModel> simpleModel{};
	std::shared_ptr<MyModel> textureModel{};
	std::unique_ptr<PointLightComponent> pointLight = nullptr;

private:
	MyGameObject(id_t objID) : m_iID{ objID } {}
	id_t m_iID;
};

#endif

