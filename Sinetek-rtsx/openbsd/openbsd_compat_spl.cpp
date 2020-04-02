#include "openbsd_compat_spl.h"

#include <machine/machine_routines.h> // FALSE

#define UTL_THIS_CLASS ""
#include "util.h"

// For now, use the function ml_set_interrupts_enabled()
// See: https://github.com/apple/darwin-xnu/blob/master/osfmk/kern/spl.h

#define RTSX_USE_GLOBAL_LOCK 1
#if RTSX_USE_GLOBAL_LOCK
#include <IOKit/IOLocks.h>
IOSimpleLock *globalLock = nullptr;

IOSimpleLock *getGlobalLock() {
	if (!globalLock)
		globalLock = IOSimpleLockAlloc();
	return globalLock;
}

spl_t splbio()
{
	UTL_DEBUG(1, "Calling IOSimpleLockLockDisableInterrupt...");
	return IOSimpleLockLockDisableInterrupt(getGlobalLock());
}

spl_t splhigh()
{
	UTL_DEBUG(1, "Calling IOSimpleLockLockDisableInterrupt...");
	return IOSimpleLockLockDisableInterrupt(getGlobalLock());
}

void  splx(spl_t val)
{
	UTL_DEBUG(1, "Calling IOSimpleLockUnlockEnableInterrupt...");
	return IOSimpleLockUnlockEnableInterrupt(getGlobalLock(), val);
}
#else
spl_t splbio()
{
	return 0;
	return (spl_t) ml_set_interrupts_enabled(FALSE);
}

spl_t splhigh()
{
	return 0;
	return (spl_t) ml_set_interrupts_enabled(FALSE);
}

void  splx(spl_t val)
{
	return;
	ml_set_interrupts_enabled((boolean_t) val);
}
#endif
