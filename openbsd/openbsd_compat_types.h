#ifndef SINETEK_RTSX_OPENBSD_COMPAT_TYPES_H
#define SINETEK_RTSX_OPENBSD_COMPAT_TYPES_H

#include <libkern/libkern.h> // size_t

typedef u_long bus_addr_t;
typedef void *bus_dmamap_callback_t; // don't know the type yet!
typedef void *bus_dmasync_op_t; // don't know the type yet!
typedef void *bus_dma_segment_t; // don't know the type yet!
typedef void *bus_dma_tag_t; // don't know the type yet!
typedef u_long bus_size_t; // don't know the type yet!
typedef void *bus_space_handle_t; // don't know the type yet!
typedef void *bus_space_tag_t; // don't know the type yet!

struct rwlock {
	int dummy;
};

struct IOBufferMemoryDescriptor; // forward declaration

typedef struct {
	// PUBLIC MEMBERS
	uint64_t	ds_addr;
	uint64_t	ds_len;
} bus_dma_segment_t;

typedef struct {
	

#endif // SINETEK_RTSX_OPENBSD_COMPAT_TYPES_H
