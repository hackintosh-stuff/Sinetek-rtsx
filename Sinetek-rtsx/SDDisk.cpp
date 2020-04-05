#include <IOKit/IOLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/IOMemoryDescriptor.h>

#define UTL_THIS_CLASS "SDDisk::"

#include "SDDisk.hpp"
#include "Sinetek_rtsx.hpp"
#include "rtsxvar.h" // rtsx_softc
#include "sdmmcvar.h" // sdmmc_mem_read_block
#include "device.h"

#include "util.h"

// Define the superclass
#define super IOBlockStorageDevice
OSDefineMetaClassAndStructors(SDDisk, IOBlockStorageDevice)

/// This is a nub, and a nub should be quite stupid, but this is not. Check:
/// https://github.com/zfsrogue/osx-zfs-crypto/blob/master/module/zfs/zvolIO.cpp -> A good nub, it just gets a state
/// from the provider (no memory allocations or resources needed. Just forward everything to the provider (except the
/// doAsyncReadWrite() method).
/// Also:
/// https://opensource.apple.com/source/IOATABlockStorage/IOATABlockStorage-112.0.1/ ->
/// IOATABlockStorage(Device/Driver) is another good example (note that IOATABlockStorageDriver does not inherit from
/// IOBlockStorageDriver, because it's not the same (IOATABlockStorageDriver is the provider of IOATABlockStorageDriver,
/// but IOBlockStorageDriver is the CLIENT of IOBlockStorageDevice). This one DOES forward doAsyncReadWrite.

bool SDDisk::init(struct sdmmc_softc *sc_sdmmc, OSDictionary* properties)
{
	UTL_DEBUG_FUN("START");
	if (super::init(properties) == false)
		return false;

	util_lock_ = NULL;
	sdmmc_softc_ = sc_sdmmc;

	UTL_DEBUG_FUN("END");
	return true;
}

// TODO: Is this even called?
void SDDisk::free()
{
	UTL_DEBUG_FUN("START");
	sdmmc_softc_ = NULL;
	super::free();
}

bool SDDisk::attach(IOService* provider)
{
	UTL_LOG("SDDisk attaching...");
	if (super::attach(provider) == false)
		return false;

	if (provider_) {
		UTL_ERR("provider should be null, but it's not!");
	}
	provider_ = OSDynamicCast(Sinetek_rtsx, provider);
	if (provider_ == NULL)
		return false;

	provider_->retain();

	UTL_CHK_PTR(sdmmc_softc_, false);
	UTL_CHK_PTR(sdmmc_softc_->sc_fn0, false);
	num_blocks_ = sdmmc_softc_->sc_fn0->csd.capacity;
	blk_size_   = sdmmc_softc_->sc_fn0->csd.sector_size;
	util_lock_ = IOLockAlloc();

	printf("rtsx: attaching SDDisk, num_blocks:%d  blk_size:%d\n",
	       num_blocks_, blk_size_);

	UTL_LOG("SDDisk attached.");
	return true;
}

void SDDisk::detach(IOService* provider)
{
	UTL_LOG("SDDisk detaching...");
	IOLockFree(util_lock_);
	util_lock_ = NULL;
	provider_->release();
	provider_ = NULL;

	super::detach(provider);
	UTL_LOG("SDDisk detached.");
}

IOReturn SDDisk::doEjectMedia(void)
{
	UTL_DEBUG_FUN("START");
	provider_->cardEject();
	UTL_DEBUG_FUN("END");
	return kIOReturnSuccess;
}

IOReturn SDDisk::doFormatMedia(UInt64 byteCapacity)
{
	UTL_DEBUG_FUN("START");
	return kIOReturnSuccess;
}

UInt32 SDDisk::GetBlockCount() const
{
	UTL_DEBUG_DEF("Returning %u blocks", num_blocks_);
	return num_blocks_;
}

UInt32 SDDisk::doGetFormatCapacities(UInt64* capacities, UInt32 capacitiesMaxCount) const
{
	UTL_DEBUG_FUN("START");
	// Ensure that the array is sufficient to hold all our formats (we require 1 element).
	if ((capacities != NULL) && (capacitiesMaxCount < 1))
		return 0;               // Error, return an array size of 0.

	/*
	 * We need to run circles around the const-ness of this function.
	 */
	//	auto blockCount = const_cast<SDDisk *>(this)->getBlockCount();
	auto blockCount = GetBlockCount();

	// The caller may provide a NULL array if it wishes to query the number of formats that we support.
	if (capacities != NULL)
		capacities[0] = blockCount * blk_size_;
	return 1;
}

IOReturn SDDisk::doLockUnlockMedia(bool doLock)
{
	UTL_DEBUG_FUN("START");
	return kIOReturnUnsupported;
}

IOReturn SDDisk::doSynchronizeCache(void)
{
	UTL_DEBUG_FUN("START");
	return kIOReturnSuccess;
}

char* SDDisk::getVendorString(void)
{
	UTL_DEBUG_FUN("START");
	// syscl - safely converted to char * use const_static due
	// to ISO C++11 does not allow conversion from string literal to 'char *'
	return const_cast<char *>("Realtek");
}

char* SDDisk::getProductString(void)
{
	UTL_DEBUG_FUN("START");
	// syscl - safely converted to char * use const_static due
	// to ISO C++11 does not allow conversion from string literal to 'char *'
	return const_cast<char *>("SD Card Reader");
}

char* SDDisk::getRevisionString(void)
{
	UTL_DEBUG_FUN("START");
	// syscl - safely converted to char * use const_static due
	// to ISO C++11 does not allow conversion from string literal to 'char *'
	return const_cast<char *>("1.0");
}

char* SDDisk::getAdditionalDeviceInfoString(void)
{
	UTL_DEBUG_FUN("START");
	// syscl - safely converted to char * use const_static due
	// to ISO C++11 does not allow conversion from string literal to 'char *''
	return const_cast<char *>("1.0");
}

IOReturn SDDisk::reportBlockSize(UInt64 *blockSize)
{
	UTL_DEBUG_FUN("START");
	if (blk_size_ != 512) {
		UTL_DEBUG_DEF("Reporting block size != 512 (%u)", blk_size_);
	}
	*blockSize = blk_size_;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportEjectability(bool *isEjectable)
{
	UTL_DEBUG_FUN("START");
	*isEjectable = true; // syscl - should be true
	return kIOReturnSuccess;
}

/* syscl - deprecated
 IOReturn SDDisk::reportLockability(bool *isLockable)
 {
 *isLockable = false;
 return kIOReturnSuccess;
 }*/

IOReturn SDDisk::reportMaxValidBlock(UInt64 *maxBlock)
{
	UTL_DEBUG_DEF("Called (maxBlock = %u)", num_blocks_ - 1);
	*maxBlock = num_blocks_ - 1;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportMediaState(bool *mediaPresent, bool *changedState)
{
	UTL_DEBUG_FUN("START");
	*mediaPresent = true;
	*changedState = false;

	return kIOReturnSuccess;
}

IOReturn SDDisk::reportPollRequirements(bool *pollRequired, bool *pollIsExpensive)
{
	UTL_DEBUG_FUN("START");
	*pollRequired = false;
	*pollIsExpensive = false;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportRemovability(bool *isRemovable)
{
	UTL_DEBUG_FUN("START");
	*isRemovable = true;
	return kIOReturnSuccess;
}

IOReturn SDDisk::reportWriteProtection(bool *isWriteProtected)
{
	UTL_DEBUG_FUN("START");
	*isWriteProtected = true; // XXX
	return kIOReturnSuccess;
}

IOReturn SDDisk::getWriteCacheState(bool *enabled)
{
	UTL_DEBUG_FUN("START");
	return kIOReturnUnsupported;
}

IOReturn SDDisk::setWriteCacheState(bool enabled)
{
	UTL_DEBUG_FUN("START");
	return kIOReturnUnsupported;
}


struct BioArgs
{
	IOMemoryDescriptor *buffer;
	IODirection direction;
	UInt64 block;
	UInt64 nblks;
	IOStorageAttributes attributes;
	IOStorageCompletion completion;
	SDDisk *that;
};

// cholonam: This task is put on a queue which is run by sc::task_execute_one_ (originally using a timer, now trying to
// change to an IOCommandGate.
void read_task_impl_(void *_args)
{
	BioArgs *args = (BioArgs *) _args;
	IOByteCount actualByteCount;
	int error = 0;

	UTL_CHK_PTR(args,);
	UTL_CHK_PTR(args->buffer,);
	UTL_CHK_PTR(args->that,);
	UTL_CHK_PTR(args->that->provider_,);
	UTL_CHK_PTR(args->that->provider_->rtsx_softc_original_,);
	UTL_CHK_PTR(args->that->provider_->rtsx_softc_original_->sdmmc,);
	auto sdmmc = (struct sdmmc_softc *) args->that->provider_->rtsx_softc_original_->sdmmc;
	UTL_CHK_PTR(sdmmc->sc_fn0,);
	UTL_DEBUG_DEF("START (block = %u nblks = %u blksize = %u)",
		  static_cast<unsigned>(args->block),
		  static_cast<unsigned>(args->nblks),
		  args->that->blk_size_);

	printf("read_task_impl_  sz %llu\n", args->nblks * args->that->blk_size_);
	printf("sf->csd.sector_size %d\n", sdmmc->sc_fn0->csd.sector_size);


	actualByteCount = args->nblks * args->that->blk_size_;
#if RTSX_USE_WRITEBYTES
	u_char buf[512];
#else
	{ auto map = args->buffer->map();
		// this is for 32-bit?? u_char * buf = (u_char *) map->getVirtualAddress();
		u_char *buf = (u_char *) map->getAddress();

		for (UInt64 b = 0; b < args->nblks; ++b)
		{
			//		sdmmc_mem_single_read_block(args->that->provider_->sc_fn0,
			//						    0, buf + b * 512, 512);
			sdmmc_mem_read_block_subr(args->that->provider_->sc_fn0,
						  0, buf, 512);
			//		sdmmc_go_idle_state(args->that->provider_);
		}
		map->release(); } // need to release map
#endif

	for (UInt64 b = 0; b < args->nblks; b++)
	{
		printf("would: %lld  last block %d\n", args->block + b, args->that->num_blocks_ - 1);
		//unsigned int would = args->block + b;
#if RTSX_USE_WRITEBYTES
		// This is a safer version
		error = sdmmc_mem_read_block(sdmmc->sc_fn0,
					     static_cast<int>(args->block + b),
					     buf, 512);
		if (!error) {
			args->buffer->writeBytes(b * 512, buf, 512);
		} else {
			UTL_ERR("ERROR READING BLOCK (blockNo=%d error=%d)", static_cast<int>(args->block + b), error);
		}
#else
		auto would = args->block + b;
		//if ( would > 60751872 ) would = 60751871;
		error = sdmmc_mem_read_block_subr(args->that->provider_->sc_fn0,
						  static_cast<int>(would), buf + b * 512, 512);
#endif
		if (error) {
			if (args->completion.action) {
				(args->completion.action)(args->completion.target, args->completion.parameter,
							  kIOReturnIOError, 0);
			} else {
				UTL_ERR("No completion action!");
			}
			goto out;
		}
	}

	if (args->completion.action) {
		(args->completion.action)(args->completion.target, args->completion.parameter,
					  kIOReturnSuccess, actualByteCount);
	} else {
		UTL_ERR("No completion action!");
	}

out:
	if (error) {
		UTL_ERR("END (error = %d)", error);
	} else {
		UTL_DEBUG_DEF("END (error = %d)", error);
	}
	delete args;
}

/**
 * Start an async read or write operation.
 * @param buffer
 * An IOMemoryDescriptor describing the data-transfer buffer. The data direction
 * is contained in the IOMemoryDescriptor. Responsibility for releasing the descriptor
 * rests with the caller.
 * @param block
 * The starting block number of the data transfer.
 * @param nblks
 * The integral number of blocks to be transferred.
 * @param attributes
 * Attributes of the data transfer. See IOStorageAttributes.
 * @param completion
 * The completion routine to call once the data transfer is complete.
 */
IOReturn SDDisk::doAsyncReadWrite(IOMemoryDescriptor *buffer,
				  UInt64 block,
				  UInt64 nblks,
				  IOStorageAttributes *attributes,
				  IOStorageCompletion *completion)
{
	IODirection		direction;

	// Return errors for incoming I/O if we have been terminated
	if (isInactive() != false)
		return kIOReturnNotAttached;

	direction = buffer->getDirection();
	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;

	if (direction == kIODirectionOut)
		return kIOReturnNotWritable; // read-only driver for now...

	if ((block + nblks) > num_blocks_)
		return kIOReturnBadArgument;

	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;

	// printf("=====================================================\n");
	UTL_DEBUG_DEF("START (block=%d nblks=%d)", (int) block, (int) nblks);
	IOLockLock(util_lock_);

	/*
	 * Copy things over as we're going to lose the parameters once this
	 * method returns. (async call)
	 */
	BioArgs *bioargs = new BioArgs;
	bioargs->buffer = buffer;
	bioargs->direction = direction;
	bioargs->block = block;
	bioargs->nblks = nblks;
	if (attributes != nullptr)
		bioargs->attributes = *attributes;
	if (completion != nullptr)
		bioargs->completion = *completion;
	bioargs->that = this;

#if RTSX_FIX_TASK_BUG
	UTL_DEBUG_DEF("Allocating read task...");
	auto newTask = UTL_MALLOC(sdmmc_task); // will be deleted after processed
	if (!newTask) return kIOReturnNoMemory;
	sdmmc_init_task(newTask, read_task_impl_, bioargs);
	sdmmc_add_task(sdmmc_softc_, newTask);
#else
	sdmmc_init_task(&provider_->read_task_, read_task_impl_, bioargs);
	sdmmc_add_task(sdmmc_softc_, &provider_->read_task_);
#endif

	IOLockUnlock(util_lock_);
	// printf("=====================================================\n");

	UTL_DEBUG_DEF("RETURNING SUCCESS");
	return kIOReturnSuccess;
}

#if 0
int mhc_count = 0;
void SDDisk::taggedRetain(const void * tag) const {
	if (tag) {
		UTL_DEBUG_DEF("                          Retain! (0x%08x) (%d) (%d -> %d)",
			      (unsigned) reinterpret_cast<int64_t>(tag), getRetainCount(), mhc_count, mhc_count + 1);
		mhc_count++;
	}
	super::taggedRetain(tag);
}
void SDDisk::taggedRelease(const void * tag, const int when) const {
	super::taggedRelease(tag, when);
	if (tag) {
		mhc_count--;
		UTL_DEBUG_DEF("                          Release! (0x%08x) (%d) (%d -> %d)",
			      (unsigned) reinterpret_cast<int64_t>(tag), getRetainCount(), mhc_count + 1, mhc_count);
	}
}
#endif // DEBUG
