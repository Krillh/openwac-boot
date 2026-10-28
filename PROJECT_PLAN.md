# OpenWAC Bootloader Project Plan

## Purpose

Create a small, freestanding C bootloader for OpenWAC. At startup it discovers
a primary text display when one is available, searches attached disks for a
root-level file named exactly `kernel.x`, and installs the first usable image
through the privileged `hal_install_kernel` HAL call.

The bootloader does not format or write to disks, use file importers, or start
the operating system. Those responsibilities are outside this loader's boot
contract.

## Project Organization

Keep the implementation split into focused modules rather than a single C
file:

- Boot orchestration coordinates discovery, search, loading, progress, and
  installation.
- HAL and block-device modules enumerate devices and provide checked,
  read-only block access.
- Format adapters expose the same small file-reading interface for TBDF and
  TFAF. Use the vendored [TBDF C API](vendor/tbdf-v1/include/tbdfv1.h) and
  [TFAF1 C API](vendor/tfaf-gen1/include/tfaf1.h) so builds do not depend on
  sibling repositories.
- A text-display abstraction provides boot progress and error output. A TTD
  adapter implements it first; adding another text-display interface should
  require a new adapter rather than changes to the boot flow.
- Kernel loading reads the selected file into a bounded contiguous buffer and
  passes it to `hal_install_kernel`.

## Discovery and Boot Flow

1. Discover the first usable text display in HAL enumeration order. Store only
   the selected display identity; do not retain a list of device UUIDs.
2. If a display is available, append progress messages without clearing or
   scrolling existing content. The TTD adapter should find an append position
   in the current contents so existing text remains visible. Keep progress
   output bounded and leave room for a final error message.
3. Enumerate disks in HAL order and search for TBDF media first. Then enumerate
   them again and search for TFAF media. Do not snapshot all disk identities;
  this trades additional enumeration work for lower memory use. Bound the
  reusable block scratch to one WASM page, and skip disks whose block size
  exceeds the TBDF scratch requirement that fits in that page.
4. In each supported format, look for an exact, root-level `kernel.x`. Skip
   missing files, unsupported or invalid media, read failures, oversized
   images, and candidate images rejected by kernel installation. Continue to
   the next candidate. Stop on a global installation failure that another
   candidate cannot resolve.
5. Return from `_start` after a successful kernel installation.

## Memory Goals

Count memory in 64 KiB WASM pages. The primary goal is to keep loader overhead,
excluding the kernel image buffer, at or below 4 pages. Treat that as an
optimization target rather than a hard limit until implementation measurements
show it is feasible. The stretch goal is below 2 overhead pages; using 1 page
would be an exceptional result.

The kernel image buffer has a hard limit of 4 pages (262,144 bytes), allocated
only when needed. When overhead meets its 4-page target, the expected worst
case is therefore 8 pages total. Images larger than the buffer limit are
rejected and search continues. Measure peak memory by boot stage on
representative TBDF and TFAF media; avoid whole-disk copies and device-list
buffers.

## Display and Failure Behavior

Progress output is append-only: do not clear the display or erase earlier
stages. The selected display is used for the final error message when a boot
failure occurs. After attempting to print the error, enter `panic()`, an
infinite yielding loop that leaves the display available for inspection.

If no primary display was found, errors call `abort()`. In this freestanding
bootloader, `abort()` calls `__builtin_trap()`.

## Acceptance Checks

- The first usable text display is selected in HAL enumeration order, and
  booting can proceed without any display.
- Progress lines are appended without clearing earlier output; a displayed
  boot error remains visible while `panic()` yields indefinitely.
- TBDF candidates are searched before TFAF candidates, with HAL order
  preserved within each pass and no stored disk UUID list.
- Only an exact root-level `kernel.x` is eligible. Missing, malformed,
  unreadable, oversized, or rejected candidates do not prevent trying later
  candidates.
- A valid candidate is installed once. Headless errors reach `abort()` and
  trap.
- Memory measurements report overhead separately from the image buffer and
  evaluate the 4-page overhead goal, the below-2-page stretch goal, and the
  1-page aspiration.
