#include "url.h"

//Local function definitions
std::vector<utf8::uint32_t> urldecode(const std::vector<utf8::uint32_t>& enc, bool plus_is_space);
std::vector<utf8::uint32_t> urlencode(const std::vector<utf8::uint32_t>& dat, bool plus_is_space);
char get_hex(char i);
bool should_escape(utf8::uint32_t codepoint);

bool xwwwformurlencodedDecode::accepts_type(DocType type) const
{
	return type == UnicodeDocumentType;
}

bool xwwwformurlencodedDecode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> xwwwformurlencodedDecode::get_reverse_transform() const
{
	return std::make_unique<xwwwformurlencodedEncode>();
}

std::unique_ptr<Document> xwwwformurlencodedDecode::transform(const Document & input) const
{
	if (input.get_type() != UnicodeDocumentType)
	{
		throw TransformError("x-www-form-urlencoded Decoder only accepts unicode documents");
	}
	std::unique_ptr<MultipartDocument> result = std::make_unique<MultipartDocument>();

	const UnicodeDocument& doc = dynamic_cast<const UnicodeDocument&>(input);


	bool in_value = false;
	std::vector<utf8::uint32_t> keybuff;
	std::vector<utf8::uint32_t> valbuff;

	for (auto&& a : doc.data)
	{
		if (a == '=')
		{
			in_value = true;
		}
		else if (a == '&')
		{
			std::unique_ptr<UnicodeDocument> part = std::make_unique<UnicodeDocument>();
			part->data = std::move(urldecode(valbuff, true));
			result->data.push_back(std::make_pair(std::move(urldecode(keybuff, true)), std::move(part)));
			valbuff.clear();
			keybuff.clear();
			in_value = false;
		}
		else {
			if (in_value)
			{
				valbuff.push_back(a);
			}
			else {
				keybuff.push_back(a);
			}
		}
	}

	if (!keybuff.empty())
	{
		std::unique_ptr<UnicodeDocument> part = std::make_unique<UnicodeDocument>();
		part->data = std::move(urldecode(valbuff, true));
		result->data.push_back(std::make_pair(std::move(urldecode(keybuff, true)), std::move(part)));
		valbuff.clear();
		keybuff.clear();
		in_value = false;
	}

	return result;
}

const std::string xwwwformurlencodedDecode::get_description() const
{
	return "x-www-form-urlencoded";
}

bool xwwwformurlencodedEncode::accepts_type(DocType type) const
{
	return type == MultipartDocumentType;
}

bool xwwwformurlencodedEncode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> xwwwformurlencodedEncode::get_reverse_transform() const
{
	return std::make_unique<xwwwformurlencodedDecode>();
}

std::unique_ptr<Document> xwwwformurlencodedEncode::transform(const Document & input) const
{

	if (input.get_type() != MultipartDocumentType)
	{
		throw TransformError("x-www-form-urlencoded encoder only accepts multipart documents");
	}
	std::unique_ptr<UnicodeDocument> result = std::make_unique<UnicodeDocument>();

	const MultipartDocument& doc = dynamic_cast<const MultipartDocument&>(input);

	for (auto&& a : doc.data)
	{
		if (a.second->get_type() != UnicodeDocumentType)
		{
			throw TransformError("x-www-form-urlencoded encoder only accepts unicode documents inside the multipart");
		}
	}

	bool first = true;

	for (auto&& a : doc.data)
	{
		if (first)
		{
			first = false;
		}
		else {
			result->data.push_back('&');
		}
		std::vector<utf8::uint32_t> buff = std::move(urlencode(a.first, true));
		result->data.insert(result->data.end(), buff.begin(), buff.end());
		result->data.push_back('=');
		buff = std::move(urlencode(dynamic_cast<const UnicodeDocument*>(a.second.get())->data, true));
		result->data.insert(result->data.end(), buff.begin(), buff.end());
	}

	return result;
}

const std::string xwwwformurlencodedEncode::get_description() const
{
	return "x-www-form-urlencoded";
}

std::vector<utf8::uint32_t> urldecode(const std::vector<utf8::uint32_t>& enc, bool plus_is_space)
{
	char in_escaped = 0;
	char buffer = 0;
	std::vector<char> sequence;
	std::vector<utf8::uint32_t> result;

	for (auto&& a : enc)
	{
		if (in_escaped)
		{
			char val = 0;
			if (a >= '0' && a <= '9')
			{
				val = a - '0';
			}
			else if (a >= 'A' && a <= 'F')
			{
				val = a - 'A' + 10;
			}
			else if (a >= 'a' && a <= 'f')
			{
				val = a - 'a' + 10;
			}
			else {
				throw TransformError("Non-hexadecimal character in escape sequence");
			}
			in_escaped--;
			buffer |= val << (4 * in_escaped);
			if (!in_escaped)
			{
				sequence.push_back(buffer);
				buffer = 0;
			}
			continue;
		}
		if (a == '%')
		{
			in_escaped = 2;
		}
		else {
			if (!sequence.empty())
			{
				for (auto it = sequence.begin(); it != sequence.end();)
				{
					result.push_back(utf8::next(it, sequence.end()));
				}
				sequence.clear();
			}
			if (plus_is_space && a == '+')
			{
				result.push_back(' ');
			}
			else {
				result.push_back(a);
			}
		}
	}
	if (!sequence.empty())
	{
		for (auto it = sequence.begin(); it != sequence.end();)
		{
			result.push_back(utf8::next(it, sequence.end()));
		}
		sequence.clear();
	}
	return result;
}

std::vector<utf8::uint32_t> urlencode(const std::vector<utf8::uint32_t>& dat, bool plus_is_space)
{
	std::vector<utf8::uint32_t> result;

	for (auto&& a : dat)
	{
		if (plus_is_space && a == ' ')
		{
			result.push_back('+');
			continue;
		}
		if (should_escape(a))
		{
			std::vector<char> utf8enc;
			utf8::append(a, std::back_inserter(utf8enc));
			for (auto&& c : utf8enc)
			{
				result.push_back('%');
				result.push_back(get_hex((c >> 4) & 0xF));
				result.push_back(get_hex(c & 0xF));
			}
		}
		else
		{
			result.push_back(a);
		}
	}
	return result;
}

char get_hex(char i)
{
	if (i < 10)
	{
		return i + '0';
	}
	else {
		return i - 10 + 'A';
	}
}

//Based on rfc3986 section 2.3. Unreserved Characters (https://tools.ietf.org/html/rfc3986#section-2.3)
bool should_escape(utf8::uint32_t codepoint)
{
	if (codepoint >= 128)
	{
		return true;
	}
	return !(isalnum(codepoint) || codepoint == '-' || codepoint == '_' || codepoint == '.' || codepoint == '~');
}