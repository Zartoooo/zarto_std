#pragma once
#include <math.h>
#include "matrix3x4.h"

struct matrix4x4
{
private:
	float m[4][4];

public:
	float* get() { return *this->m; }
	const float* get() const { return *this->m; }

	inline const matrix3x4& as_3x4() {
		return *((const matrix3x4*)this);
	}

	float* operator[](int idx) { return m[idx]; }
	const float* operator[](int idx) const { return m[idx]; }
};