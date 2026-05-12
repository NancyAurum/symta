// reader.h -- public surface of the C parser port.
//
// Each entry point mirrors a Symta-level function from
// symta/src/reader.s. The Symta side is being gradually replaced
// by these C implementations; see issues.md B8.
//
// All entry points take/return Symta heap objects (`dyn`) and run
// with GC disabled internally so the caller doesn't have to worry
// about pointer relocation.

#ifndef SYMTA_READER_H
#define SYMTA_READER_H

#include "symta.h"

// reader.s:add_bars -- prepends a `|` token before every
// line-starting non-continuation token, plus one at the front.
// Returns a fresh T_LIST.
dyn reader_add_bars(dyn token_list);

// reader.s:parse_strip -- walks the token+list AST returned by
// parse_tokens and unwraps tokens to leave just .value (or the
// parsed form if .parsed is set). Lists are recursively stripped
// into fresh T_LIST objects. Does NOT attach source meta (the
// Symta-level wrapper handles that downstream).
dyn reader_parse_strip(dyn ast);

#endif
