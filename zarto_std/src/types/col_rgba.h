#pragma once
#include "col_rgb.h"

struct col_rgba : public col_rgb 
{
	uint8_t a;

	col_rgba lerp(const col_rgba& to, float t) const {
		return col_rgba{ col_rgb::lerp(to, t), lerp_channel(a, to.a, t) };
	}

	static col_rgba white(uint8_t alpha = 255)   { return col_rgba{ col_rgb::white(), alpha }; }
	static col_rgba black(uint8_t alpha = 255)   { return col_rgba{ col_rgb::black(), alpha }; }
	static col_rgba red(uint8_t alpha = 255)     { return col_rgba{ col_rgb::red(), alpha }; }
	static col_rgba green(uint8_t alpha = 255)   { return col_rgba{ col_rgb::green(), alpha }; }
	static col_rgba blue(uint8_t alpha = 255)    { return col_rgba{ col_rgb::blue(), alpha }; }
	static col_rgba yellow(uint8_t alpha = 255)  { return col_rgba{ col_rgb::yellow(), alpha }; }
	static col_rgba cyan(uint8_t alpha = 255)    { return col_rgba{ col_rgb::cyan(), alpha }; }
	static col_rgba magenta(uint8_t alpha = 255) { return col_rgba{ col_rgb::magenta(), alpha }; }
	static col_rgba orange(uint8_t alpha = 255)  { return col_rgba{ col_rgb::orange(), alpha }; }
	static col_rgba purple(uint8_t alpha = 255)  { return col_rgba{ col_rgb::purple(), alpha }; }
	static col_rgba gray(uint8_t alpha = 255)    { return col_rgba{ col_rgb::gray(), alpha }; }
	static col_rgba grey(uint8_t alpha = 255)    { return col_rgba{ col_rgb::grey(), alpha }; }
	static col_rgba transparent()                { return col_rgba{ col_rgb::black(), 0 }; }
};

using ColorRGBA = col_rgba;
