#include "b64.h"

#include <array>
#include "../utf8.h"
#include "../utf8_charclass.h"

//Based on https://stackoverflow.com/a/13935718/3864664

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

bool Base64Decode::accepts_type(DocType type) const
{
	return type == UnicodeDocumentType;
}

bool Base64Decode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> Base64Decode::get_reverse_transform() const
{
	return std::make_unique<Base64Encode>();
}

std::unique_ptr<Document> Base64Decode::transform(const Document& input) const
{
	if (input.get_type() != UnicodeDocumentType)
	{
		throw TransformError("Base64 Decoder only accepts unicode documents");
	}
	const UnicodeDocument& doc = dynamic_cast<const UnicodeDocument&>(input);

	std::unique_ptr<OctetDocument> result = std::make_unique<OctetDocument>();

	int i = 0;
	std::array<char, 4> char_array;
	
	int equals_count = 0;

	for (auto&& a : doc.data)
	{
		if (utf8::is_space(a))
		{
			continue;
		}
		if (equals_count > 2)
		{
			throw TransformError("Invalid base64 input");
		}
		if (a == '=')
		{
			equals_count++;
			continue;
		}
		if (equals_count != 0)
		{
			throw TransformError("Invalid base64 input");
		}

		if (a >= 128 || !is_base64(a))
		{
			throw TransformError("Encountered a non-base64 char");
		}
		char_array[i++] = (char)a;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				char_array[i] = (char)base64_chars.find(char_array[i]);
			}
			result->data.push_back((char_array[0] << 2) + ((char_array[1] & 0x30) >> 4));
			result->data.push_back(((char_array[1] & 0xf) << 4) + ((char_array[2] & 0x3c) >> 2));
			result->data.push_back(((char_array[2] & 0x3) << 6) + char_array[3]);
			i = 0;
		}
	}


	if (i) {
		for (int j = i; j <4; j++)
			char_array[j] = 0;

		for (int j = 0; j < i; j++)
			char_array[j] = (char)base64_chars.find(char_array[j]);

		if (i > 0)
		{
			result->data.push_back((char_array[0] << 2) + ((char_array[1] & 0x30) >> 4));
		}
		if (i > 2)
		{
			result->data.push_back(((char_array[1] & 0xf) << 4) + ((char_array[2] & 0x3c) >> 2));
		}

	}
	
	return move(result);

}

const std::string Base64Decode::get_description() const
{
	return "Base64";
}

bool Base64Encode::accepts_type(DocType type) const
{
	return type == OctetDocumentType;
}

bool Base64Encode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> Base64Encode::get_reverse_transform() const
{
	return std::make_unique<Base64Decode>();
}

std::unique_ptr<Document> Base64Encode::transform(const Document& input) const
{
	if (input.get_type() != OctetDocumentType)
	{
		throw TransformError("Base64 Encoder only accepts octet documents");
	}
	const OctetDocument& doc = dynamic_cast<const OctetDocument&>(input);

	std::unique_ptr<UnicodeDocument> result = std::make_unique<UnicodeDocument>();

	size_t i = 0;
	std::array<char, 3> byte_array;

	for (auto&& a : doc.data)
	{
		byte_array[i++] = a;
		if (i == 3)
		{
			result->data.push_back(base64_chars[(byte_array[0] & 0xfc) >> 2]);
			result->data.push_back(base64_chars[((byte_array[0] & 0x03) << 4) + ((byte_array[1] & 0xf0) >> 4)]);
			result->data.push_back(base64_chars[((byte_array[1] & 0x0f) << 2) + ((byte_array[2] & 0xc0) >> 6)]);
			result->data.push_back(base64_chars[byte_array[2] & 0x3f]);
			i = 0;
		}
	}


	if (i) {
		for (int j = i; j < 3; j++)
		{
			byte_array[j] = 0;
		}

		result->data.push_back(base64_chars[(byte_array[0] & 0xfc) >> 2]);
		result->data.push_back(base64_chars[((byte_array[0] & 0x03) << 4) + ((byte_array[1] & 0xf0) >> 4)]);
		
		if (i == 2)
		{
			result->data.push_back(base64_chars[((byte_array[1] & 0x0f) << 2) + ((byte_array[2] & 0xc0) >> 6)]);
		}

		for(int j = i;j<3;j++)
		{
			result->data.push_back('=');	
		}
	}

	return move(result);
}

const std::string Base64Encode::get_description() const
{
	return "Base64";
}
