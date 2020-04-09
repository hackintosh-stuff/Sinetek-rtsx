#include "openbsd_compat_rwlock.h"

void rw_init(struct rwlock *rwl, const char *name)
{
}

int rw_assert_wrlock(struct rwlock *rwl)
{
	return 0;
}

void rw_enter_write(struct rwlock *rwl)
{
}

void rw_exit(struct rwlock *rwl) {
}
