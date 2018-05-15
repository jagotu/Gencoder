#pragma once

#include "utf8.h"

namespace utf8
{
	inline bool is_newline(utf8::uint32_t codepoint)
	{
		//Based on https://en.wikipedia.org/wiki/Newline#Unicode
		return (codepoint == 0xA || codepoint == 0xB || codepoint == 0xC || codepoint == 0xD || codepoint == 0x85 || codepoint == 0x2028 || codepoint == 0x2029);
	}

	inline bool is_space(utf8::uint32_t codepoint)
	{
		//Based on https://en.wikipedia.org/wiki/Whitespace_character#Unicode
		return is_newline(codepoint) || codepoint == 0x9 || codepoint == 0x20 || codepoint == 0xA0 || codepoint == 0x1680 || (codepoint >= 0x2000 && codepoint <= 0x200A) || codepoint == 0x202F || codepoint == 0x205F || codepoint == 0x3000;
	}
}
