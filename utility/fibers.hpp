#pragma once
#include <windows.h>
#include <thread>
#include <unordered_map>
#include <functional>

namespace utility
{

	class fiber_context;

	class fiber
	{
	public:
		virtual ~fiber() = default;

		void switch_to();

	protected:
		LPVOID _this_fiber;
		fiber_context& _master;

		fiber(fiber_context& master_fiber_, LPVOID this_fiber_);

		fiber(const fiber&) = delete;
		fiber(fiber&& other_);
	};

	class secondary_fiber : public fiber
	{
	public:
		secondary_fiber(fiber_context& master_fiber_);

		secondary_fiber(fiber_context& master_fiber_, std::function<void()> handler_);

		secondary_fiber(const secondary_fiber&) = delete;

		secondary_fiber(secondary_fiber&& other_);

		~secondary_fiber() override;

		void reset_handler(std::function<void()> handler_);

	private:
		std::function<void()> _handler;

		static void __stdcall _dispatcher(LPVOID parameter_);

		void _dispatcher();
	};

	class primary_fiber : public fiber
	{
	public:
		primary_fiber(fiber_context& master_fiber_);

		primary_fiber(const primary_fiber&) = delete;

		primary_fiber(primary_fiber&& other_);

		~primary_fiber() override;
	};


	class fiber_context
	{
	public:
		fiber_context() = default;

		fiber_context(const fiber_context&) = delete;

		void switch_to(int i_);

		void switch_to_primary();

		bool erase(int i_);

		fiber& get_or_create(int i_, std::function<void()> func_);

		secondary_fiber& get_or_create_secondary(int i_, std::function<void()> func_);

		secondary_fiber& get_or_create_secondary(int i_);

		fiber& get_or_create(int i_);

	private:
		primary_fiber _primary_fiber { *this };
		std::unordered_map<int, secondary_fiber> _slave_fibers;
	};
}