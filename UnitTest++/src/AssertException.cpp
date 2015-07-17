#include "AssertException.h"
#include <cstring>

#ifndef WIN32
#define strcpy_s(_buffer, _cbuffer, _source)   strcpy((_buffer), (_source))
#endif

namespace UnitTest {

AssertException::AssertException(char const* description, char const* filename, int lineNumber)
    : m_lineNumber(lineNumber)
{
    using namespace std;

    strcpy_s(m_description, sizeof(m_description), description);
    strcpy_s(m_filename,    sizeof(m_filename),    filename);
}

AssertException::~AssertException() throw()
{
}

char const* AssertException::what() const throw()
{
    return m_description;
}

char const* AssertException::Filename() const
{
    return m_filename;
}

int AssertException::LineNumber() const
{
    return m_lineNumber;
}

}
