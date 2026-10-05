#pragma once
#include <math.h>

struct vec2
{
	float x, y;

	// returns the lenght of the vector
	float lenght() const { return sqrtf(x * x + y * y); }

	// returns distance between this and other
	float dist_to(vec2 other) const {
		return sqrtf(powf(x - other.x, 2) + powf(y - other.y, 2));
	}

	// returns the normalized vector
	vec2 normalize() const  {
		float l = lenght();

		return vec2{ x / l, y / l };
	}

	// sets this vector as his normalized version
	void normalize_in_place() {
		float l = lenght();
		x = x / l;
		y = y / l;
	}

	vec2 operator+(const vec2& other) const { return vec2{ x + other.x, y + other.y }; }
	vec2 operator-(const vec2& other) const { return vec2{ x - other.x, y - other.y }; }
	vec2 operator*(const vec2& other) const { return vec2{ x * other.x, y * other.y }; }
	vec2 operator/(const vec2& other) const { return vec2{ x / other.x, y / other.y }; }

	vec2 operator+(float s) const { return vec2{ x + s, y + s }; }
	vec2 operator-(float s) const { return vec2{ x - s, y - s }; }
	vec2 operator*(float s) const { return vec2{ x * s, y * s }; }
	vec2 operator/(float s) const { return vec2{ x / s, y / s }; }

	vec2 operator-() const { return vec2{ -x, -y }; }

	vec2& operator+=(const vec2& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	vec2& operator-=(const vec2& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	vec2& operator*=(const vec2& other) {
		x *= other.x;
		y *= other.y;
		return *this;
	}

	vec2& operator/=(const vec2& other) {
		x /= other.x;
		y /= other.y;
		return *this;
	}

	vec2& operator+=(float s) {
		x += s;
		y += s;
		return *this;
	}

	vec2& operator-=(float s) {
		x -= s;
		y -= s;
		return *this;
	}

	vec2& operator*=(float s) {
		x *= s;
		y *= s;
		return *this;
	}

	vec2& operator/=(float s) {
		x /= s;
		y /= s;
		return *this;
	}

	bool operator==(const vec2& other) const { return x == other.x && y == other.y; }
	bool operator!=(const vec2& other) const { return !(*this == other); }


};

inline vec2 operator*(float s, const vec2& v) { return vec2{ s * v.x, s * v.y }; }
inline vec2 operator/(float s, const vec2& v) { return vec2{ s / v.x, s / v.y }; }