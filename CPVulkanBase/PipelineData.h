#pragma once
#include <glm/glm.hpp>

struct VertexBuiltinOutput
{
	glm::vec4 position;
	float pointSize;
	float clipDistance[1];
};