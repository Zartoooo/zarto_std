#include "./math.h"

bool math::world_to_screen(matrix3x4& viewmatrix, vec2 size, vec3 origin, vec2& vScreen)
{
	vec3 screen{};

	if (math::world_to_screen_depth(viewmatrix, size, origin, screen))
	{
		vScreen = vec2{ screen.x, screen.y };
		return true;
	}
	else
	{
		return true;
	}
}

bool math::world_to_screen_depth(matrix3x4& viewmatrix, vec2 size, vec3 origin, vec3& vScreen)
{

	float w = viewmatrix[3][0] * origin.x + viewmatrix[3][1] * origin.y + viewmatrix[3][2] * origin.z + viewmatrix[3][3];
	vScreen.z = 0;

	if (w > 0.01)
	{
		float inverseWidth = 1 / w;
		vScreen.x = (size.x / 2) + (0.5 * ((viewmatrix[0][0] * origin.x + viewmatrix[0][1] * origin.y + viewmatrix[0][2] * origin.z + viewmatrix[0][3]) * inverseWidth) * size.x + 0.5);
		vScreen.y = (size.y / 2) - (0.5 * ((viewmatrix[1][0] * origin.x + viewmatrix[1][1] * origin.y + viewmatrix[1][2] * origin.z + viewmatrix[1][3]) * inverseWidth) * size.y + 0.5);
		return true;
	}
	return false;
}
