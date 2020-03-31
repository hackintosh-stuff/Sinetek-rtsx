#include "openbsd_compat_bus_space.h"

#include <IOKit/IOMemoryDescriptor.h>

#define UTL_THIS_CLASS ""
#include "util.h"

/// We will use bus_space_handle_t as a IOMemoryDescriptor
typedef IOMemoryDescriptor *_bus_space_handle_t;

u_int32_t bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset)
{
	u_int32_t ret;
	_bus_space_handle_t memDesc = (_bus_space_handle_t) handle;
	UTL_CHK_PTR(memDesc, 0);
	memDesc->readBytes((IOByteCount) offset, &ret, 4);
	return ret;
}

void bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, u_int32_t value)
{
	_bus_space_handle_t memDesc = (_bus_space_handle_t) handle;
	UTL_CHK_PTR(memDesc,);
	memDesc->writeBytes((IOByteCount) offset, &value, 4);
}
