#include "stdafx.h"
#include "mirror.h"
#include "macros"
#include "string_utils.hpp"
#include "stream_collections.hpp"
#include "hash.hpp"

using namespace std;

typedef pair<type_index, type_index> edge_t;
typedef vector<edge_t>::const_iterator edge_iterator_t;
//typedef boost::edge_list<edge_iterator_t> G;



enum vertex_type_index_t { vertex_type_index };
enum edge_conversion_t { edge_conversion };

namespace boost
{
	BOOST_INSTALL_PROPERTY(vertex, type_index);
	BOOST_INSTALL_PROPERTY(edge, conversion);
}




namespace utility
{
	namespace detail
	{
		/*[[deprecated]] vector<any_ptr> to_any_ptr_vec(vector<object_ref> v_)
		{
			vector<any_ptr> result { v_.size() };

			transform(begin(v_), end(v_), begin(result), [](auto& e_)
			{
				return any_ptr { e_.pointer().get(), e_.type() };
			});

			return result;
		}*/


		ostream& operator<<(ostream& os_, const ctor_key_t& obj_)
		{
			return obj_ >> os_;
		}

		ostream& operator<<(ostream& os_, const method_key_t& obj_)
		{
			return obj_ >> os_;
		}

		ostream& operator<<(ostream& os_, const field_key_t& obj_)
		{
			return obj_ >> os_;
		}


		cpp_ctor_key_t::cpp_ctor_key_t(std::vector<std::type_index> vec_tis_)
			: _vec_tis { move(vec_tis_) }
		{
		}

		bool operator==(const cpp_ctor_key_t& a_, const cpp_ctor_key_t& b_)
		{
			return a_._vec_tis == b_._vec_tis;
		}

		cpp_method_key_t::cpp_method_key_t(string name_, vector<type_index> vec_tis_)
			: _name { move(name_) }
			, _vec_tis { move(vec_tis_) }
		{
		}

		bool operator==(const cpp_method_key_t& a_, const cpp_method_key_t& b_)
		{
			return 
				a_._name == b_._name &&
				a_._vec_tis == b_._vec_tis;
		}

		ostream& detail::cpp_method_key_t::operator>>(ostream& os_) const
		{
			return utility::operator<<(os_ << _name, _vec_tis);
		}

		cpp_field_key_t::cpp_field_key_t(string name_)
			: _name{ move(name_) }
		{
		}

		bool operator==(const cpp_field_key_t& a_, const cpp_field_key_t& b_)
		{
			return a_._name == b_._name;
		}

		ostream& detail::cpp_field_key_t::operator>>(ostream& os_) const
		{
			return os_ << _name;
		}
	}

	ostream& detail::cpp_ctor_key_t::operator>>(ostream& os_) const
	{
		return utility::operator<<(os_, _vec_tis);

		//return os_ << _vec_tis;
	}


	//
	//	exceptions
	//
	class_not_found_exception::class_not_found_exception(string msg_)
		: runtime_error { move(msg_) }
	{
	}

	member_not_found_exception::member_not_found_exception(type_index type_, string msg_)
		: runtime_error { _format_message(move(type_), move(msg_)) }
	{
	}

	string member_not_found_exception::_format_message(type_index type_, string member_name_)
	{
		stringstream msg;
		msg << "Type [" << type_.name() << "] doesn't have member with name [" << member_name_ << "].";

		return msg.str();
	}


	//
	//
	//
	mirrored_object::mirrored_object(std::string name_, std::type_index type_)
		: _name { move(name_) }
		, _type { move(type_) }
	{
	}

	string mirrored_object::name() const
	{
		return _name;
	}

	type_index mirrored_object::type() const
	{
		return _type;
	}

	void mirrored_object::set_runtime(const base_runtime_database& runtime_)
	{
		_ptr_runtime = &runtime_;
	}


	//
	class_t::class_t(string name_, type_index ti_)
		: mirrored_object { move(name_), move(ti_) }
	{
	}

	void class_t::set_runtime(const base_runtime_database& runtime_)
	{
		mirrored_object::set_runtime(runtime_);

		_set_runtime();
	}

	type_index class_t::get_polymorphic_type(object_ref obj_) const
	{
		return _poly_type_extractor(move(obj_));
	}


	//
	//
	//
	member_t::member_t(string name_, type_index type_, bool is_static_)
		: mirrored_object { move(name_), move(type_) }
		, _is_static { is_static_ }
	{
	}

	bool member_t::is_static() const
	{
		return _is_static;
	}

	method_like_t::method_like_t(std::string name_, type_index type_, bool is_static_, type_index return_type_, vector<type_index> signature_, std::shared_ptr<method_holder> method_)
		: member_t { move(name_), move(type_), is_static_ }
		, _signature { move(signature_) }
		, _method { move(method_) }
		, _return_type { move(return_type_) }
	{
	}

	std::vector<std::type_index> method_like_t::signature() const
	{
		return _signature;
	}

	//
	//
	//
	object_ref constructor_t::create(std::vector<object_ref> args_) const
	{
		return _method->apply(std::move(args_));
	}

	//
	//
	//
	object_ref method_t::invoke(object_ref obj_, vector<object_ref> args_) const
	{
		if (is_static())
		{
			stringstream ss;
			ss << name() << "was tried to call as a non-static method, although it is.";

			throw std::logic_error{ ss.str().c_str() };
		}

		obj_ = _ptr_runtime->convert_to(obj_, _method->expected_object_type());

		args_.insert(args_.begin(), std::move(obj_));

		return _method->apply(std::move(args_));
	}

	object_ref method_t::invoke_static(vector<object_ref> args_) const
	{
		if(!is_static())
		{
			stringstream ss;
			ss << name() << "was tried to call as a static method, although it isn't.";

			throw std::logic_error { ss.str().c_str() };
		}

		return _method->apply(std::move(args_));
	}
	
	//
	//
	//
	object_ref field_t::value(object_ref obj_) const
	{
		if(is_static())
		{
			stringstream ss;
			ss << name() << "field was tried to accessed as a non-static field, although it is.";

			throw std::logic_error{ ss.str().c_str() };
		}

		auto ptr = dynamic_pointer_cast<field_holder>(_ptr_field);

		auto ref = ptr->value(std::move(obj_));

		return ref;
	}

	object_ref field_t::value() const
	{
		if (!is_static())
		{
			stringstream ss;
			ss << name() << "field was tried to accessed as a non-static field, although it is.";

			throw std::logic_error{ ss.str().c_str() };
		}

		auto ptr = dynamic_pointer_cast<static_field_holder>(_ptr_field);

		return ptr->value();
	}

	void field_t::set_value(object_ref obj_, object_ref value_) const
	{
		if (is_static())
		{
			stringstream ss;
			ss << name() << "field was tried to accessed as a non-static field, although it is.";

			throw std::logic_error{ ss.str().c_str() };
		}

		auto ptr = dynamic_pointer_cast<field_holder>(_ptr_field);

		ptr->set_value(obj_, value_);
	}

	void field_t::set_value(object_ref value_) const
	{
		if (!is_static())
		{
			stringstream ss;
			ss << name() << "field was tried to accessed as a non-static field, although it is.";

			throw std::logic_error{ ss.str().c_str() };
		}

		auto ptr = dynamic_pointer_cast<static_field_holder>(_ptr_field);

		ptr->set_value(value_);
	}

	type_index field_t::held_type() const
	{
		return _ptr_field->held_type();
	}

	//
	//
	//
	type_index class_converter_t::base_type() const 
	{
		return _base_type;
	}

	type_index class_converter_t::derived_type() const
	{
		return _derived_type;
	}

	object_ref class_converter_t::convert(object_ref obj_) const
	{
		return _cast_from.at(obj_.type())(move(obj_));
	}

	std::function<object_ref(object_ref)> class_converter_t::upcast_functor() const
	{
		return _cast_from.at(_derived_type);
	}

	std::function<object_ref(object_ref)> class_converter_t::downcast_functor() const
	{
		return _cast_from.at(_base_type);
	}


	//
	//
	//
	const constructor_t& cpp_class_t::ctor(detail::ctor_key_t&& key_) const
	{

		try
		{
			auto&& key = dynamic_cast<detail::cpp_ctor_key_t&&>(key_);
			return _ctors.at(move(key));
		}
		catch(out_of_range&)
		{
			stringstream ss;
			ss << "constructor[" << key_ << "]";

			throw_with_nested(member_not_found_exception { type(), ss.str() });
		}
	}

	vector<constructor_t> cpp_class_t::ctors() const
	{
		std::vector<constructor_t> result;

		for (const auto& e : _ctors)
		{
			result.push_back(e.second);
		}

		return result;
	}
	
	const method_t& cpp_class_t::method(detail::method_key_t&& key_) const
	{
		try
		{
			auto&& key = dynamic_cast<detail::cpp_method_key_t&&>(key_);

			return _methods.at(move(key));
		}
		catch (out_of_range&)
		{
			auto base_classes = _ptr_runtime->bases_of(type());

			for(auto base_class : base_classes)
			{
				try
				{
					auto base_type = base_class.base_type();
					return _ptr_runtime->get_class(base_type).method(move(key_));
				}
				catch (out_of_range&)
				{
					// method is not present in the either base class
					// No-Op
				}
			}

			stringstream ss;
			ss << key_;

			throw_with_nested(member_not_found_exception { type(), ss.str() });
		}
	}

	vector<method_t> cpp_class_t::methods() const
	{
		std::vector<method_t> result;

		for(const auto& e : _methods)
		{
			result.push_back(e.second);
		}

		//return as_stream(_methods).map([](auto& e_)
		//{
		//	return e_.second;
		//}).as<std::vector>();

		return result;
	}

	vector<field_t> cpp_class_t::fields() const
	{
		vector<field_t> res;

		for(const auto& e: _fields)
		{
			res.push_back(e.second);
		}

		return res;
	}

	const field_t& cpp_class_t::field(detail::field_key_t&& key_) const
	{
		try
		{
			auto&& key = dynamic_cast<detail::cpp_field_key_t&&>(key_);

			return _fields.at(move(key));
		}
		catch (out_of_range&)
		{
			stringstream ss;
			ss << key_;

			throw_with_nested(member_not_found_exception { type(), ss.str() });
		}
	}

	void cpp_class_t::_set_runtime()
	{
		for(auto& e : _methods)
		{
			e.second.set_runtime(*_ptr_runtime);
		}

		for(auto& c : _ctors)
		{
			c.second.set_runtime(*_ptr_runtime);
		}

		for(auto& f : _fields)
		{
			f.second.set_runtime(*_ptr_runtime);
		}
	}

	cpp_class_t::nothing_t cpp_class_t::set_member(method_t method_)
	{
		auto sig = method_.signature();
		auto name = method_.name();

		_methods.insert(
		{
			detail::cpp_method_key_t { move(name), move(sig) },
			move(method_)
		});

		return { };
	}

	cpp_class_t::nothing_t cpp_class_t::set_member(constructor_t ctor_)
	{
		auto sig = ctor_.signature();

		static_assert(is_copy_constructible<constructor_t>::value, "it must be copy constructible");
		static_assert(is_copy_assignable<constructor_t>::value, "it must be copy assignable");

		_ctors.insert({ move(sig), move(ctor_) });

		return { };
	}

	cpp_class_t::nothing_t cpp_class_t::set_member(field_t field_)
	{
		auto name = field_.name();

		_fields.insert({ move(name), move(field_) });

		return { };
	}

	//
	//
	//
	int enum_const_t::integral_value() const
	{
		return _integral_value;
	}

	object_ref enum_const_t::value() const
	{
		return _value;
	}

	//
	//
	//
	enum_t::enum_t(std::string name_, std::type_index type_, std::initializer_list<enum_const_t> consts_)
		: mirrored_object { std::move(name_), std::move(type_) }
	{
		for(auto e : consts_)
		{
			_values.insert({e.integral_value(), e});
		}
	}

	vector<enum_const_t> enum_t::values() const
	{
		vector<enum_const_t> result;

		for(auto e : _values)
		{
			result.push_back(e.second);
		}

		return result;
	}

	const enum_const_t& enum_t::at(int integral_value_) const
	{
		return _values.at(integral_value_);
	}

	enum_t::const_iterator enum_t::find(int integral_value_) const
	{
		return _values.find(integral_value_);
	}

	enum_t::iterator enum_t::begin()
	{
		return _values.begin();
	}

	enum_t::iterator enum_t::end()
	{
		return _values.end();
	}

	enum_t::const_iterator enum_t::begin() const
	{
		return _values.begin();
	}

	enum_t::const_iterator enum_t::end() const
	{
		return _values.end();
	}

	//
	//
	//


	namespace detail
	{
		class base_runtime_database_impl
		{
		public:
			struct type_index_prop
			{
				type_index ti;

				type_index_prop() : ti{ typeid(void) } { }
				type_index_prop(type_index ti_) : ti{ ti_ } { }
			};

			typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
			/* edge properties		*/	type_index_prop,
			/* vertex properties	*/	boost::property<edge_conversion_t, function<object_ref(object_ref)>>
			> G;

			G g;

			typedef boost::property_map<detail::base_runtime_database_impl::G, edge_conversion_t>::type edge_conversion_prop_t;

			std::unordered_map<std::tuple<std::type_index, std::type_index>, std::function<object_ref(object_ref)>> _type_converters;
			unordered_map<type_index, size_t> _type_to_vertex;

			mutable unordered_map<tuple<type_index, type_index, type_index>, int> _cast_offset;

			size_t get_vertex(type_index ti_)
			{
				auto it = _type_to_vertex.find(ti_);

				if(it == _type_to_vertex.end())
				{
					auto v = boost::add_vertex(type_index_prop{ ti_ }, g);
					it = _type_to_vertex.insert({ ti_, v }).first;
				}

				return it->second;
			};

			object_ref do_recursive_cast(object_ref obj_, type_index target_type_) const
			{
				auto it_cast = _cast_offset.find(make_tuple(obj_.type(), obj_.real_type(), target_type_ ));
				if(it_cast != _cast_offset.end() && false)
				{
					void* raw_ptr = obj_.pointer().get();

					int iptr = reinterpret_cast<int>(raw_ptr) +	it_cast->second;
					void* shifted_ptr = reinterpret_cast<void*>(iptr);

					return object_ref
					{
						shared_ptr<void> { obj_.pointer(), shifted_ptr },
						obj_.type(),
						obj_.real_type()
					};
				}


				auto it = _type_to_vertex.find(target_type_);
				auto jt = _type_to_vertex.find(obj_.type());

				if (it != _type_to_vertex.end() && jt != _type_to_vertex.end())
				{
					typedef boost::graph_traits<G>::vertex_descriptor vertex_desc_t;
					typedef boost::graph_traits<G>::vertex_iterator vertex_iterator_t;
					typedef detail::base_runtime_database_impl::type_index_prop type_index_prop;

					vector<vertex_desc_t> trace ( boost::num_vertices(g) );

					static const auto wmap = boost::make_static_property_map<vertex_desc_t>(1);
					auto pmap = boost::make_iterator_property_map(trace.begin(), get(boost::vertex_index, g));

					boost::dijkstra_shortest_paths_no_color_map(g, it->second, boost::predecessor_map(pmap).weight_map(wmap));

					auto casted_obj = do_recursive_cast(obj_, trace, jt->second, it->second);

					int offset = reinterpret_cast<int>(casted_obj.pointer().get()) - reinterpret_cast<int>(obj_.pointer().get());

					_cast_offset[make_tuple(obj_.type(), obj_.real_type(), target_type_)] = offset;

					return casted_obj;
				}

				return { };
			}

			object_ref do_recursive_cast(object_ref ptr_, const vector<size_t>& trace_, size_t vertex_, size_t target_vertex_) const
			{
				if(vertex_ == target_vertex_)
					return ptr_;

				auto next_vertex = trace_[vertex_];

				auto e = boost::edge(vertex_, next_vertex, g);
				assert(e.second);

				boost::property_map<G, edge_conversion_t>::const_type edge_prop = boost::get(edge_conversion, g);

				auto& converter = edge_prop[e.first];

				return do_recursive_cast(converter(ptr_), trace_, next_vertex, target_vertex_);
			}
		};
	}


	base_runtime_database::base_runtime_database() : _ptr_impl { make_unique<detail::base_runtime_database_impl>() }
	{
	}

	base_runtime_database::~base_runtime_database() = default;

	bool base_runtime_database::has_class(string class_name_)
	{
		auto it = _known_classes.find(move(class_name_));

		return it != _known_classes.end();
	}

	bool base_runtime_database::has_class(type_index ti_)
	{
		auto it = _known_classes.find(ti_);

		return it != _known_classes.end();
	}

	class_t& base_runtime_database::get_class(std::string name_) const
	{
		try
		{
			auto& ref =	*_known_classes.at<shared_ptr<class_t>>(name_);
			return ref;
		}
		catch(out_of_range&)
		{
			throw_with_nested(class_not_found_exception { name_ });
		}
	}

	class_t& base_runtime_database::get_class(std::type_index ti_) const
	{
		try
		{
			return *_known_classes.at<shared_ptr<class_t>>(ti_);
		}
		catch (out_of_range&)
		{
			throw_with_nested(class_not_found_exception{ ti_.name() });
		}
	}

	class_t& base_runtime_database::get_polymorph_class(object_ref obj_) const
	{
		/* non-polymorphic class can extract type polymorphic pointer/reference

			class B
			class D : B

			object_ref obj -> shared<B> -> D {}
			obj.as<B> -> D&
			obj.as<D> #error, it was initialized by B, not by D
			* in this case
				D*	-- [safe] -> B*
					-- [can be ok] -> void*
					-- [unsafe: should the ptr be adjusted?] -> D*
		*/
		auto& direct_class = get_class(obj_.type());

		auto polymorphic_type = direct_class.get_polymorphic_type(obj_);

		return get_class(polymorphic_type);
	}

	vector<class_t*> base_runtime_database::classes() const
	{
		vector<class_t*> result;

		// table doesn't have iterator, only const_iterator
		//from(_known_classes).map<class_t*>();

		for(auto& e : _known_classes)
		{
			auto ptr = get<2>(e);

			result.push_back(ptr.get());
		}

		return result;
	}


	struct my_property
	{
		string name;
	};


	object_ref base_runtime_database::convert_to(object_ref obj_, std::type_index target_type_) const
	{
		if(obj_.type() == target_type_)	return obj_;

		return _ptr_impl->do_recursive_cast(obj_, target_type_);
	}

	vector<class_converter_t> base_runtime_database::bases_of(type_index ti_) const
	{
		auto bcs = _base_classes.equal_range(ti_);

		auto ans = from(bcs).map<class_converter_t>([](auto e_)
		{
			return e_.second;
		});

		return ans.operator vector<class_converter_t>();
	}

	base_runtime_database& base_runtime_database::add(shared_ptr<class_t> class_)
	{
		auto name = class_->name();
		auto type = class_->type();

		class_->set_runtime(*this);

		_known_classes.insert(move(name), move(type), move(class_));

		return *this;
	}

	base_runtime_database& base_runtime_database::add(std::shared_ptr<enum_t> enum_)
	{
		auto name = enum_->name();
		auto type = enum_->type();

		_known_enums.insert(move(name), move(type), move(enum_));

		return *this;
	}

	base_runtime_database& base_runtime_database::add(class_converter_t base_class_)
	{
		auto dt = base_class_.derived_type();
		auto bt = base_class_.base_type();

		_derived_classes.insert({ move(bt), base_class_ });
		_base_classes.insert({ move(dt), base_class_ });

		_ptr_impl->_type_converters.insert({ make_tuple(dt, bt), base_class_.upcast_functor() });
		_ptr_impl->_type_converters.insert({ make_tuple(bt, dt), base_class_.downcast_functor() });

		auto u = _ptr_impl->get_vertex(dt);
		auto v = _ptr_impl->get_vertex(bt);

		using namespace detail;
		//base_runtime_database_impl::edge_conversion_prop_t edge_conv_prop = boost::get(edge_conversion, _ptr_impl->g);

		boost::property_map<base_runtime_database_impl::G, edge_conversion_t>::type edge_prop = boost::get(edge_conversion, _ptr_impl->g);

		auto e = boost::add_edge(u, v, _ptr_impl->g);
		assert(e.second);
		edge_prop[e.first] = base_class_.upcast_functor();

		auto f = boost::add_edge(v, u, _ptr_impl->g);
		assert(f.second);
		edge_prop[f.first] = base_class_.downcast_functor();

		return *this;
	}

	enum_t& base_runtime_database::enums(std::string enum_name_) const
	{
		try
		{
			return *_known_enums.at<shared_ptr<enum_t>>(enum_name_);
		}
		catch (out_of_range&)
		{
			throw_with_nested(class_not_found_exception{ enum_name_ });
		}
	}

	enum_t& base_runtime_database::enums(std::type_index type_) const
	{
		try
		{
			return *_known_enums.at<shared_ptr<enum_t>>(type_);
		}
		catch (out_of_range&)
		{
			throw_with_nested(class_not_found_exception{ type_.name() });
		}
	}

	std::vector<enum_t*> base_runtime_database::enums() const
	{
		vector<enum_t*> result;

		for (auto& e : _known_enums)
		{
			auto ptr = get<2>(e);

			result.push_back(ptr.get());
		}

		return result;
	}
}
