#pragma once
#include <cstdint>


struct col_rgb 
{
	uint8_t r, g, b;

	static uint8_t lerp_channel(uint8_t from, uint8_t to, float t) {
		float v = (float)from + ((float)to - (float)from) * t;

		if (v < 0.f) v = 0.f;
		else if (v > 255.f) v = 255.f;

		return (uint8_t)(v + 0.5f);
	}

	col_rgb lerp(const col_rgb& to, float t) const {
		return col_rgb{
			lerp_channel(r, to.r, t),
			lerp_channel(g, to.g, t),
			lerp_channel(b, to.b, t)
		};
	}

	static col_rgb white()   { return col_rgb{ 255, 255, 255 }; }
	static col_rgb black()   { return col_rgb{ 0, 0, 0 }; }
	static col_rgb red()     { return col_rgb{ 255, 0, 0 }; }
	static col_rgb green()   { return col_rgb{ 0, 255, 0 }; }
	static col_rgb blue()    { return col_rgb{ 0, 0, 255 }; }
	static col_rgb yellow()  { return col_rgb{ 255, 255, 0 }; }
	static col_rgb cyan()    { return col_rgb{ 0, 255, 255 }; }
	static col_rgb magenta() { return col_rgb{ 255, 0, 255 }; }
	static col_rgb orange()  { return col_rgb{ 255, 165, 0 }; }
	static col_rgb purple()  { return col_rgb{ 128, 0, 128 }; }
	static col_rgb gray()    { return col_rgb{ 128, 128, 128 }; }
	static col_rgb grey()    { return gray(); }
};

using ColorRGB = col_rgb;
