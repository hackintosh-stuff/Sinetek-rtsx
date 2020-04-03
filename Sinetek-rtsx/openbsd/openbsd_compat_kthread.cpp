#include "openbsd_compat_kthread.h"

#include <sys/errno.h> // E*
#include <sys/types.h> // struct proc
#include <kern/thread.h> // kernel_thread_start

#define UTL_THIS_CLASS ""
#include "util.h"

struct MyArgStruct {
	// OpenBSD parameters
	void (*func)(void *);
	void *arg;

	// macOS thread
	thread_t thread;
};

static void my_thread_continue(void *arg, wait_result_t wait_result)
{
	struct MyArgStruct *myArg = (struct MyArgStruct *) arg;

	UTL_CHK_PTR(myArg,);

	UTL_DEBUG(1, "Thred created (wait_result=%d), calling OpenBSD function...", (int) wait_result);
	// Call OpenBSD thread
	myArg->func(myArg->arg);
	UTL_LOG("OpenBSD thread function returned!");
}

int
kthread_create(void (*func)(void *), void *arg, struct proc **newpp, const char *name)
{
	int ret;

	UTL_DEBUG(1, "Creating new thread (name=%s)...", name);
	auto myArg = UTL_MALLOC(struct MyArgStruct); // TODO: Where do we free this?
	UTL_CHK_PTR(myArg, ENOMEM);

	myArg->func = func;
	myArg->arg = arg;

	ret = kernel_thread_start(my_thread_continue, myArg, &myArg->thread);
	if (ret) {
		UTL_ERR("Error %d creating thread!", ret);
	}
	// set thread name
	thread_set_thread_name(myArg->thread, name);
	if (newpp)
		*newpp = (proc *) myArg->thread;
	return 0;
}

void
kthread_create_deferred(void (*func)(void *), void *arg)
{
	// just assume we can create the thread any time...
	func(arg);
}

void
kthread_exit(int ecode)
{
	// TODO: delete myArg here??
}
