#pragma once
#include <math.h>

struct vec3
{
	float x, y, z;

	// returns the lenght of the vector
	float lenght() const { return sqrtf(x * x + y * y + z * z); }

	// returns distance between this and other
	float dist_to(vec3 other) const {
		return sqrtf(powf(x - other.x, 2) + powf(y - other.y, 2) + powf(z - other.z, 2));
	}

	// returns the normalized vector
	vec3 normalize() const {
		float l = lenght();

		return vec3{ x / l, y / l, z / l };
	}

	// sets this vector as his normalized version
	void normalize_in_place() {
		float l = lenght();
		x = x / l;
		y = y / l;
		z = z / l;
	}

	vec3 operator+(const vec3& other) const { return vec3{ x + other.x, y + other.y, z + other.z }; }
	vec3 operator-(const vec3& other) const { return vec3{ x - other.x, y - other.y, z - other.z }; }
	vec3 operator*(const vec3& other) const { return vec3{ x * other.x, y * other.y, z * other.z }; }
	vec3 operator/(const vec3& other) const { return vec3{ x / other.x, y / other.y, z / other.z }; }

	vec3 operator+(float s) const { return vec3{ x + s, y + s, z + s }; }
	vec3 operator-(float s) const { return vec3{ x - s, y - s, z - s }; }
	vec3 operator*(float s) const { return vec3{ x * s, y * s, z * s }; }
	vec3 operator/(float s) const { return vec3{ x / s, y / s, z / s }; }

	vec3 operator-() const { return vec3{ -x, -y, -z }; }

	vec3& operator+=(const vec3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	vec3& operator-=(const vec3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	vec3& operator*=(const vec3& other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	vec3& operator/=(const vec3& other) {
		x /= other.x;
		y /= other.y;
		z /= other.z;
		return *this;
	}

	vec3& operator+=(float s) {
		x += s;
		y += s;
		z += s;
		return *this;
	}

	vec3& operator-=(float s) {
		x -= s;
		y -= s;
		z -= s;
		return *this;
	}

	vec3& operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	vec3& operator/=(float s) {
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}

	bool operator==(const vec3& other) const { return x == other.x && y == other.y && z == other.z; }
	bool operator!=(const vec3& other) const { return !(*this == other); }

	
};

inline vec3 operator*(float s, const vec3& v) { return vec3{ s * v.x, s * v.y, s * v.z }; }
inline vec3 operator/(float s, const vec3& v) { return vec3{ s / v.x, s / v.y, s / v.z }; }