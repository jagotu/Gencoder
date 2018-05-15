#pragma once

#include "../transform.h"

//Implements decoding UTF-8 encoded unicode documents
class UTF8Decode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};

//Implements encoding unicode documents using UTF-8
class UTF8Encode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};