#ifndef SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
#define SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H

typedef unsigned spl_t;

#if DEBUG
#define RTSX_OPENBSD_COMPAT_SPL_DBG(...) printf(__VA_ARGS__)
#else
#define RTSX_OPENBSD_COMPAT_SPL_DBG(...) do {} while (0)
#endif

#define splbio() \
({ \
	RTSX_OPENBSD_COMPAT_SPL_DBG("splbio called from %s (line %d)", __FILE__, __LINE__); \
	Sinetek_rtsx_openbsd_compat_splbio(); \
})
#define splhigh() \
({ \
	RTSX_OPENBSD_COMPAT_SPL_DBG("splhigh called from %s (line %d)", __FILE__, __LINE__); \
	Sinetek_rtsx_openbsd_compat_splhigh(); \
})
#define splx(v) \
({ \
	Sinetek_rtsx_openbsd_compat_splx(v); \
	RTSX_OPENBSD_COMPAT_SPL_DBG("splx called from %s (line %d)", __FILE__, __LINE__); \
})

spl_t Sinetek_rtsx_openbsd_compat_splbio();

spl_t Sinetek_rtsx_openbsd_compat_splhigh();

void Sinetek_rtsx_openbsd_compat_splx(spl_t val);

#endif // SINETEK_RTSX_OPENBSD_OPENBSD_COMPAT_SPL_H
