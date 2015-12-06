#pragma once

namespace reactive_framework7
{
	namespace detail
	{
		class inode;

		struct ireactive_context
		{
			virtual void on_value_holder_changed(inode&) = 0;

			virtual void release_rv(const inode&) = 0;
		};
	}
}
