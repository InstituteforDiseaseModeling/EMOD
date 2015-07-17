/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. See class headers for additional
info.

***********************************************/

#pragma once

#include <stdexcept> // DJM - seems to be needed by gcc  
#include "reader.h"
namespace json
{

class Exception : public std::runtime_error
{
public:
   Exception(const std::string& sMessage) :
      std::runtime_error(sMessage) {}
};



class ScanException : public json::Exception
{
public:
   ScanException(const std::string& sMessage, const Reader::Location& locError) :
      Exception(sMessage),
      m_locError(locError) {}

   Reader::Location m_locError;
};

class ParseException : public Exception
{
public:
   ParseException(const std::string& sMessage, const Reader::Location& locTokenBegin, const Reader::Location& locTokenEnd) :
      Exception(sMessage),
      m_locTokenBegin(locTokenBegin),
      m_locTokenEnd(locTokenEnd) {}

   Reader::Location m_locTokenBegin;
   Reader::Location m_locTokenEnd;
};

} // End namespace
