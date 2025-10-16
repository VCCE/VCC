#pragma once
#include <vcc/core/detail/exports.h>
#include <Windows.h>


namespace vcc { namespace core { namespace utils
{

	class critical_section
	{
	public:

		critical_section()
		{
			InitializeCriticalSection(&section_);
		}

		~critical_section()
		{
			DeleteCriticalSection(&section_);
		}

		critical_section(const critical_section&) = delete;
		critical_section& operator=(const critical_section&) = delete;

		void lock()
		{
			EnterCriticalSection(&section_);
		}

		void unlock()
		{
			LeaveCriticalSection(&section_);
		}


	private:

		CRITICAL_SECTION	section_;
	};


	class section_locker
	{
	public:

		explicit section_locker(critical_section& section)
			: section_(section)
		{
			section_.lock();
		}

		~section_locker()
		{
			section_.unlock();
		}

		section_locker& operator=(const section_locker&) = delete;
		section_locker& operator=(section_locker&&) = delete;


	private:

		critical_section& section_;
	};

} } }
