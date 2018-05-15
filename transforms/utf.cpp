#include "utf.h"

bool UTF8Decode::accepts_type(DocType type) const
{
	return type == OctetDocumentType;
}

bool UTF8Decode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> UTF8Decode::get_reverse_transform() const
{
	return std::make_unique<UTF8Encode>();
}

std::unique_ptr<Document> UTF8Decode::transform(const Document& input) const
{
	if (input.get_type() != OctetDocumentType)
	{
		throw TransformError("UTF-8 Decoder only accepts octet documents");
	}
	const OctetDocument& doc = dynamic_cast<const OctetDocument&>(input);

	std::unique_ptr<UnicodeDocument> result = std::make_unique<UnicodeDocument>();
	

	try {
		for (auto it = doc.data.begin(); it != doc.data.end();)
		{
			result->data.push_back(utf8::next(it, doc.data.end()));
		}
	}
	catch (const utf8::invalid_code_point &)
	{
		throw TransformError("UTF-8 Decoder encountered an invalid code point");
	}
	catch (const utf8::not_enough_room &)
	{
		throw TransformError("End of file in the middle of UTF-8 char");
	}
	catch (const utf8::invalid_utf8 &)
	{
		throw TransformError("Input is not a valid UTF-8 document");
	}

	return move(result);
	
}

const std::string UTF8Decode::get_description() const
{
	return "UTF-8";
}

bool UTF8Encode::accepts_type(DocType type) const
{
	return type == UnicodeDocumentType;
}

bool UTF8Encode::reverse_transform() const
{
	return true;
}

std::unique_ptr<Transform> UTF8Encode::get_reverse_transform() const
{
	return std::make_unique<UTF8Decode>();
}

std::unique_ptr<Document> UTF8Encode::transform(const Document& input) const
{
	if (input.get_type() != UnicodeDocumentType)
	{
		throw TransformError("UTF-8 Encoder only accepts unicode documents");
	}
	const UnicodeDocument& doc = dynamic_cast<const UnicodeDocument&>(input);

	std::unique_ptr<OctetDocument> result = std::make_unique<OctetDocument>();


	auto inserter =  std::back_inserter(result->data);

	try {
		for (auto&& cp : doc.data)
		{
			utf8::append(cp, inserter);
		}
	}
	catch (utf8::invalid_code_point)
	{
		throw TransformError("UTF-8 Encoder encountered an invalid code point");
	}

	return move(result);
}

const std::string UTF8Encode::get_description() const
{
	return "UTF-8";
}
