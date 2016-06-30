#pragma once

namespace utility
{
	typedef signed char sbyte;
	typedef unsigned char byte;

	typedef unsigned int uint;

	typedef sbyte					int8;
	typedef signed short int		int16;
	typedef int						int32;
	typedef signed long long int	int64;

	typedef byte					uint8;
	typedef unsigned short int		uint16;
	typedef uint					uint32;
	typedef unsigned long long int	uint64;


	static_assert(sizeof(uint16) == 2, "size of type uint16 must be 2 bytes");
	static_assert(sizeof(uint32) == 4, "size of type uint32 must be 4 bytes");
	static_assert(sizeof(uint64) == 8, "size of type uint64 must be 8 bytes");

	static_assert(sizeof(int64) == 8, "size of type int64 must be 8 bytes");
}
