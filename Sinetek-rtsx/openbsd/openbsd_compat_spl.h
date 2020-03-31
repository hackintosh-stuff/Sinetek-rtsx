#ifndef SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
#define SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H

#include <machine/machine_routines.h>

// For now, use the function ml_set_interrupts_enabled()
// See: https://github.com/apple/darwin-xnu/blob/master/osfmk/kern/spl.h

// TODO: It may not be necessary to block interrupts, since we are using workloops too.

typedef unsigned spl_t;

inline spl_t splbio()
{
	return (spl_t) ml_set_interrupts_enabled(FALSE);
}

inline spl_t splhigh()
{
	return (spl_t) ml_set_interrupts_enabled(FALSE);
}

inline void  splx(spl_t val)
{
	ml_set_interrupts_enabled((boolean_t) val);
}

#endif // SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
