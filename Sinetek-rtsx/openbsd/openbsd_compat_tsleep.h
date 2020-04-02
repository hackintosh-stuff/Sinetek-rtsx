#ifndef SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_TSLEEP_H
#define SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_TSLEEP_H

#include <sys/proc.h> // wakeup is defined here, but not tsleep*

static constexpr uint64_t NS_PER_SEC = 1000000000ull;

extern int lbolt; // implement once a second sleep address

#define SEC_TO_NSEC(secs) (NS_PER_SEC * secs)

int tsleep(void *ident, int priority, const char *wmesg, int timo);

int tsleep_nsec(void *ident, int priority, const char *wmesg, uint64_t nsecs);

#endif // SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_TSLEEP_H
