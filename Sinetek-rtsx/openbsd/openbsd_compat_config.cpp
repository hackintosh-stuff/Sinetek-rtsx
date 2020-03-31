#include "openbsd_compat_config.h"

#include <string.h> // strcmp

#include "device.h"
#include "sdmmcvar.h" // sdmmc_attach_args, sdmmc_softc_original

#define UTL_THIS_CLASS ""
#include "util.h" // UTL_MALLOC, UTL_FREE

// See: https://github.com/openbsd/src/blob/master/sys/kern/subr_autoconf.c

extern struct cfattach sdmmc_ca;
extern struct cfdriver sdmmc_cd;

#if 0 // not needed for now...
// static for now...
static void *config_search(cfmatch_t fn, struct device *parent, void *aux)
{
	void *ret = new sdmmc_softc_original;
	return ret;
}

/**
 * Attach a device (allocate memory for device)
 */
static struct device *config_attach(struct device *parent, void *match, void *aux, cfprint_t print) {
	return nullptr;
}
#endif

struct device *config_found_sm(struct device *parent, void *aux, cfprint_t print, cfmatch_t submatch) {
	struct sdmmc_softc_original *ret = UTL_MALLOC(struct sdmmc_softc_original);
	bzero(ret, sizeof(struct sdmmc_softc_original));

	// copy name (the only used member?)
	strlcpy(ret->sc_dev.dv_xname, "sdmmc", sizeof(ret->sc_dev.dv_xname));

	// call attach (and activate?)
	UTL_CHK_PTR(sdmmc_ca.ca_attach, nullptr);
	sdmmc_ca.ca_attach(parent, (struct device *) ret, aux);

	return (struct device *) ret;
	/*
	void *match = config_search(submatch, parent, aux);
	if (match)
		return config_attach(parent, match, aux, print);
	else
		return nullptr;
	*/
}

int config_detach(struct device *dev, int flags) {
	UTL_CHK_PTR(dev, 1);

	if (strcmp(dev->dv_xname, "sdmmc") != 0) {
		UTL_ERR("Device is not sdmmc!");
		return 1;
	}

	// call detach
	UTL_CHK_PTR(sdmmc_ca.ca_detach, 1);
	sdmmc_ca.ca_detach(dev, flags);
	// should free memory
	return 1;
}

int config_activate_children(struct device *, int) {
	return 0; // nothing to do?
}

void config_pending_incr(void) {
	return; // nothing to do?
}
void config_pending_decr(void) {
	return; // nothing to do?
}
