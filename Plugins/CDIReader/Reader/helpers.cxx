#include "helpers.h"
#include <sstream>

namespace helpers
{

std::string GetPathName(const std::string& s)
{
  char sep = '/';
#ifdef _WIN32
  sep = '\\';
#endif

  size_t i = s.rfind(sep, s.length());
  if (i != std::string::npos)
  {
    return (s.substr(0, i));
  }

  return ("");
}

std::string GetBasenameFromUri(const std::string& s)
{
  const char sep = '/';
  size_t i = s.rfind(sep, s.length());
  if (i != std::string::npos)
  {
    return (s.substr(i + 1));
  }
  std::string res = s;
  return (res);
}

bool CheckFileAccess(const std::string& name)
{
  if (FILE* file = fopen(name.c_str(), "r"))
  {
    fclose(file);
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
//  Function to convert cartesian coordinates to spherical, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------

// Strip leading and trailing punctuation
// http://answers.yahoo.com/question/index?qid=20081226034047AAzf8lj
void Strip(std::string& s)
{
  std::string::iterator i = s.begin();
  while (ispunct(*i))
    s.erase(i);
  std::string::reverse_iterator j = s.rbegin();
  while (ispunct(*j))
  {
    s.resize(s.length() - 1);
    j = s.rbegin();
  }
}

std::string ConvertInt(int number)
{
  std::stringstream ss;
  ss << number;
  return ss.str();
}

}; // end of namespace helpers
