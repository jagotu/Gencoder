#pragma once

#include "../transform.h"

//This class implements decoding of application/x-www-form-urlencoded serialized form data
//as described in XForms W3C Recommendation (https://www.w3.org/TR/xforms/)
class xwwwformurlencodedDecode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};

//This class implements serialization of form data to application/x-www-form-urlencoded
//as described in XForms W3C Recommendation (https://www.w3.org/TR/xforms/)
class xwwwformurlencodedEncode : public Transform {
public:
	bool accepts_type(DocType type) const final;
	bool reverse_transform() const final;
	std::unique_ptr<Transform> get_reverse_transform() const final;
	std::unique_ptr<Document> transform(const Document& input) const final;
	const std::string get_description() const final;
};	