// NCM-3 regression: cmd_fmt's decimal parser used to stop at 'x',
// so `#("%d" 0xFF)` read only the leading 0.  Now 0x / 0X triggers
// base-16 parsing in the input.  Decimal path unchanged.
//
// Output-case convention: NCM has %x lowercase-->UPPERCASE and
// %X UPPERCASE-->lowercase (inverted from C's printf).  Pinned here
// so the inversion doesn't get "corrected" by accident -- see
// dev/ncm.md for the rationale.
hex_in_X  = #("0x%02X" 0xFF)
hex_in_x  = #("0x%02x" 0xFF)
dec_of_hex = #("%d" 0x10)
neg_hex    = #("%d" -0x10)
big_hex    = #("0x%08X" 0xFEEDFACE)
mix        = #("%d + %d = %d" 0x10 0x20 0x30)
