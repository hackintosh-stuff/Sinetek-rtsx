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

	card_is_write_protected_ = true;
	sdmmc_softc_ = sc_sdmmc;

	UTL_DEBUG_FUN("END");
	return true;
}

void SDDisk::free()
{
	UTL_DEBUG_FUN("START");
	sdmmc_softc_ = NULL;
	super::free();
	UTL_LOG("SDDisk freed.");
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

	printf("rtsx: attaching SDDisk, num_blocks:%d  blk_size:%d\n",
	       num_blocks_, blk_size_);

	// check whether the card is write-protected
	card_is_write_protected_ = provider_->cardIsWriteProtected();

	UTL_LOG("SDDisk attached%s", card_is_write_protected_ ? " (card is write-protected)" : "");
	return true;
}

void SDDisk::detach(IOService* provider)
{
	UTL_LOG("SDDisk detaching...");
	UTL_SAFE_RELEASE_NULL(provider_);

	super::detach(provider);
	UTL_LOG("SDDisk detached (retainCount=%d).", this->getRetainCount());
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
	UTL_DEBUG_FUN("CALLED");
	*isWriteProtected = !provider_->writeEnabled() || card_is_write_protected_;
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
	UTL_DEBUG_FUN("START (%s block = %u nblks = %u blksize = %u physSectSize = %u)",
		      args->direction == kIODirectionIn ? "READ" : "WRITE",
		      static_cast<unsigned>(args->block),
		      static_cast<unsigned>(args->nblks),
		      args->that->blk_size_,
		      sdmmc->sc_fn0->csd.sector_size);

	actualByteCount = args->nblks * args->that->blk_size_;
	IOByteCount maxSendBytes = 128 * 1024;
	IOByteCount remainingBytes = args->nblks * 512;
	IOByteCount sentBytes = 0;
	int blocks = (int) args->block;

#if RTSX_USE_WRITEBYTES
	u_char *buf = new u_char[actualByteCount];
#else
	auto map = args->buffer->map();
	u_char *buf = (u_char *) map->getAddress();
#endif

	while (remainingBytes > 0) {
		IOByteCount sendByteCount = remainingBytes > maxSendBytes ? maxSendBytes : remainingBytes;

		if (args->direction == kIODirectionIn) {
			error = sdmmc_mem_read_block(sdmmc->sc_fn0, blocks, buf + sentBytes, sendByteCount);
			if (error)
				break;
#if RTSX_USE_WRITEBYTES
			IOByteCount copied_bytes = args->buffer->writeBytes(sentBytes, buf + sentBytes, sendByteCount);
			if (copied_bytes == 0) {
				error = EIO;
				break;
			}
#endif
		} else {
#if RTSX_USE_WRITEBYTES
			IOByteCount copied_bytes = args->buffer->readBytes(sentBytes, buf + sentBytes, sendByteCount);
			if (copied_bytes == 0) {
				error = EIO;
				break;
			}
#endif
			error = sdmmc_mem_write_block(sdmmc->sc_fn0, blocks, buf + sentBytes, sendByteCount);
			if (error)
				break;
		}
		blocks += (sendByteCount / 512);
		remainingBytes -= sendByteCount;
		sentBytes += sendByteCount;
	}
#if RTSX_USE_WRITEBYTES
	delete[] buf;
#else
	map->release();
#endif
	if (args->completion.action) {
		if (error == 0) {
			(args->completion.action)(args->completion.target, args->completion.parameter,
						  kIOReturnSuccess, actualByteCount);
		} else {
			UTL_ERR("Returning an IO Error!"
				" (%s block = %u nblks = %u blksize = %u physSectSize = %u error = %d)",
				args->direction == kIODirectionIn ? "READ" : "WRITE",
				static_cast<unsigned>(args->block),
				static_cast<unsigned>(args->nblks),
				args->that->blk_size_,
				sdmmc->sc_fn0->csd.sector_size, error);
			(args->completion.action)(args->completion.target, args->completion.parameter,
						  kIOReturnIOError, 0);
		}
	} else {
		UTL_ERR("No completion action!");
	}
	delete args;
	UTL_DEBUG_FUN("END (error = %d)", error);
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

	if (!provider_->writeEnabled() && direction == kIODirectionOut)
		return kIOReturnNotWritable; // read-only driver

	if ((block + nblks) > num_blocks_)
		return kIOReturnBadArgument;

	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;

	// printf("=====================================================\n");
	UTL_DEBUG_DEF("START (block=%d nblks=%d)", (int) block, (int) nblks);

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
