// tc_zlib — call into zlib (vendored, lives in symta/ffi/zlib.ffi).
//
// zlib is a richer real-library test than libc because it
// exercises ptr-with-in/out-param patterns: `compress` takes
// `uLong *dstLen` which both reads (capacity in) and writes
// (compressed-size out). If the FFI's pointer marshalling
// loses bits, the value the C side writes back is unreadable
// from Symta.
//
// The example in examples/13-ffi already demonstrates these
// patterns; here we use them as assertions instead of
// demonstrations.

use cls zlib

export tc_zlib

ffi_begin local zlib

// const char *zlibVersion(void);
ffi zlibVersion.text

// uLong crc32(uLong crc, const Bytef *buf, uInt len);
ffi crc32.u4 Crc.u4 Buf.ptr Len.u4

// int compress  (Bytef *dst, uLong *dstLen, const Bytef *src, uLong srcLen);
// int uncompress(Bytef *dst, uLong *dstLen, const Bytef *src, uLong srcLen);
ffi compress.int   Dst.ptr DstLen.ptr Src.ptr SrcLen.u4
ffi uncompress.int Dst.ptr DstLen.ptr Src.ptr SrcLen.u4

crc_text Text =
  Bs Text.utf8
  N Bs.n
  P ffi_alloc N
  for I,B Bs.i: _ffi_set uint8_t P I B
  R crc32 0 P N
  ffi_free P
  R

tc_zlib =
  V zlibVersion()
  if V.n > 0
    then say "PASS zlib version = [V]"
    else say "FAIL zlib version came back empty"

  // CRC-32 of "Hello, World!" is a documented constant.
  Crc crc_text "Hello, World!"
  if Crc >< 0xEC4AC3D0
    then say "PASS crc32('Hello, World!') = 0xEC4AC3D0"
    else say "FAIL crc32('Hello, World!') = [Crc.x] (expected EC4AC3D0)"

  // Round-trip: compress + uncompress should give back the input.
  Original "Some text to compress and decompress through zlib"
  In Original.utf8
  InN In.n
  InP ffi_alloc InN
  for I,B In.i: _ffi_set uint8_t InP I B

  Cap InN*2 + 64
  OutP ffi_alloc Cap
  LenP ffi_alloc 4
  _ffi_set uint32_t LenP 0 Cap
  Rc compress OutP LenP InP InN
  CompLen _ffi_get uint32_t LenP 0
  if Rc <> 0:
    say "FAIL zlib compress() returned [Rc]"
    ret

  // Decompress.
  Out ffi_alloc InN
  _ffi_set uint32_t LenP 0 InN
  Rc2 uncompress Out LenP OutP CompLen
  OutLen _ffi_get uint32_t LenP 0
  if Rc2 <> 0:
    say "FAIL zlib uncompress() returned [Rc2]"
    ret

  // Verify the decompressed bytes match.
  Match 1
  for I InN:
    A _ffi_get uint8_t InP I
    B _ffi_get uint8_t Out I
    if A <> B: Match = 0
  if Match and OutLen >< InN
    then say "PASS zlib compress + uncompress round-trip"
    else say "FAIL zlib round-trip mismatch (OutLen=[OutLen] InN=[InN])"

  ffi_free InP
  ffi_free OutP
  ffi_free Out
  ffi_free LenP
