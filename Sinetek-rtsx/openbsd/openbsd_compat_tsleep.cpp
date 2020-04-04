#include "openbsd_compat_tsleep.h"

#include <sys/errno.h>

#define UTL_THIS_CLASS ""
#include "util.h"

extern int hz;
int lbolt; // TODO: Need to implement a 1-second timer here, but it's not worth the effort (probably unused).

// we need to implement these two in terms of msleep, which is what macOS provides...
// See: https://github.com/apple/darwin-xnu/blob/master/bsd/kern/kern_synch.c

// WARNING: The kernel will crash if msleep is called while a IOSimpleLock is held!

// will sleep at most timo/hz seconds
int tsleep(void *ident, int priority, const char *wmesg, int timo)
{
	struct timespec ts = { 0, 0 };
	if (timo != 0) {
		uint64_t nsecs = (uint64_t) timo * NS_PER_SEC / hz;
		ts.tv_sec = nsecs / NS_PER_SEC;
		ts.tv_nsec = nsecs % NS_PER_SEC;
	}
	UTL_DEBUG(4, "%s: Calling msleep(%d/%d) (this may crash if lock is held)...", wmesg, (int) ts.tv_sec, (int) ts.tv_nsec);
	auto ret = msleep(ident, nullptr, priority, wmesg, &ts);
	UTL_DEBUG(4, "%s: msleep returned %d", wmesg, ret);
	return ret;
}

int tsleep_nsec(void *ident, int priority, const char *wmesg, uint64_t nsecs)
{
	struct timespec ts = { 0, 0 };
	if (nsecs != 0) {
		ts.tv_sec = nsecs / NS_PER_SEC;
		ts.tv_nsec = nsecs % NS_PER_SEC;
	}
	UTL_DEBUG(4, "%s: Calling msleep(%d/%d) (this may crash if lock is held)...", wmesg, (int) ts.tv_sec, (int) ts.tv_nsec);
	auto ret = msleep(ident, nullptr, priority, wmesg, &ts);
	UTL_DEBUG(4, "%s: msleep returned %d", wmesg, ret);
	return ret;
}
