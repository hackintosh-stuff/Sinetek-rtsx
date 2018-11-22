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
#define	RTSX_F_CARD_PRESENT 0x01
#define	RTSX_F_SDIO_SUPPORT 0x02
#define	RTSX_F_5209         0x04
#define	RTSX_F_5229         0x08
#define	RTSX_F_5229_TYPE_C  0x10
/*
 * syscl - as of v 1.4-1.5 added support: 0x525A
 */
#define    RTSX_F_525A       0x20


#define RTSX_PCI_BAR         0x10
#define RTSX_PCI_BAR_525A    0x14
/* syscl - end of 1.4-1.5 change */


/* maz-1 - rts525A register values from linux driver */
#define RTS524A_PM_CTRL3	0xFF7E
#define D3_DELINK_MODE_EN	0x10
#define _PHY_ANA03			0x03
#define   _PHY_ANA03_TIMER_MAX		0x2700
#define   _PHY_ANA03_OOBS_DEB_EN	0x0040
#define   _PHY_CMU_DEBUG_EN		0x0008
#define _PHY_REV0			0x19
#define   _PHY_REV0_FILTER_OUT		0x3800
#define   _PHY_REV0_CDR_BYPASS_PFD	0x0100
#define   _PHY_REV0_CDR_RX_IDLE_BYPASS	0x0002
#define _PHY_FLD0			0x1D
#define   _PHY_FLD0_CLK_REQ_20C		0x8000
#define   _PHY_FLD0_RX_IDLE_EN		0x1000
#define   _PHY_FLD0_BIT_ERR_RSTN	0x0800
#define   _PHY_FLD0_BER_COUNT		0x01E0
#define   _PHY_FLD0_BER_TIMER		0x001E
#define   _PHY_FLD0_CHECK_EN		0x0001
#define PCLK_MODE_SEL	0x20
#define PCLK_CTL	0xFE55
#define LDO_VIO_CFG			0xFF75
#define   LDO_VIO_SR_MASK		0xC0
#define   LDO_VIO_SR_DF			0x40
#define   LDO_VIO_REF_TUNE_MASK		0x30
#define   LDO_VIO_REF_1V2		0x20
#define   LDO_VIO_TUNE_MASK		0x07
#define   LDO_VIO_1V7			0x03
#define   LDO_VIO_1V8			0x04
#define LDO_VIO_3V3	0x07
#define OOBS_CONFIG			0xFF6E
#define   OOBS_AUTOK_DIS		0x80
#define OOBS_VAL_MASK	0x1F
#define LDO_VCC_LMTVTH_MASK	0x30
#define LDO_AV12S_TUNE_MASK	0x07
#define LDO_VCC_LMTVTH_2A	0x10
#define LDO_VCC_CFG0	0xFF72
#define LDO_AV12S_CFG			0xFF77
#define   LDO_AV12S_TUNE_MASK		0x07
#define LDO_AV12S_TUNE_DF	0x04
#define LDO_DV12S_CFG			0xFF76
#define   LDO_D12_TUNE_MASK		0x07
#define LDO_D12_TUNE_DF	0x04
#define L1SUB_CONFIG2	0xFE8E
#define L1SUB_AUTO_CFG	0x02
#define RREF_CFG			0xFF6C
#define   RREF_VBGSEL_MASK		0x38
#define RREF_VBGSEL_1V25	0x28


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
