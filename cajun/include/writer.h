/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. 

***********************************************/

#pragma once

#include "elements.h"
#include "visitor.h"

namespace json
{


class Writer : private ConstVisitor
{
public:
   static void Write(const Element& elementRoot, std::ostream& ostr);
      
private:
   Writer(std::ostream& ostr) :
      m_ostr(ostr),
      m_nTabDepth(0) {}

   virtual void Visit(const Array& array);
   virtual void Visit(const Object& object);
   virtual void Visit(const Number& number);
   virtual void Visit(const String& string);
   virtual void Visit(const Boolean& boolean);
   virtual void Visit(const Null& null);

   std::ostream& m_ostr;
   int m_nTabDepth;
};


} // End namespace