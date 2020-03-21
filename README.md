# Sintekek-rtsx

This is a try to make this kext working on my laptop (Dell XPS 9350). All credits are due to the original authors, @sinetek and @syscl.

## Overal notes

It took me a while to understand the code. Some problems I found:

1. We need a way to wait for the interrupt while in `workloop_`. The solution adopted is to have two workloops, the normal `workloop_` and a serate workloop for tasks. I'm not sure whether this approach is right or not (an IOCommandGate with one single workloop seems the way to go), but for now it seems to work.

## Changes made

### Options

Used by `#define`/`#if <option>`:

| Option                   | Requires          | Notes                                                                                                                       |
|--------------------------|-------------------|-----------------------------------------------------------------------------------------------------------------------------|
| `RTSX_FIX_TASK_BUG`      |                   | Fix task bug (reusing task pointer instead of allocating it).                                                               |
| `RTSX_USE_WRITEBYTES`    |                   | Use `IOMemoryDescriptor::writeBytes()` method to write buffer instead of the original `buffer->map()->getVirtualAddress()`. |
| `RTSX_USE_IOLOCK`        |                   | This should use more locks to protect critical sections.                                                                    |
| `RTSX_USE_IOCOMMANDGATE` | `RTSX_USE_IOLOCK` | A try to make `IOCommandGate` working, but never really worked.                                                             |
| `RTSX_USE_IOMALLOC`      |                   | Use `IOMalloc`/`IOFree` for memory management instead of new/delete.                                                        |

## To Do

 - Multi-block read?
 - Use command gate instead of two workloops? Is it even possible?
 - Once a disk is mounted, the kext cannot be unloaded (check why).
