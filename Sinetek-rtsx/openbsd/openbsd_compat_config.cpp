#include "openbsd_compat_config.h"

#include <string.h> // strcmp
#include <libkern/OSMalloc.h> // OSMalloc, OSFree
#include <sys/errno.h> // ENOTSUP

#include "device.h"
#include "sdmmcvar.h" // sdmmc_attach_args, sdmmc_softc_original

#define UTL_THIS_CLASS ""
#include "util.h" // UTL_CHK_PTR, UTL_ERR

// See: https://github.com/openbsd/src/blob/master/sys/kern/subr_autoconf.c

extern struct cfattach sdmmc_ca;
extern struct cfdriver sdmmc_cd;
extern struct cfdriver rtsx_cd;

// forward-declare static functions
static int config_deactivate(struct device *dev);
static int config_suspend(struct device *dev, int act);

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
	UTL_CHK_PTR(this_cfdata, nullptr);
	UTL_CHK_PTR(this_cfdata->cf_attach, nullptr);

	int32_t devSize = (int32_t) this_cfdata->cf_attach->ca_devsize;
	// allocate memory (TODO: check that it is freed on detach)
	ret = (struct device *) OSMalloc(devSize, OSMT_DEFAULT);
	UTL_CHK_PTR(ret, nullptr);
	bzero(ret, devSize);

	// copy name (the only used member?)
	UTL_CHK_PTR(this_cfdata->cf_driver, nullptr);
	UTL_CHK_PTR(this_cfdata->cf_driver->cd_name, nullptr);

	// set dv_xname
	strlcpy(ret->dv_xname, this_cfdata->cf_driver->cd_name , sizeof(ret->dv_xname));
	ret->dv_cfdata = this_cfdata; // used by config_detach
	ret->dv_parent = parent; // parent device (is it used?)

	// call attach (and activate?)
	UTL_CHK_PTR(this_cfdata->cf_attach->ca_attach, nullptr);
	UTL_DEBUG(1, "Calling ca_attach...");
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
	UTL_DEBUG(1, "START");
	// check all members...
	for (int i = 0; i < sizeof(my_cfdata) / sizeof(my_cfdata[0]); i++) {
		UTL_DEBUG(1, "Trying my_cfdata[%d]...", i);
		if (!my_cfdata[i].cf_attach) continue;

		UTL_DEBUG(2, "Calling ca_match...");
		if (my_cfdata[i].cf_attach->ca_match(parent, my_cfdata + i, aux)) {
			UTL_DEBUG(1, "Match found. Calling config_attach...");
			return config_attach(parent, &my_cfdata[i], aux, print);
		}
	}
	UTL_ERR("Config not found. Returning null.");
	return nullptr; // not found
	/* ORIGINAL OPENBSD CODE:
	void *match = config_search(submatch, parent, aux);
	if (match)
		return config_attach(parent, match, aux, print);
	else
		return nullptr;
	*/
}

/// Calls deactivate, detach, and frees memory
int config_detach(struct device *dev, int flags)
{
	int ret = 0;
	UTL_CHK_PTR(dev, EINVAL);
	UTL_CHK_PTR(dev->dv_cfdata, EINVAL);

	struct cfattach *cf_attach = dev->dv_cfdata->cf_attach;

	UTL_DEBUG(1, "Detaching %s device", dev->dv_xname);

	// call deactivate
	ret = config_deactivate(dev);
	if (ret == 0) {
		// call detach
		if (cf_attach && cf_attach->ca_detach)
			ret = cf_attach->ca_detach(dev, flags);
		else
			ret = ENOTSUP;
	}

	// TODO: Check OpenBSD code. It does more processing here...
	
	uint32_t devSize = (uint32_t) cf_attach->ca_devsize;
	OSFree(dev, devSize, OSMT_DEFAULT);
	return ret;
}

int config_activate_children(struct device *, int) {
	return 0; // nothing to do?
}

static /* for now */
int config_deactivate(struct device *dev)
{
	int rv = 0, oflags = dev->dv_flags;

	if (dev->dv_flags & DVF_ACTIVE) {
		dev->dv_flags &= ~DVF_ACTIVE;
		rv = config_suspend(dev, DVACT_DEACTIVATE);
		if (rv)
			dev->dv_flags = oflags;
	}
	return rv;
}

void config_pending_incr(void) {
	return; // nothing to do?
}
void config_pending_decr(void) {
	return; // nothing to do?
}

static /* for now */
int config_suspend(struct device *dev, int act)
{
	struct cfattach *ca = dev->dv_cfdata->cf_attach;
	int r;

	// device_ref(dev);
	if (ca->ca_activate)
		r = (*ca->ca_activate)(dev, act);
	else
		r = config_activate_children(dev, act);
	// device_unref(dev);
	return (r);
}
