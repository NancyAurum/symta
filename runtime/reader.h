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
dyn reader_add_bars(dyn token_list);

// reader.s:parse_strip -- walks the token+list AST and unwraps
// tokens to leave values (or the parsed form if .parsed is set).
// Does NOT attach source meta -- the Symta-level wrapper handles
// that downstream.
dyn reader_parse_strip(dyn ast);

// reader.s:parse_table -- given a stripped list of items where some
// are `[`!` K V]` pairs, return the [[K V] ...] table builder list.
dyn reader_parse_table(dyn xs);

// reader.s:parse_tokens (in progress) -- token list -> raw AST with
// tokens at the leaves. Currently incomplete: handles the precedence
// ladder + parse_term's literal/list cases. parse_if, parse_bar,
// parse_offside fall back to errors. Not yet wired into text.parse.
dyn reader_parse_tokens(dyn token_list);

#endif
