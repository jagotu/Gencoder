#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <memory>

#include "utf8.h"

enum DocType { OctetDocumentType, UnicodeDocumentType, MultipartDocumentType };

//Base class for storing data in a specific format that can later be used with transforms
class Document
{
public:
	//Generate preview of the contents that when printed in a fixed-width font is
	//exactly width x height characters.
	virtual std::string generate_preview(size_t width, size_t height) const = 0;

	//Whether this document can be exported to a file and later imported back
	virtual bool is_exportable() const = 0;

	//Export document to the supplied stream
	virtual void do_export(std::ostream& output) const = 0;

	//Import data from the supplied stream
	virtual void do_import(std::istream& input) = 0;

	//Return document format
	virtual DocType get_type() const = 0;

	virtual ~Document() = default;
};

//Document that stores data as a sequence of bytes, without any information
//about their meaning
class OctetDocument : public Document
{
	int get_safe(size_t pos) const;
public:
	std::vector<char> data;
	std::string generate_preview(size_t width, size_t height) const final;
	bool is_exportable() const final;
	void do_export(std::ostream& output) const final;
	void do_import(std::istream& input) final;
	DocType get_type() const final;
};

//Document that stores data as a sequence of unicode codepoints
class UnicodeDocument : public Document
{	
public:
	std::vector<utf8::uint32_t> data;
	std::string generate_preview(size_t width, size_t height) const final;
	bool is_exportable() const final;
	void do_export(std::ostream& output) const final;
	void do_import(std::istream& input) final;
	DocType get_type() const final;
};

//Document that stores multiple documents, each identified by a unicode sequence
class MultipartDocument : public Document
{	
public:
	std::vector < std::pair< std::vector<utf8::uint32_t>, std::unique_ptr<Document> > > data;
	std::string generate_preview(size_t width, size_t height) const final;
	bool is_exportable() const final;
	void do_export(std::ostream& output) const final;
	void do_import(std::istream& input) final;
	DocType get_type() const final;
};