#include "openbsd_compat_config.h"

#include <libkern/OSMalloc.h> // OSMalloc, OSFree
#include <string.h> // strcmp

#include "device.h"
#include "sdmmcvar.h" // sdmmc_attach_args, sdmmc_softc_original

#define UTL_THIS_CLASS ""
#include "util.h" // UTL_CHK_PTR, UTL_ERR

// See: https://github.com/openbsd/src/blob/master/sys/kern/subr_autoconf.c

extern struct cfattach sdmmc_ca;
extern struct cfdriver sdmmc_cd;
extern struct cfdriver rtsx_cd;

/// Array of cfdata to be matched
/// TODO: should be this named cfdata as defined in device.h?
static struct cfdata my_cfdata[2] = {
	{ &sdmmc_ca, &sdmmc_cd },
	//{ nullptr, &rtsx_cd },
};

#if 0 // not needed for now...
// static for now...
static void *config_search(cfmatch_t fn, struct device *parent, void *aux)
{
	void *ret = new sdmmc_softc_original;
	return ret;
}
#endif

/**
 * Attach a device (allocate memory for device)
 */
static struct device *config_attach(struct device *parent, void *match, void *aux, cfprint_t print) {
	struct device *ret;
	struct cfdata *this_cfdata = (struct cfdata *) match;
	int32_t devSize = (int32_t) this_cfdata->cf_attach->ca_devsize;
	// allocate memory (TODO: check that it is freed on detach)
	ret = (struct device *) OSMalloc(devSize, OSMT_DEFAULT);
	UTL_CHK_PTR(ret, nullptr);
	bzero(ret, devSize);

	// copy name (the only used member?)
	UTL_CHK_PTR(this_cfdata->cf_driver, nullptr);
	UTL_CHK_PTR(this_cfdata->cf_driver->cd_name, nullptr);

	// set dv_xname (the only used attribute)
	strlcpy(ret->dv_xname, this_cfdata->cf_driver->cd_name , sizeof(ret->dv_xname));

	// call attach (and activate?)
	UTL_CHK_PTR(this_cfdata->cf_attach->ca_attach, nullptr);
	this_cfdata->cf_attach->ca_attach(parent, (struct device *) ret, aux);

	return (struct device *) ret;
}

/// This function is called from:
///  - rtsx_attach, passing a struct sdmmcbus_attach_args as aux argument
///  - sdmmc_io_attach, passing a struct sdmmc_attach_args as aux argument,
///                             a print function, and
///                             a submatch function (sdmmc_submatch)
struct device *config_found_sm(struct device *parent, void *aux, cfprint_t print, cfmatch_t submatch)
{
	// check all members...
	for (int i = 0; i < sizeof(my_cfdata) / sizeof(my_cfdata[0]); i++) {
		if (!my_cfdata[i].cf_attach) continue;

		if (my_cfdata[i].cf_attach->ca_match(parent, my_cfdata + i, aux)) {
			return config_attach(parent, &my_cfdata[i], aux, print);
		}
	}
	return nullptr; // not found
	/* ORIGINAL OPENBSD CODE:
	void *match = config_search(submatch, parent, aux);
	if (match)
		return config_attach(parent, match, aux, print);
	else
		return nullptr;
	*/
}

int config_detach(struct device *dev, int flags)
{
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
