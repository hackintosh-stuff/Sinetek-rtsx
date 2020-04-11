#include "openbsd_compat.h"

#include <sys/errno.h>

#include "Sinetek_rtsx.hpp"

void *Sinetek_rtsx_openbsd_compat_owner = nullptr;

int openbsd_compat_start(void *owner)
{
	// initialize all needed variables here
	UTL_CHK_PTR(owner, EINVAL);

	if (Sinetek_rtsx_openbsd_compat_owner) {
		UTL_ERR("Did you try to initialize twice?");
		return ENOTSUP;
	}
	Sinetek_rtsx_openbsd_compat_owner = owner;
	((Sinetek_rtsx *)Sinetek_rtsx_openbsd_compat_owner)->retain();
	return 0;
}

void openbsd_compat_stop()
{
	// free all resources here
	if (Sinetek_rtsx_openbsd_compat_owner == nullptr) {
		UTL_ERR("You either did not initialize, or called the stop() method twice");
		return;
	}
	((Sinetek_rtsx *)Sinetek_rtsx_openbsd_compat_owner)->release();
	Sinetek_rtsx_openbsd_compat_owner = nullptr;
}
