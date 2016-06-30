#pragma once
#include <unordered_map>
#include "meta.hpp"

namespace utility
{
	namespace detail
	{
		template<class T> class table_component
		{
		protected:
			size_t _at_index(T t_) const
			{
				using namespace std;

				auto index = _to_common_idx.at(std::move(t_));

				return index;
			}

			void _insert(size_t i_, T t_)
			{
				_to_common_idx[t_] = i_;
			}

			auto _begin() const
			{
				return _to_common_idx.begin();
			}

			auto _end() const
			{
				return _to_common_idx.end();
			}

			auto _find(T t_) const
			{
				return _to_common_idx.find(t_);
			}

		private:
			std::unordered_map<T, size_t> _to_common_idx;
		};
	}

	template<class... Ts> class table : protected detail::table_component<Ts> ...
	{
		template<class T> using base_t = detail::table_component<T>;
	public:

		class const_iterator
		{
		public:
			const_iterator(const table& owner_, size_t i_) : _owner {owner_}, _i { i_ }
			{
			}

			const std::tuple<Ts...>& operator*() const
			{
				return _owner._common_storage[_i];
			}

			const_iterator& operator++()
			{
				++_i;

				return *this;
			}

			friend bool operator==(const const_iterator& lhs_, const const_iterator& rhs_)
			{
				return lhs_._i == rhs_._i;
			}

			friend bool operator!=(const const_iterator& lhs_, const const_iterator& rhs_)
			{
				return lhs_._i != rhs_._i;
			}

		private:
			const table& _owner;
			size_t _i;
		};

		std::pair<const_iterator, bool> insert(Ts... args_)
		{
			using namespace std;

			const_iterator found_keys[] { find<Ts>(args_) ... };

			auto it_of_array = std::find_if(std::begin(found_keys), std::end(found_keys),
				[this](auto e_)
				{
					return e_ != end();
				});

			if (it_of_array == std::end(found_keys))
			{

				size_t id = _common_storage.size();
			
				_insert_impl(id, args_...);

				_common_storage.push_back(make_tuple(std::forward<Ts>(args_)...));

				return make_pair(const_iterator { *this, id }, true);
			}

			return make_pair(*it_of_array, false);
		}

		template<class R, class T> R at(T t_) const
		{
			using namespace std;

			typedef std::decay_t<R> decayed_R;
			
			static_assert(utility::is_any_of<T, Ts...>::value, "The argument type of at must be either of the contained type of this container.");
			static_assert(utility::is_any_of<decayed_R, Ts...>::value, "The output type of at must be either of the contained type of this container.");

			int index = base_t<T>::_at_index(std::move(t_));

			auto& tpl = _common_storage[index];

			return std::get<R>(tpl);
		}

		template<class T> auto find(T t_) const
		{
			auto it = base_t<T>::_find(std::move(t_));

			if(it == base_t<T>::_end())
				return end();

			return const_iterator{ *this, it->second };
		}

		auto begin() const
		{
			return const_iterator{ *this, 0 };
		}

		auto end() const
		{
			return const_iterator{ *this, _common_storage.size() };
		}

	private:
		
		std::vector<std::tuple<Ts...>> _common_storage;

		template<class T> inline void _insert_impl(size_t i_, T t_)
		{
			base_t<T>::_insert(i_, t_);
		}

		template<class T, class... Ts> void _insert_impl(size_t i_, T t_, Ts... ts_)
		{
			base_t<T>::_insert(i_, t_);
			_insert_impl(i_, ts_...);
		}
	};
}
