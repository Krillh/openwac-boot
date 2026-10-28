# OpenWAC Bootloader

This directory contains the freestanding C bootloader implementation. The
loader selects the first usable supported text display, searches disks for a
root-level `kernel.x`, and installs the first valid image as the computer's
kernel. See [PROJECT_PLAN.md](PROJECT_PLAN.md) for the design and acceptance
goals.

## Layout

- `src/boot.c` coordinates boot, format preference, kernel installation, and
  fatal handling.
- `src/boot_disk.c` provides disk discovery, geometry, and checked block reads.
- `src/fs_tbdf.c` and `src/fs_tfaf.c` adapt the common block interface to the
  read-only TBDF and TFAF readers.
- `src/text_display.c` defines display discovery and the backend registry;
  `src/display_ttd.c` implements append-only TTD output.
- `src/boot_memory.c` provides the page-growing arena and bounded image buffer.

The bootloader searches all disks for TBDF before trying TFAF, retaining only
one disk descriptor at a time. It does not use root-disk markers, format or
write disks, read file importers, or spawn the OS.

## Build

From PowerShell, with Clang and Zig on `PATH`, run:

```powershell
.\build.ps1
```

The required TBDF v1 and TFAF1 sources are vendored under `vendor/`, so the
build does not depend on sibling repositories or network access. Clang compiles
and links the freestanding C bootloader and TFAF core. Zig builds the vendored
TBDF library because that v1 implementation is written in Zig. The result is
`out/bootloader.wasm`, which exports `_start` and `memory` and imports only the
privileged OpenWAC HAL calls used by the loader. See [vendor/README.md](vendor/README.md)
for source revisions. TBDF's build requires Zig 0.18.0-dev.253 or newer.

## Memory behavior

The reusable arena contains TBDF volume storage, block scratch, and the kernel
image buffer. The image buffer is page-aligned, grows in place, and is limited
to four WASM pages (262,144 bytes). A geometry pass finds the largest attached
supported disk block size before allocating one reusable scratch area. To keep
scratch within one WASM page, disks with blocks larger than 65,472 bytes are
skipped. No disk UUID list, whole-disk image, or filesystem metadata copy is
retained by the bootloader.

The target is at most four pages of loader overhead, excluding the image
buffer; below two pages is the stretch goal, and one page is an aspiration.
These overhead goals are not enforced until measured. If the overhead target is
met, the four-page image bound gives an eight-page total target. Larger kernels
are skipped in favor of later candidates.
