#include "Sinetek_rtsx.hpp"

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOTimerEventSource.h>
#if RTSX_USE_IOFIES
#include <IOKit/IOFilterInterruptEventSource.h>
#endif

#undef super
#define super IOService
OSDefineMetaClassAndStructors(Sinetek_rtsx, super);

#include "rtsxreg.h"
#include "rtsxvar.h" // rtsx_softc
#include "SDDisk.hpp"

#undef UTL_THIS_CLASS
#define UTL_THIS_CLASS "Sinetek_rtsx::"
#include "util.h"

//
// syscl - define & enumerate power states
//
enum
{
	kPowerStateSleep    = 0,
	kPowerStateDoze     = 1,
	kPowerStateNormal   = 2,
	kPowerStateCount
};

//
// syscl - Define usable power states
//
static IOPMPowerState ourPowerStates[kPowerStateCount] =
{
	{ 1,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,kIOPMDeviceUsable,kIOPMDoze,kIOPMDoze,0,0,0,0,0,0,0,0 },
	{ 1,kIOPMDeviceUsable,IOPMPowerOn,IOPMPowerOn,0,0,0,0,0,0,0,0 }
};

bool Sinetek_rtsx::init(OSDictionary *dictionary) {
	if (!super::init()) return false;
	UTL_DEBUG(1, "START");

	// initialize variables / allocate memory
	if (!(rtsx_softc_original_ = UTL_MALLOC(struct rtsx_softc))) {
		UTL_ERR("ERROR ALLOCATING MEMORY!");
		return false;
	}
	UTL_DEBUG(1, "END");
	return true;
}

void Sinetek_rtsx::free() {
	UTL_DEBUG(1, "START");
	// deallocate memory
	UTL_FREE(rtsx_softc_original_, struct rtsx_softc);

	super::free();
	UTL_DEBUG(1, "END");
}

bool Sinetek_rtsx::start(IOService *provider)
{
	UTL_DEBUG(1, "START");
	if (!super::start(provider))
		return false;

#if RTSX_USE_IOLOCK
	this->intr_status_lock = IOLockAlloc();
	this->splsdmmc_rec_lock = IORecursiveLockAlloc();
	this->intr_status_event = false;
#endif // RTSX_USE_IOLOCK
	assert(OSDynamicCast(IOPCIDevice, provider));
	provider_ = OSDynamicCast(IOPCIDevice, provider);
	if (!provider_)
	{
		UTL_ERR("IOPCIDevice cannot be cast.");
		goto ERROR;
	}

	workloop_ = getWorkLoop();
	if (!workloop_) goto ERROR;
	/*
	 * Enable memory response from the card.
	 */
	provider_->setMemoryEnable(true);

	prepare_task_loop();
	rtsx_pci_attach();
	
	PMinit();
	provider_->joinPMtree(this);
	if (registerPowerDriver(this, ourPowerStates, kPowerStateCount) != IOPMNoErr)
	{
		UTL_ERR("registerPowerDriver failed");
	}
	UTL_DEBUG(2, "Power management initialized");

	UTL_LOG("Driver started (%s version)",
#if DEBUG
		"debug");
#else
		"release");
#endif
	return true;
ERROR:
	workloop_ = NULL;
	if (splsdmmc_rec_lock)
		IORecursiveLockFree(splsdmmc_rec_lock);
	if (intr_status_lock)
		IOLockFree(intr_status_lock);

	UTL_DEBUG(1, "END");
	return false;
}

// WATCH OUT: stop() may not be called if start() fails!
void Sinetek_rtsx::stop(IOService *provider)
{
	rtsx_pci_detach();
	destroy_task_loop();
	if (workloop_) {
		workloop_->release();
		workloop_ = nullptr;
	} else {
		UTL_ERR("workloop_ is null");
	}
	PMstop();

	super::stop(provider);
	UTL_LOG("Driver stopped.");
}

static void trampoline_intr(OSObject *ih, IOInterruptEventSource *ies, int count)
{
	UTL_DEBUG(3, "Interrupt received (ies=" RTSX_PTR_FMT " count=%d)!", RTSX_PTR_FMT_VAR(ies), count);
	/* go to isr handler */
	auto self = OSDynamicCast(Sinetek_rtsx, ih);
	if (self && self->rtsx_softc_original_)
		::rtsx_intr(self->rtsx_softc_original_);
	else
		UTL_ERR("Object received is not a Sinetek_rtsx!");
}

void Sinetek_rtsx::rtsx_pci_attach()
{
	uint device_id;
	//uint32_t flags;
	int bar = RTSX_PCI_BAR;

	UTL_DEBUG(2, "START");

	if ((provider_->extendedConfigRead16(RTSX_CFG_PCI) & RTSX_CFG_ASIC) != 0)
	{
		printf("no asic\n");
		return;
	}

	/* Enable the device */
	provider_->setBusMasterEnable(true);

	/* Map device memory with register. */
	device_id = provider_->extendedConfigRead16(kIOPCIConfigDeviceID);
	if (device_id == PCI_PRODUCT_REALTEK_RTS525A)
		bar = RTSX_PCI_BAR_525A;
	map_ = provider_->mapDeviceMemoryWithRegister(bar);
	if (!map_) return;
	memory_descriptor_ = map_->getMemoryDescriptor();

	/* Map device interrupt. */
#if RTSX_USE_IOFIES
	intr_source_ = IOFilterInterruptEventSource::filterInterruptEventSource(this, trampoline_intr, is_my_interrupt, provider_);
#else // RTSX_USE_IOFIES
	intr_source_ = IOInterruptEventSource::interruptEventSource(this, trampoline_intr, provider_);
#endif // RTSX_USE_IOFIES
	if (!intr_source_)
	{
		printf("can't map interrupt source\n");
		return;
	}
	workloop_->addEventSource(intr_source_);
	intr_source_->enable();

	/* Get the vendor and try to match on it. */
	//device_id = provider_->extendedConfigRead16(kIOPCIConfigDeviceID);
	switch (device_id) {
		case PCI_PRODUCT_REALTEK_RTS5209:
			flags = RTSX_F_5209;
			break;
		case PCI_PRODUCT_REALTEK_RTS5229:
		case PCI_PRODUCT_REALTEK_RTS5249:
			flags = RTSX_F_5229;
			break;
		case PCI_PRODUCT_REALTEK_RTS525A:
			/* syscl - RTS525A */
			flags = RTSX_F_525A;
			bar = RTSX_PCI_BAR_525A;
			break;
		default:
			flags = 0;
			break;
	}

	UTL_CHK_PTR(this->rtsx_softc_original_,);
	UTL_DEBUG(2, "Calling attach...");
	int error = ::rtsx_attach(this->rtsx_softc_original_, gBusSpaceTag,
				  (bus_space_handle_t) memory_descriptor_,
				  0/* ignored */,
				  gBusDmaTag, flags);

	if (!error) {
		//		pci_present_and_attached_ = true;
		if (this->flags & RTSX_F_SDIO_SUPPORT)
			UTL_DEBUG(1, "SDIO detected");
	} else {
		UTL_ERR("rtsx_attach returned error %d", error);
	}
	UTL_DEBUG(2, "END");
}

void Sinetek_rtsx::rtsx_pci_detach()
{
	//	rtsx_detach();

	UTL_CHK_PTR(workloop_,);
	UTL_CHK_PTR(intr_source_,);

	workloop_->removeEventSource(intr_source_);
	intr_source_->release();
	intr_source_ = nullptr;
#if RTSX_USE_IOLOCK
	// should this be called in free()?
	UTL_CHK_PTR(splsdmmc_rec_lock,);
	UTL_CHK_PTR(intr_status_lock,);
	IORecursiveLockFree(this->splsdmmc_rec_lock);
	this->splsdmmc_rec_lock = nullptr;
	IOLockFree(this->intr_status_lock);
	this->intr_status_lock = nullptr;
#endif // RTSX_USE_IOLOCK
}

// TODO: Seems like this is wrong. Power states should match those supported by the OpenBSD driver.
// TODO: Review this method and power in general.
// TODO: Be careful because this is called from a different thread, (in parallel with start())
IOReturn Sinetek_rtsx::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker)
{
	IOReturn ret = IOPMAckImplied;
	auto previousState = this->getPowerState();

	UTL_DEBUG(1, "START");

	if (powerStateOrdinal)
	{
		UTL_DEBUG(0, "Wake from sleep (powerStateOrdinal: %u -> %u)",
			  (unsigned) previousState,
			  (unsigned) powerStateOrdinal);
		// TODO: SEND PROPER ARGUMENT (DVACT_*)
		//        rtsx_activate(this, 1);
		goto done;
	}
	else
	{
		UTL_DEBUG(0, "Sleep the card (powerStateOrdinal: %u -> %u)\n",
			  (unsigned) previousState,
			  (unsigned) powerStateOrdinal);
		// TODO: SEND PROPER ARGUMENT (DVACT_*)
		//        rtsx_activate(this, 0);
		goto done;
	}

done:
	UTL_DEBUG(1, "END");

	return ret;
}

void Sinetek_rtsx::prepare_task_loop()
{
#if RTSX_USE_IOCOMMANDGATE
	task_command_gate_ = IOCommandGate::commandGate(this, rtsx_softc::executeOneCommandGateAction);
	workloop_->addEventSource(task_command_gate_);
#endif
#if RTSX_USE_IOCOMMANDGATE
	// connect the timer to the interrupt workloop_
	task_execute_one_ = IOTimerEventSource::timerEventSource(this, task_execute_one_impl_);
	workloop_->addEventSource(task_execute_one_);
#else
	task_loop_ = IOWorkLoop::workLoop();
	task_execute_one_ = IOTimerEventSource::timerEventSource(this, task_execute_one_impl_);
	task_loop_->addEventSource(task_execute_one_);
#endif
}

void Sinetek_rtsx::destroy_task_loop()
{
	UTL_CHK_PTR(task_execute_one_,);
	task_execute_one_->cancelTimeout();
#if RTSX_USE_IOCOMMANDGATE
	UTL_CHK_PTR(workloop_,)
	workloop_->removeEventSource(task_command_gate_);
	UTL_CHK_PTR(task_command_gate_,);
	task_command_gate_->release();
	task_command_gate_ = nullptr;
	UTL_CHK_PTR(workloop_,);
	workloop_->removeEventSource(task_execute_one_);
	task_execute_one_->release();
	task_execute_one_ = nullptr;
#else
	UTL_CHK_PTR(task_loop_,);
	task_loop_->removeEventSource(task_execute_one_);
	task_execute_one_->release();
	task_execute_one_ = nullptr;
	task_loop_->release();
	task_loop_ = nullptr;
#endif
}

void Sinetek_rtsx::task_execute_one_impl_(OSObject *target, IOTimerEventSource *sender)
{
	extern void read_task_impl_(void *_args);
	UTL_DEBUG(1, "  ===> TOOK TASK WORKLOOP...");
	if (!target) return;
	Sinetek_rtsx *sc = OSDynamicCast(Sinetek_rtsx, target);
	struct sdmmc_task *task;

#if RTSX_USE_IOLOCK
	IORecursiveLockLock(sc->splsdmmc_rec_lock);
#endif
	struct sdmmc_softc *sdmmc = (struct sdmmc_softc *) sc->rtsx_softc_original_->sdmmc;
	auto tskq = &sdmmc->sc_tskq;
	for (task = TAILQ_FIRST(tskq); task != NULL;
	     task = TAILQ_FIRST(tskq)) {
		UTL_DEBUG(1, "  => Executing one task (CRASHED HERE!)...");
		sdmmc_del_task(task);
		UTL_DEBUG(1, "  => Task deleted from queue");
		task->func(task->arg);
		UTL_DEBUG(1, "  => Executed one task!");
#if RTSX_FIX_TASK_BUG
		if (task->func == read_task_impl_) {
			// free only read_task
			UTL_DEBUG(0, "Freeing read task...");
			UTL_FREE(task, sizeof(sdmmc_task));
		}
#endif
	}
#if RTSX_USE_IOLOCK
	IORecursiveLockUnlock(sc->splsdmmc_rec_lock);
#endif
	UTL_DEBUG(1, "  <=== RELEASING TASK WORKLOOP...");
}

// forward declare
extern void rtsx_card_eject(struct rtsx_softc *);

void Sinetek_rtsx::cardEject()
{
	::rtsx_card_eject(rtsx_softc_original_);
}

/**
 *  Attach the macOS portion of the driver: block storage.
 *
 *  That block device will hand us back calls such as read/write blk.
 */
void Sinetek_rtsx::blk_attach()
{
	// TODO: Return an error when this method fails!
	printf("rtsx: blk_attach()\n");

	sddisk_ = new SDDisk();
	UTL_CHK_PTR(sddisk_,);
	if (!sddisk_->init((struct sdmmc_softc *) rtsx_softc_original_->sdmmc)) { // TODO: Fix this!
		sddisk_->release();
		sddisk_ = NULL;
		return;
	};
	sddisk_->attach(this);
	UTL_DEBUG(0, "Registering service...");
	sddisk_->registerService(); // this should probably be called by the start() method of sddisk_
	sddisk_->release();
}

void Sinetek_rtsx::blk_detach()
{
	UTL_DEBUG(0, "START");

	if (!sddisk_->terminate()) {
		UTL_DEBUG(0, "sddisk->terminate() returns false!");
	}
	sddisk_ = NULL;
	UTL_DEBUG(0, "END");
}

#if RTSX_USE_IOFIES
static uint32_t READ4(Sinetek_rtsx *sc, IOByteCount reg) {
	uint32_t ret = 0;
	sc->memory_descriptor_->readBytes(reg, &ret, 4);
	return ret;
}

/// This function runs in interrupt context, meaning that IOLog CANNOT be used (only basic functionality is available).
bool Sinetek_rtsx::is_my_interrupt(OSObject *arg, IOFilterInterruptEventSource *source) {
	if (!arg) return false;

	Sinetek_rtsx *sc = OSDynamicCast(Sinetek_rtsx, arg);
	if (!sc) return false;

	auto status = READ4(sc, RTSX_BIPR);
	if (!status) {
		return false;
	}

	auto bier = READ4(sc, RTSX_BIER);
	if ((status & bier) == 0) return false;

	return true;
}
#endif // RTSX_USE_IOFIES


#if RTSX_USE_IOCOMMANDGATE
void Sinetek_rtsx::executeOneAsCommand() {
	UTL_DEBUG(0, "Calling runAction()...");
	task_command_gate_->runCommand(nullptr);
	task_command_gate_->runAction((IOCommandGate::Action) executeOneCommandGateAction);
	UTL_DEBUG(0, "runAction() returns");
}

IOReturn Sinetek_rtsx::executeOneCommandGateAction(OSObject *obj,
						 void *,
						 void *, void *, void * )
{
	UTL_DEBUG(0, "EXECUTE ONE COMMANDGATEACTION CALLED!");
	rtsx_softc::task_execute_one_impl_(obj, nullptr /* unused */);
	return kIOReturnSuccess;
	UTL_DEBUG(0, "EXECUTE ONE COMMANDGATEACTION RETURNING");
}
#endif // RTSX_USE_IOCOMMANDGATE
