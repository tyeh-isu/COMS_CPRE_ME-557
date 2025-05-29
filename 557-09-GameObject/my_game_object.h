#ifndef __MY_GAMEOBJECT_H__
#define __MY_GAMEOBJECT_H__

#include "my_model.h"

// std
#include <memory>

struct Transform2dComponent
{
	glm::vec2 translation{};  // (position offset)
	glm::vec2 scale{ 1.f, 1.f };
	float rotation;

	glm::mat2 mat2()
	{
		// note: glm matrix is column major
		const float s = glm::sin(rotation);
		const float c = glm::cos(rotation);
		glm::mat2 rotMatrix{ {c, s}, {-s, c} };

		glm::mat2 scaleMat{ {scale.x, .0f}, {.0f, scale.y} };
		return rotMatrix * scaleMat;
	}
};

class MyGameObject
{
public:
	using id_t = unsigned int;

	static MyGameObject createGameObject()
	{
		static id_t currentID = 0;
		return MyGameObject{ currentID++ };
	}

	MyGameObject(const MyGameObject&) = delete;
	MyGameObject& operator=(const MyGameObject&) = delete;
	MyGameObject(MyGameObject&&) = default;
	MyGameObject& operator=(MyGameObject&&) = default;

	id_t                     getID() const { return m_iID; }
	std::shared_ptr<MyModel> model{};
	glm::vec3                color{};
	Transform2dComponent     transform2d{};

private:
	MyGameObject(id_t objID) : m_iID{ objID } {}
	id_t m_iID;
};

#endif

