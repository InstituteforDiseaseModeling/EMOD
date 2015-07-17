/******************************************************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

functions: json_cast

json_cast is intended as a alternatives to static_cast and dynamic_cast. The
symantics are identical: one can cast a const/nonconst poitner/reference, and 
all conventional rules regarding casting are preserved. It is similar to 
static_cast in that it simply returns a "reinterpretation" of the pointer or
reference passed in. However, it is closer in functionality to dynamic_cast,
in that it verifies the operation is legal, and fails otherwise (returns null
during pointer cast, throws json::Exception during reference cast). Examples:

Number num = Number();
const Element& el = num; // implicit cast

json_cast<const Number&>(el); // runtime success
json_cast<const Array&>(el); // runtime exception

json_cast<Number&>(el); // compile time failure (loses const qualifier)
json_cast<const Number*>(el); // compile time failure (cannot convert ref to pointer)

******************************************************************************/

#pragma once


namespace json
{


class Element;

// reference to non-const
template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(Element& element);

// reference to const
template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(const Element& element);

// pointer to non-const
template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(Element* pElement);

// pointer to const
template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(const Element* pElement);

} // End namespace

#include "cast.inl"

