#include "stdafx.h"
#include "inode"
#include "reactive_context7.h"


namespace reactive_framework7
{
	namespace detail
	{
		std::atomic<int> inode::_id_counter{ 0 };

		inode::inode(ireactive_context& rc_)
			: _rc { rc_ }
		{
		}

		inode::~inode()
		{
			_rc.release_rv(*this);
		}

		int inode::id() const
		{
			return _id;
		}
	}
}
