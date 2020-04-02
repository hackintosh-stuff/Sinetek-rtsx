#ifndef SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
#define SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H

// TODO: It may not be necessary to block interrupts, since we are using workloops too.

typedef unsigned spl_t;

spl_t splbio();

spl_t splhigh();

void splx(spl_t val);

#endif // SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
