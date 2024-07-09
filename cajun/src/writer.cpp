/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. 

***********************************************/

#include "stdafx.h"

#include "../include/writer.h"
#include <iostream>


/*  

TODO:
* better documentation
* unicode character encoding

*/

namespace json
{


void Writer::Write(const Element& elementRoot, std::ostream& ostr) { 
   Writer writer(ostr);
   elementRoot.Accept(writer);
   ostr.flush(); // all done
}

void Writer::Visit(const Array& array) {
   if (array.Empty())
      m_ostr << "[]";
   else
   {
      m_ostr << '[' << std::endl;
      ++m_nTabDepth;

      Array::const_iterator it(array.Begin()),
                            itEnd(array.End());
      while (it != itEnd) {
         m_ostr << std::string(m_nTabDepth, '\t');
         it->Accept(*this); 

         if (++it != itEnd)
            m_ostr << ',';
         m_ostr << std::endl;
      }

      --m_nTabDepth;
      m_ostr << std::string(m_nTabDepth, '\t') << ']';
   }
}

void Writer::Visit(const Object& object) {
   if (object.Empty())
      m_ostr << "{}";
   else
   {
      m_ostr << '{' << std::endl;
      ++m_nTabDepth;

      Object::const_iterator it(object.Begin()),
                             itEnd(object.End());
      while (it != itEnd) {
         m_ostr << std::string(m_nTabDepth, '\t') << '"' << it->name << "\" : ";
         it->element.Accept(*this); 

         if (++it != itEnd)
            m_ostr << ',';
         m_ostr << std::endl;
      }

      --m_nTabDepth;
      m_ostr << std::string(m_nTabDepth, '\t') << '}';
   }
}

void Writer::Visit(const Number& number) {
   m_ostr << number;
}

void Writer::Visit(const Uint64& number) {
    m_ostr << number;
}

void Writer::Visit(const Boolean& booleanElement) {
   m_ostr << (booleanElement ? "true" : "false");
}

void Writer::Visit(const Null& nullElement) {
   m_ostr << "null";
}

void Writer::Visit(const String& stringElement) {
   m_ostr << '"';

   const std::string& s = stringElement;
   
   std::string::const_iterator it(s.begin()),
                               itEnd(s.end());
   for (; it != itEnd; ++it)
   {
      switch (*it)
      {
         case '"':         m_ostr << "\\\"";    break;
         case '\\':        m_ostr << "\\\\";    break;
         case '\b':        m_ostr << "\\b";    break;
         case '\f':        m_ostr << "\\f";    break;
         case '\n':        m_ostr << "\\n";    break;
         case '\r':        m_ostr << "\\r";    break;
         case '\t':        m_ostr << "\\t";    break;
         //case '\u':        m_ostr << "";    break;  ??
         default:          m_ostr << *it;       break;
      }
   }

   m_ostr << '"';   
}


} // End namespace
