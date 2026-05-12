#cut_body() {
if (cut) {
  if (c==1) goto do_cut_term;
  cuts_count++;
  if (cuts_count != voxel_count) goto do_split_term;
}
continue;
}

#cut_tail() {
  if (cuts_count) {
    //FIXME: handle interior clay
#if #PASTE_COLOR
do_cut_term:
    if (c == 1) {
      seg.term_set(t,paste_color);
    }
#else
    if (cuts_count == voxel_count) {
do_cut_term:
      seg.term_erase(t);
    }
#endif
    else {
do_split_term:
      seg.term_split(t,terms);
    }
  }
}
