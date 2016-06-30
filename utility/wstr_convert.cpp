#include "stdafx.h"
#include "wstr_convert"




std::wstring utility::to_wstring(std::string str_)
{
	using namespace std;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conversion;

	return conversion.from_bytes(str_);
}
