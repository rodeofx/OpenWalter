#ifndef TO_STRING_PATCH_
#define TO_STRING_PATCH_

#include <sstream>

template <typename T> std::string to_string( T Number )
{
	std::stringstream ss;
	ss << Number;
	return ss.str();
}


#endif //TO_STRING_PATCH_
#ifndef TO_STRING_PATCH_
#define TO_STRING_PATCH_

#include <sstream>

template <typename T> std::string to_string( T Number )
{
	std::stringstream ss;
	ss << Number;
	return ss.str();
}


#endif //TO_STRING_PATCH_
