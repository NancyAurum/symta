// NCM-3 regression: cmd_fmt's decimal parser used to stop at 'x',
// so `#("%d" 0xFF)` read only the leading 0.  Now 0x / 0X triggers
// base-16 parsing in the input.  Decimal path unchanged.
//
// Output case convention: matches C's printf.
//   %x  -> lowercase hex
//   %X  -> uppercase hex
// Pre-May 2026 these were inverted; fixed once the audit confirmed
// no source-code callers depended on the inversion.
hex_in_X  = #("0x%02X" 0xFF)
hex_in_x  = #("0x%02x" 0xFF)
dec_of_hex = #("%d" 0x10)
neg_hex    = #("%d" -0x10)
big_hex_X  = #("0x%08X" 0xFEEDFACE)
big_hex_x  = #("0x%08x" 0xFEEDFACE)
mix        = #("%d + %d = %d" 0x10 0x20 0x30)
