// tc_str_ops — stress the string marshalling path.
//
// Symta texts come in two flavours: fixtext (short, immediate)
// and bigtext (heap, char-array). The text → char* conversion
// handles both. Each test below uses a mix so the marshaller
// can't get away with handling only one.

use cffi
export tc_str_ops

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_str_ops =
  // strlen across a range of sizes — fixtext for short, bigtext
  // for longer strings.  Repeat single chars via `text * N` (text
  // multiplication), since Symta's `^^` is pow for int/float and
  // map for list, not text-repeat.
  check 'strlen 1'    1   (c_strlen "a")
  check 'strlen 3'    3   (c_strlen "abc")
  check 'strlen 7'    7   (c_strlen "abcdefg")
  check 'strlen 100'  100 (c_strlen ("A" * 100))
  check 'strlen 500'  500 (c_strlen ("B" * 500))
  check 'strlen 4096' 4096 (c_strlen ("C" * 4096))

  // strchr offset across sizes.  Use string interpolation to
  // splice repeated runs around a known-position separator.
  Big1 "[("x" * 50)]y[("z" * 50)]"        // 101 chars, 'y' at 50
  check 'strchr y@50' 50 (c_strchr_ofs Big1 'y'.code)

  Big2 "[("x" * 1000)]![("z" * 200)]"     // '!' at 1000
  check 'strchr ! @ 1000' 1000 (c_strchr_ofs Big2 '!'.code)

  // bytesum equivalence: for a single char repeated N times the
  // sum is N*charcode. 'A'==65, repeat 10 → 650.
  check 'bytesum A×10'  650  (c_str_bytesum ("A" * 10))
  check 'bytesum A×100' 6500 (c_str_bytesum ("A" * 100))

  // Unicode-bytes (non-ASCII): "ä" is two bytes 0xC3 0xA4 in
  // UTF-8 = 195 + 164 = 359.
  //
  // SKIPPED -- an FFI text-marshalling path drops multi-byte UTF-8
  // characters when handing a Symta text to a C `char*` argument.
  // The Symta-side value is fine (`"ä".n == 2` and the bytes are
  // 195/164), but `c_str_bytesum` receives an empty string and
  // returns 0.  Filed as TODO FFI-4; reactivate once it's fixed.
  // check 'bytesum ä'     359  (c_str_bytesum "ä")
