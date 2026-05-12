// 13-ffi -- calling C from Symta
//
// Symta has two FFI styles, both backed by C/Invoke for portable
// argument marshalling:
//
//   1. Declarative -- `ffi_begin <lib>` followed by typed `ffi`
//      lines. Compact, integrates with the build, gives you Symta
//      functions that look just like ordinary calls.
//
//   2. Low-level -- `ffi_load` + `_ffi_call`. Useful when you only
//      need a single call and don't want to declare a binding, or
//      when you build the FFI table at runtime.
//
// In both cases the runtime looks for `ffi/<lib>.ffi` (a renamed
// DLL/dylib) under the project root. We bundle a copy of zlib1.dll
// as `ffi/zlib.ffi` to make this example self-contained.
//
// Run:  symta examples/13-ffi && examples/13-ffi/go.exe


// ----------------------------------------------------------------
// 1. Declarative bindings against zlib. The first arg is a mode
// flag (`export` to also export the bindings; bare keyword to keep
// them private to this module).
// ----------------------------------------------------------------
ffi_begin local zlib

// const char *zlibVersion(void);
ffi zlibVersion.text

// uLong crc32(uLong crc, const Bytef *buf, uInt len);
ffi crc32.u4 Crc.u4 Buf.ptr Len.u4

// int compress  (Bytef *dst, uLong *dstLen, const Bytef *src, uLong srcLen);
// int uncompress(Bytef *dst, uLong *dstLen, const Bytef *src, uLong srcLen);
ffi compress.int   Dst.ptr DstLen.ptr Src.ptr SrcLen.u4
ffi uncompress.int Dst.ptr DstLen.ptr Src.ptr SrcLen.u4


say "linked against zlib [zlibVersion]"
say ""


// ----------------------------------------------------------------
// 2. CRC-32 of "Hello, World!". The pattern is generic for any
// "byte buffer" C call: alloc raw memory, fill it byte by byte
// using `_ffi_set uint8_t`, call C, free.
// ----------------------------------------------------------------
crc_text Text =
  Bs Text.utf8                            // text -> list of utf-8 bytes
  N Bs.n
  P ffi_alloc N
  for I,B Bs.i: _ffi_set uint8_t P I B
  R crc32 0 P N
  ffi_free P
  R

CRC crc_text "Hello, World!"
say "crc32(\"Hello, World!\") = [CRC.x]    (expected ec4ac3d0)"
say ""


// ----------------------------------------------------------------
// 3. Round-trip: compress, then uncompress, then compare.
// `compress` writes the actual output length back through a
// `uLong *dstLen` -- so we allocate a 4-byte cell and read it back
// with `_ffi_get uint32_t`.
// ----------------------------------------------------------------
copy_to_ptr Bytes =
  N Bytes.n
  P ffi_alloc N
  for I,B Bytes.i: _ffi_set uint8_t P I B
  P,N

read_ptr_bytes P N = dup I N: _ffi_get uint8_t P I

zip_compress Text =
  // serialize input
  In Text.utf8
  [InP InN] copy_to_ptr In
  // zlib needs ~ srcLen*1.001 + 12 bytes for output
  Cap InN*2 + 64
  OutP ffi_alloc Cap
  // *DstLen = Cap (in/out parameter -- input: capacity)
  LenP ffi_alloc 4
  _ffi_set uint32_t LenP 0 Cap
  Rc compress OutP LenP InP InN
  CompLen _ffi_get uint32_t LenP 0
  // copy compressed bytes back to a Symta list
  Comp read_ptr_bytes OutP CompLen
  ffi_free InP
  ffi_free OutP
  ffi_free LenP
  if Rc <> 0: bad "compress() returned [Rc]"
  Comp

zip_uncompress Comp ExpectedSize =
  [InP InN] copy_to_ptr Comp
  OutP ffi_alloc ExpectedSize
  LenP ffi_alloc 4
  _ffi_set uint32_t LenP 0 ExpectedSize
  Rc uncompress OutP LenP InP InN
  Out _ffi_get uint32_t LenP 0
  Bytes read_ptr_bytes OutP Out
  ffi_free InP
  ffi_free OutP
  ffi_free LenP
  if Rc <> 0: bad "uncompress() returned [Rc]"
  Bytes

Original "Symta talks to C via C/Invoke trampolines built at runtime."
say "original    ([Original.utf8.n] B): [Original]"

Compressed zip_compress Original
HexDump Compressed{?.x}.text(' ')
say "compressed ([Compressed.n] B): [HexDump]"

Bytes zip_uncompress Compressed (Original.utf8.n)
Decoded Bytes.utf8
say "decompressed: [Decoded]"
say ""

say "round-trip OK? [Decoded >< Original]"


// ----------------------------------------------------------------
// 4. Low-level form. Same crc32 call without `ffi_begin`/`ffi`.
// ----------------------------------------------------------------
&Crc32Fn ffi_load \zlib \crc32

raw_crc Text =
  Bs Text.utf8
  N Bs.n
  P ffi_alloc N
  for I,B Bs.i: _ffi_set uint8_t P I B
  R _ffi_call \(u4 u4 ptr u4) Crc32Fn 0 P N
  ffi_free P
  R

CrcRaw raw_crc "Hello, World!"
say "via ffi_load:  [CrcRaw.x]"
