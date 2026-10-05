#pragma once
#define PI  3.14159265358979

#include "../types/vec2.h"
#include "../types/vec3.h"
#include "../types/matrix3x4.h"

namespace math
{
	bool world_to_screen(matrix3x4& viewmatrix, vec2 size, vec3 origin, vec2& vScreen);

	bool world_to_screen_depth(matrix3x4& viewmatrix, vec2 size, vec3 origin, vec3& vScreen);
}