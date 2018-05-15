#pragma once

#include "document.h"


enum TransformType {EncodeTransformType, DecodeTransformType, NoneTransformType};

class Transform
{
public:
	//Returns whether this transformation can convert from the type
	virtual bool accepts_type(DocType type) const = 0;

	//Returns whether this transformation can be reversed
	//If this returns false, this transform is ignored when reverting
	virtual bool reverse_transform() const = 0;

	//Returns the transformation that reverts effects on this one
	virtual std::unique_ptr<Transform> get_reverse_transform() const = 0;

	//Returns a document created by transformin the input
	virtual std::unique_ptr<Document> transform(const Document& input) const = 0;

	//Returns a description of this transformation
	virtual const std::string get_description() const = 0;
};

class TransformError : public std::runtime_error {
public:
	TransformError(const std::string what) : runtime_error(what)
	{

	}
};	

#include "transforms/utf.h"
#include "transforms/b64.h"
#include "transforms/url.h"
