// 17-bits.s -- bit operations and integer formatting
//
// Symta uses an unusual notation for bit ops to keep `+ - * / %`
// available as the normal arithmetic operators. The bit ops are:
//
//   A -*- B    bitwise AND
//   A -+- B    bitwise OR (inclusive)
//   A -^- B    bitwise XOR
//   A -<- N    shift A left by N bits
//   A ->- N    shift A right by N bits
//
// `int.x` formats as hex (uppercase, no `0x` prefix).
//
// Run:  symta -f examples/17-bits.s


// ----------------------------------------------------------------
// 1. The five operators on small numbers.
// ----------------------------------------------------------------
say "AND  0xFF -*- 0x0F = [(0xFF -*- 0x0F).x]    (= 0F)"
say "OR   0x10 -+- 0x01 = [(0x10 -+- 0x01).x]    (= 11)"
say "XOR  0xFF -^- 0x0F = [(0xFF -^- 0x0F).x]    (= F0)"
say "SHL  1 -<- 4       = [(1 -<- 4).x]          (= 10)"
say "SHR  256 ->- 4     = [(256 ->- 4).x]        (= 10)"
say ""


// ----------------------------------------------------------------
// 2. Packing 4 bytes (R, G, B, A) into a 32-bit integer and
//    unpacking them back. This is the canonical pattern for
//    storing pixels, flag sets, and tagged values.
// ----------------------------------------------------------------
pack_rgba R G B A =
  (A -<- 24) -+- (R -<- 16) -+- (G -<- 8) -+- B

unpack_rgba V =
  Alpha (V ->- 24) -*- 0xFF
  Red   (V ->- 16) -*- 0xFF
  Green (V ->- 8)  -*- 0xFF
  Blue  V -*- 0xFF
  [Red Green Blue Alpha]

C pack_rgba 0xA0 0x40 0xC0 0xFF
say "packed:   [C.x]"
[R G B A] unpack_rgba C
say "unpacked: R=[R.x] G=[G.x] B=[B.x] A=[A.x]"
say ""


// ----------------------------------------------------------------
// 3. Bitfield flags. A door is 8 boolean attributes packed into
//    one int. Set with -+-, clear with -*- of the inverted mask,
//    test with -*-, toggle with -^-.
// ----------------------------------------------------------------
FLAG_OPEN     1
FLAG_LOCKED   2
FLAG_TRAPPED  4
FLAG_SECRET   8

Door 0
Door = Door -+- FLAG_OPEN
Door = Door -+- FLAG_LOCKED
say "door bits = [Door.x]"
say "open?    = [Door -*- FLAG_OPEN]"
say "trapped? = [Door -*- FLAG_TRAPPED]"

// toggle locked
Door = Door -^- FLAG_LOCKED
say "after toggle locked: [Door.x] (open=[Door -*- FLAG_OPEN] locked=[Door -*- FLAG_LOCKED])"
say ""


// ----------------------------------------------------------------
// 4. Hex / int parsing. Symta's `text.int` takes an optional radix
//    argument (`text.int 16` for hex, `text.int 2` for binary).
// ----------------------------------------------------------------
H "CAFEBABE"
N H.int 16
say "[H] -> [N] -> back to hex: [N.x]"

B "10110011"
say "[B] (binary) -> [B.int 2]"

Pi 3.14159265
say "round: [(Pi*100.0).int]"      // truncate toward zero
say ""


// ----------------------------------------------------------------
// 5. Common helpers from the standard library.
// ----------------------------------------------------------------
say "(-7).abs    = [(-7).abs]"
say "(-7).sign   = [(-7).sign]   (3.sign = [3.sign]   0.sign = [0.sign])"
say "12.clip 0 5 = [12.clip 0 5]"
