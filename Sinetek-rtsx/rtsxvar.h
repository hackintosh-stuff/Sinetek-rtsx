/*	$OpenBSD: rtsxvar.h,v 1.5 2017/09/06 13:07:38 jcs Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 * Copyright (c) 2012 Stefan Sperling <stsp@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _RTSXVAR_H_
#define _RTSXVAR_H_

#include "Sinetek_rtsx.hpp"

/* Number of registers to save for suspend/resume in terms of their ranges. */
#define RTSX_NREG ((0XFDAE - 0XFDA0) + (0xFD69 - 0xFD32) + (0xFE34 - 0xFE20))

extern char * DEVNAME(rtsx_softc *);
int splsdmmc();
void splx(int);

#define SET(t, f)	((t) |= (f))
#define ISSET(t, f)	((t) & (f))
#define	CLR(t, f)	((t) &= ~(f))

/* Host controller functions called by the attachment driver. */
int	rtsx_attach(struct rtsx_softc *);
/* syscl - enable Power Management function */
int rtsx_activate(struct rtsx_softc *self, int act);
int	rtsx_intr(void *);

/* flag values */
#define	RTSX_F_CARD_PRESENT	0x0001
#define	RTSX_F_SDIO_SUPPORT	0x0002
#define	RTSX_F_5209		0x0004
#define	RTSX_F_5229		0x0008
#define	RTSX_F_5229_TYPE_C	0x0010
#define	RTSX_F_525A		0x0020

#define	RTSX_F_525A_TYPE_A	0x0040
#define RTSX_F_REVERSE_SOCKET	0x0080
#define RTSX_F_FORCE_CLKREQ_0	0x1000

#define RTSX_PCI_BAR         0x10
#define RTSX_PCI_BAR_525A    0x14
/* syscl - end of 1.4-1.5 change */

#define PCI_PRODUCT_REALTEK_RTS5209     0x5209          /* RTS5209 PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTS5227     0x5227          /* RTS5227 PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTS5229     0x5229          /* RTS5229 PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTS5249     0x5249          /* RTS5249 PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTL8402     0x5286          /* RTL8402 PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTL8411B    0x5287          /* RTL8411B PCI-E Card Reader */
#define PCI_PRODUCT_REALTEK_RTL8411     0x5289          /* RTL8411 PCI-E Card Reader */

/*
 * syscl - add extra support for new card reader here
 */
#define PCI_PRODUCT_REALTEK_RTS525A     0x525A          /* RTS525A PCI-E Card Reader (XPS 13/15 Series) */


#endif
