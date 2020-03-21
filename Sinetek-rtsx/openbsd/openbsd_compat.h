// This file aims to make code imported from OpenBSD compile under macOS with the minimum changes.

#ifndef SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_H
#define SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_H

// include necessary headers
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <IOKit/IOTimerEventSource.h>

// other headers from OpenBSD
#include "sdmmc_ioreg.h"
#include "sdmmcdevs.h"
#include "sdmmcvar.h"
#include "rtsxreg.h"
#include "rtsxvar.h"
#include "Sinetek_rtsx.hpp"
#define UTL_THIS_CLASS "" // there is no class, since it is c code
#include "util.h"

#if RTSX_USE_IOLOCK
#define splsdmmc(...) UTLsplsdmmc(sc->splsdmmc_rec_lock)
inline int UTLsplsdmmc(IORecursiveLock *l) {
    IORecursiveLockLock(l);
    /* UTL_DEBUG(2, "Locked splsdmmc_lock"); */
    return 0;
}

#define splx(n) do { \
    /* UTL_DEBUG(2, "Unlocking splsdmmc_lock"); */ \
    IORecursiveLockUnlock(sc->splsdmmc_rec_lock); \
} while (0)
#endif

// disable execcisve logging from OpenBSD code
#define printf(...) do {} while (0)

extern int hz;

#endif // SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_H
