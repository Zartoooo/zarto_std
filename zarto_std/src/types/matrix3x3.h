#pragma once
#include <math.h>

struct matrix3x3 
{
private:
	float m[3][3];

public:
	float* get() { return *this->m; }
	const float* get() const { return *this->m; }

	float* operator[](int idx) { return m[idx]; }
	const float* operator[](int idx) const { return m[idx]; }
};