#include "document.h"

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "transform.h"
#include "utf8.h"
#include "utf8_charclass.h"


int OctetDocument::get_safe(size_t pos) const
{
	if (pos >= data.size())
	{
		return -1;
	}
	return (unsigned char)data[pos];
}

std::string OctetDocument::generate_preview(size_t width, size_t height) const
{
	size_t bytes_on_line = (width - 3) / 4;

	std::ostringstream ss;

	if (bytes_on_line == 0)
	{
		for (size_t i = 0; i < height; i++)
		{
			ss << std::endl;
		}
	}

	for (size_t linestart = 0; linestart < height * bytes_on_line; linestart += bytes_on_line)
	{
		for (size_t pos = linestart; pos < linestart + bytes_on_line; pos++)
		{
			int data = get_safe(pos);
			if (data < 0)
			{
				ss << "   ";
			}
			else {
				ss << std::hex << std::setw(2) << std::setfill('0') << data << ' ';
			}
		}

		ss << "| ";

		for (size_t pos = linestart; pos < linestart + bytes_on_line; pos++)
		{
			int data = get_safe(pos);
			if (data < 0)
			{
				ss << ' ';
			}
			else if (!isprint(data))
			{
				ss << '.';
			}
			else {
				ss << (char)data;
			}
		}

		ss << std::endl;
	}

	return ss.str();

}

bool OctetDocument::is_exportable() const
{
	return false;
}

void OctetDocument::do_export(std::ostream & output) const
{
	output.write(&data[0], data.size());
}

void OctetDocument::do_import(std::istream & input)
{
	data.clear();
	data.insert(data.begin(), std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

DocType OctetDocument::get_type() const
{
	return OctetDocumentType;
}

std::string UnicodeDocument::generate_preview(size_t width, size_t height) const
{
	std::string s;
	width--;
	size_t line = 0, col = 0;
	bool last_was_cr = false;
	for (auto&& a : data)
	{
		if (col >= width)
		{
			s += '\n';
			col = 0;
			line++;
		}
		if (line >= height)
			break;
		if (utf8::is_newline(a))
		{
			if (last_was_cr)
			{
				continue;
			}
			if (a == 0xD)
			{
				last_was_cr = true;
			}
			else {
				last_was_cr = false;
			}
			s += '\n';
			col = 0;
			line++;
			continue;
		}
		utf8::append(a, std::back_inserter(s));
		if (a == 0x9)
		{
			//it's a tabstop
			col += 8 - (col % 8);
		}
		else {
			col++;
		}
	}
	for (size_t i = line; i < height; i++)
	{
		s += '\n';
	}

	return s;
}

bool UnicodeDocument::is_exportable() const
{
	return true;
}

void UnicodeDocument::do_export(std::ostream & output) const
{
	std::ostream_iterator<char> outit(output);
	for (auto&& a : data)
	{
		utf8::append(a, outit);
	}
}

void UnicodeDocument::do_import(std::istream & input)
{
	data.clear();
	std::istreambuf_iterator<char> it(input.rdbuf());
	std::istreambuf_iterator<char> eos;

	try {
		for (; it != eos;)
		{
			data.push_back(utf8::next(it, eos));
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
}

DocType UnicodeDocument::get_type() const
{
	return UnicodeDocumentType;
}

std::string MultipartDocument::generate_preview(size_t width, size_t height) const
{
	std::string s;

	size_t line = 0;

	for (auto&& item : data)
	{
		if (line > height)
			break;
		
		//This assumes that all code-points are at most one char wide when printed to console.
		size_t lentoprint = std::min(item.first.size(), width-1);
		for (size_t i = 0; i < lentoprint; i++)
		{
			utf8::append(item.first[i], std::back_inserter(s));
		}
		
		if (width - lentoprint > 3)
		{
			s += ": ";
			s += item.second->generate_preview(width - lentoprint - 2, 1);
		}
		else {
			s += "\n";
		}
		line++;
	}
	return s;
}

bool MultipartDocument::is_exportable() const
{
	return false;
}

void MultipartDocument::do_export(std::ostream & output) const
{
	throw std::logic_error("Can't export a MultipartDocument");
}

void MultipartDocument::do_import(std::istream & input)
{
	throw std::logic_error("Can't import a MultipartDocument");
}

DocType MultipartDocument::get_type() const
{
	return MultipartDocumentType;
}
