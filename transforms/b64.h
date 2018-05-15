#pragma once

#include "../transform.h"

//This class implements decoding of Base64 encoded-data
//It should support most of the variants mentioned in https://en.wikipedia.org/wiki/Base64#Variants_summary_table:
//* = padding isn't mandatory
//* ignores all whitespace
//*	rejects any non-base64 chars (breaks RFC 2045 but seems good practice to me)
class Base64Decode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};

//This class implements encoding to Base64
//Variant used:
//* = padding is added
//* linebreaks are not added
class Base64Encode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};