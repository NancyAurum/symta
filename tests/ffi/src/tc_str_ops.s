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
  // for longer strings.
  check 'strlen 1'    1   (c_strlen "a")
  check 'strlen 3'    3   (c_strlen "abc")
  check 'strlen 7'    7   (c_strlen "abcdefg")
  check 'strlen 100'  100 (c_strlen ('A'^^100).text)
  check 'strlen 500'  500 (c_strlen ('B'^^500).text)
  check 'strlen 4096' 4096 (c_strlen ('C'^^4096).text)

  // strchr offset across sizes.
  Big1 ('x'^^50).text^"y"^('z'^^50).text   // 101 chars, 'y' at 50
  check 'strchr y@50' 50 (c_strchr_ofs Big1 'y'.code)

  Big2 ('x'^^1000).text^"!"^('z'^^200).text // 'y' missing, '!' at 1000
  check 'strchr ! @ 1000' 1000 (c_strchr_ofs Big2 '!'.code)

  // bytesum equivalence: for a single char repeated N times the
  // sum is N*charcode. 'A'==65, repeat 10 → 650.
  check 'bytesum A×10'  650  (c_str_bytesum ('A'^^10).text)
  check 'bytesum A×100' 6500 (c_str_bytesum ('A'^^100).text)

  // Unicode-bytes (non-ASCII): "ä" is two bytes 0xC3 0xA4 in
  // UTF-8 = 195 + 164 = 359.
  check 'bytesum ä'     359  (c_str_bytesum "ä")
