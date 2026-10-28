$ErrorActionPreference = 'Stop'

$outDir = Join-Path $PSScriptRoot 'out'
$objectDir = Join-Path $outDir 'obj'
$tbdfPrefix = Join-Path $outDir 'tbdf-wasm'
$tbdfCache = Join-Path $outDir 'zig-cache'
$tbdfGlobalCache = Join-Path $outDir 'zig-global-cache'
$vendorDir = Join-Path $PSScriptRoot 'vendor'
$tbdfDir = Join-Path $vendorDir 'tbdf-v1'
$tfafDir = Join-Path $vendorDir 'tfaf-gen1'
$clang = (Get-Command clang -ErrorAction Stop).Source
$zig = (Get-Command zig -ErrorAction Stop).Source

New-Item -ItemType Directory -Force -Path $objectDir | Out-Null

Push-Location $tbdfDir
try {
  & $zig build -Dtarget=wasm32-freestanding -Doptimize=ReleaseSmall `
    --prefix $tbdfPrefix --cache-dir $tbdfCache `
    --global-cache-dir $tbdfGlobalCache
  if ($LASTEXITCODE -ne 0) { throw 'TBDF WASM library build failed.' }
}
finally {
  Pop-Location
}

$sources = @(
  (Join-Path $PSScriptRoot 'src/boot.c'),
  (Join-Path $PSScriptRoot 'src/boot_memory.c'),
  (Join-Path $PSScriptRoot 'src/boot_disk.c'),
  (Join-Path $PSScriptRoot 'src/text_display.c'),
  (Join-Path $PSScriptRoot 'src/display_ttd.c'),
  (Join-Path $PSScriptRoot 'src/fs_tbdf.c'),
  (Join-Path $PSScriptRoot 'src/fs_tfaf.c'),
  (Join-Path $tfafDir 'src/tfaf1.c')
)
$includeFlags = @(
  '-I', (Join-Path $PSScriptRoot 'include'),
  '-I', (Join-Path $tbdfDir 'include'),
  '-I', (Join-Path $tfafDir 'include')
)
$objects = @()
foreach ($source in $sources) {
  $object = Join-Path $objectDir (([IO.Path]::GetFileNameWithoutExtension($source)) + '.o')
  & $clang -target wasm32-unknown-unknown -std=c11 -Oz -Wall -Wextra `
    -Werror -ffreestanding -fno-builtin -nostdlib @includeFlags `
    -c $source -o $object
  if ($LASTEXITCODE -ne 0) { throw "C compilation failed: $source" }
  $objects += $object
}

$tbdfLibrary = Join-Path $tbdfPrefix 'lib/libtbdfv1.a'
$output = Join-Path $outDir 'bootloader.wasm'
& $clang -target wasm32-unknown-unknown -Oz -nostdlib @objects $tbdfLibrary `
  '-Wl,--no-entry' '-Wl,--export=_start' '-Wl,--export-memory' `
  '-Wl,-z,stack-size=32768' '-Wl,--max-memory=33554432' `
  -o $output
if ($LASTEXITCODE -ne 0) { throw 'Bootloader WASM link failed.' }

Write-Output "Built $output"
