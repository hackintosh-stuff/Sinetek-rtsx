#include "openbsd_compat_spl.h"

#include <machine/machine_routines.h> // FALSE

#define UTL_THIS_CLASS ""
#include "util.h"

// One alternative was to use the function ml_set_interrupts_enabled() (See:
// https://github.com/apple/darwin-xnu/blob/master/osfmk/kern/spl.h), but blocking interrupts seems too drastic.
// It is better to implement a "global lock" for the OpenBSD-based code to simulate the same behavior.

// For now, I opted for an IOSimpleLock, but maybe an IORecursiveLock is better. The problem is that the BSD code
// (sdmmc.c) calls tsleep_nsec (implemented using msleep) while holding splbio. In macOS, it seems that calling mswait
// while holding an IOSimpleLock (a spin-lock) will crash the kernel. For this reason, we need to modify the BSD code
// slightly to release the lock right before calling tsleep_nsec(), and reacquire it after. Also, using IORecursiveLock
// would probably allow us to use IORecursiveLockSleep/IORecursiveLockSleepDeadline/IORecursiveLockWakeup for
// tsleep/wakeup. TODO: Look into this.

#include <IOKit/IOLocks.h>
IOSimpleLock *globalLock = nullptr;

#if 0
#define LOCK	IOSimpleLockLockDisableInterrupt
#define UNLOCK	IOSimpleLockUnlockEnableInterrupt
#else
// We create these two functions just so that the debug message is printed correctly
static int  MySimpleLockLock  (IOSimpleLock *l)      { IOSimpleLockLock(l); return 0; }
static void MySimpleLockUnLock(IOSimpleLock *l, int) { IOSimpleLockUnlock(l); }
#define LOCK	MySimpleLockLock
#define UNLOCK	MySimpleLockUnLock
#endif

#define RTSX_STRING(a) RTSX_STRING2(a)
#define RTSX_STRING2(a)	#a

static IOSimpleLock *getGlobalLock()
{
	if (!globalLock) {
		// TODO: There is a race condition here! (globalLock must be initialized before we create the thread).
		UTL_DEBUG_MEM("Allocating global IOSimpleLock");
		globalLock = IOSimpleLockAlloc();
	}
	return globalLock;
}

spl_t Sinetek_rtsx_openbsd_compat_splbio()
{
	spl_t ret = LOCK(getGlobalLock());
	return ret;
}

spl_t Sinetek_rtsx_openbsd_compat_splhigh()
{
	spl_t ret = LOCK(getGlobalLock());
	return ret;
}

void  Sinetek_rtsx_openbsd_compat_splx(spl_t val)
{
	UNLOCK(getGlobalLock(), val);
}
