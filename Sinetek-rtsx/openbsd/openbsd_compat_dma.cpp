#include "openbsd_compat_dma.h"

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOMemoryDescriptor.h> // IOMemoryMap

#define UTL_THIS_CLASS ""
#include "util.h"

static inline IOBufferMemoryDescriptor *TO_IO(bus_dmamap_t dmamp) {
	return (IOBufferMemoryDescriptor *) dmamp;
}

typedef struct {
	IOBufferMemoryDescriptor *memoryDescriptor;
	IOMemoryMap              *memoryMap;
} _bus_dma_tag;

_bus_dma_tag _busDmaTag = { nullptr, nullptr };
bus_space_tag_t gBusSpaceTag = {};
bus_space_handle_t gBusSpaceHandle = {};
bus_dma_tag_t gBusDmaTag = (bus_dma_tag_t) &_busDmaTag;

// bus_dmamap_create();         /* get a dmamap to load/unload          */
// for each DMA xfer {
//         bus_dmamem_alloc();  /* allocate some DMA'able memory        */
//         bus_dmamem_map();    /* map it into the kernel address space */
//         /* Fill the allocated DMA'able memory with whatever data
//          * is to be sent out, using the pointer obtained with
//          * bus_dmamem_map(). */
//         bus_dmamap_load();   /* initialize the segments of dmamap    */
//         bus_dmamap_sync();   /* synchronize/flush any DMA cache      */
//         for (i = 0; i < dm_nsegs; i++) {
//                 /* Tell the DMA device the physical address
//                  * (dmamap->dm_segs[i].ds_addr) and the length
//                  * (dmamap->dm_segs[i].ds_len) of the memory to xfer.
//                  *
//                  * Start the DMA, wait until it's done */
//         }
//         bus_dmamap_sync();   /* synchronize/flush any DMA cache      */
//         bus_dmamap_unload(); /* prepare dmamap for reuse             */
//         /* Copy any data desired from the DMA'able memory using the
//          * pointer created by bus_dmamem_map(). */
//         bus_dmamem_unmap();  /* free kernel virtual address space    */
//         bus_dmamem_free();   /* free DMA'able memory                 */
// }
// bus_dmamap_destroy();        /* release any resources used by dmamap */

int
bus_dmamap_create(bus_dma_tag_t tag, bus_size_t size, int nsegments, bus_size_t maxsegsz,
		  bus_size_t boundary, int flags, bus_dmamap_t *dmamp) {
	UTL_DEBUG(1, "START");
	if (!dmamp) return kIOReturnBadArgument;

	bus_dmamap_t ret = UTL_MALLOC(bus_dmamap);
	if (!ret) return kIOReturnNoMemory;

	memset(ret, 0, sizeof(bus_dmamap));
	ret->_dm_size = size;
	ret->_dm_segcnt = nsegments;
	ret->_dm_maxsegsz = maxsegsz;
	ret->_dm_boundary = boundary;
	ret->_dm_flags = flags;

	*dmamp = ret;
	return 0;
}

void
bus_dmamap_destroy(bus_dma_tag_t tag, bus_dmamap_t dmamp) {
	UTL_DEBUG(1, "START");
	UTL_FREE(dmamp, _bus_dmamap);
}

int
bus_dmamap_load(bus_dma_tag_t tag, bus_dmamap_t dmam, void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	UTL_DEBUG(1, "START");
	return kIOReturnUnsupported;
}

void
bus_dmamap_unload(bus_dma_tag_t dmat, bus_dmamap_t map)
{
	UTL_DEBUG(1, "START");
	return;
}

void
bus_dmamap_sync(bus_dma_tag_t tag, bus_dmamap_t dmam, bus_addr_t offset, bus_size_t size, int ops)
{
	// This function should probably call prepare() / complete(), but we already call them in
	// bus_dmamem_alloc()/bus_dmamem_free()
	UTL_DEBUG(1, "START");
	return;
}

int
bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size, bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
		 int nsegs, int *rsegs, int flags)
{
	UTL_DEBUG(1, "START");
	if (nsegs != 1) return kIOReturnBadArgument; // only one supported for now
	_bus_dma_tag *_tag = reinterpret_cast<_bus_dma_tag *>(tag);
	UTL_CHK_PTR(_tag, kIOReturnBadArgument);
	UTL_CHK_PTR(segs, kIOReturnBadArgument);
	UTL_CHK_PTR(rsegs, kIOReturnBadArgument);

	if (_tag->memoryDescriptor) return kIOReturnUnsupported; // only one dma_alloc

	_tag->memoryDescriptor =
	IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
							 kernel_task,
							 kIODirectionInOut |
							 kIOMemoryPhysicallyContiguous |
							 kIOMapInhibitCache,
							 size,
							 0x00000000ffffffffull);
	if (!_tag->memoryDescriptor) return kIOReturnNoMemory;

	IOByteCount len;
	auto addr = _tag->memoryDescriptor->getPhysicalSegment(0, &len);
	if (len != size) {
		UTL_ERR("len (%d) != size (%d)", (int) len, (int) size);
		_tag->memoryDescriptor->release();
		_tag->memoryDescriptor = nullptr;
		return kIOReturnUnsupported;
	}
	segs[0].ds_addr = addr;
	segs[0].ds_len = len;

	// call prepare here? does this wire the pages?
	_tag->memoryDescriptor->prepare();
	return 0;
}

void
bus_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs)
{
	UTL_DEBUG(1, "START");
	UTL_CHK_PTR(segs,);
	if (nsegs != 1) return; // only one supported for now
	_bus_dma_tag *_tag = reinterpret_cast<_bus_dma_tag *>(tag);
	if (!_tag->memoryDescriptor) return;

	_tag->memoryDescriptor->complete();
	_tag->memoryDescriptor->release();
	_tag->memoryDescriptor = nullptr;
}

int
bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs, size_t size, caddr_t *kvap, int flags)
{
	UTL_DEBUG(1, "START");
	UTL_CHK_PTR(segs, kIOReturnBadArgument);
	if (nsegs != 1) return kIOReturnUnsupported; // only one supported for now
	_bus_dma_tag *_tag = reinterpret_cast<_bus_dma_tag *>(tag);
	UTL_CHK_PTR(_tag, kIOReturnBadArgument);
	UTL_CHK_PTR(kvap, kIOReturnBadArgument);

	if (!_tag->memoryDescriptor) return kIOReturnBadArgument;
	if (_tag->memoryMap) return kIOReturnUnsupported;

	_tag->memoryMap = _tag->memoryDescriptor->map();
	if (!_tag->memoryMap) {
		UTL_ERR("memoryDescriptor->map() returned null!");
		return kIOReturnNoMemory;
	}

	*kvap = (caddr_t) _tag->memoryMap->getAddress(); // getVirtualAddress();
	return kIOReturnSuccess;
}

void
bus_dmamem_unmap(bus_dma_tag_t tag, void *kva, size_t size)
{
	UTL_DEBUG(1, "START");
	_bus_dma_tag *_tag = reinterpret_cast<_bus_dma_tag *>(tag);

	_tag->memoryMap->release();
	_tag->memoryMap = nullptr;
}

uint64_t getPhysicalAddress(const bus_dmamap_t dmamp) {
	UTL_DEBUG(1, "START");
	return (uint64_t) TO_IO(dmamp)->getPhysicalAddress();
}
