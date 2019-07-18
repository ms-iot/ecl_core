/**
 * @file /src/lib/thread_win.cpp
 *
 * @brief Win32 thread implementation.
 *
 * @date April 2013
 **/
/*****************************************************************************
** Platform Check
*****************************************************************************/

#include <ecl/config/ecl.hpp>
#if defined(ECL_IS_WIN32)

/*****************************************************************************
** Includes
*****************************************************************************/

#include <windows.h>

#include <iostream>
#include "../../include/ecl/threads/thread_win.hpp"

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace ecl {

/*****************************************************************************
* Thread Class Methods
*****************************************************************************/

Thread::Thread(VoidFunction function, const Priority &priority, const long &stack_size)
{
	start(function, priority, stack_size);
}

Error Thread::start(VoidFunction function, const Priority &priority, const long &stack_size)
{
	if ( has_started ) {
		ecl_debug_throw(StandardException(LOC,BusyError,"The thread has already been started."));
		return Error(BusyError); // if in release mode, gracefully fall back to return values.
	} else {
		has_started = true;
	}

	NullaryFreeFunction<void> nullary_function_object = generateFunctionObject(function);
	thread_task = new threads::ThreadTask< NullaryFreeFunction<void> >(nullary_function_object, priority);
    return initialise(threads::ThreadTask< NullaryFreeFunction<void> >::EntryPoint, priority, stack_size);
}

Thread::~Thread() {
	cancel();
}

void Thread::cancel() {
	if (thread_handle) {
		unsigned long exitcode;
		if (::GetExitCodeThread(thread_handle, &exitcode)) {
			if (exitcode == 0x0103) {
				// unsafe termination.
				::TerminateThread(thread_handle, exitcode);
			}
		}
		::CloseHandle(thread_handle);
		thread_handle = NULL;
	}
	if (thread_task) {
		delete thread_task;
		thread_task = NULL;
	}
	has_started = false;
	join_requested = false;
}

void Thread::join() {
	join_requested = true;

	if (thread_handle) {
		WaitForSingleObject(thread_handle, INFINITE);
	}
}

Error Thread::initialise(const entryPointFunc& entryPoint, const Priority& priority, const long&) {
    DWORD threadid;

    auto thread = ::CreateThread(
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(entryPoint),
        thread_task,
        0,
        &threadid);
    thread_handle = reinterpret_cast<void*>(thread);

    if (!thread_handle) {
        ecl_debug_throw(StandardException(LOC, UnknownError, "Failed to create thread."));
        return Error(UnknownError);
    }

    BOOL bResult = FALSE;

    if (priority >= RealTimePriority1) {
        bResult = ::SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
    }

    switch (priority) {
        case CriticalPriority:
            bResult = ::SetThreadPriority(thread, HIGH_PRIORITY_CLASS);
            break;
        case HighPriority:
            bResult = ::SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);
            break;
        case LowPriority:
            bResult = ::SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
            break;
        case BackgroundPriority:
            bResult = ::SetThreadPriority(thread, THREAD_PRIORITY_IDLE);
            break;
        default:
            break;
    }

    if (!bResult) {
        ecl_debug_throw(threads::throwPriorityException(LOC));
    }
    return Error(NoError);
}

}; // namespace ecl

#endif /* ECL_IS_WIN32 */
