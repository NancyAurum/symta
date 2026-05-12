#if 1
#DBG([xs]) 
#VDBG(text,v)
#else
#DBG([xs]) if (bug) fprintf(stderr, xs)
#VDBG(text,v) \
  if (bug) fprintf(stderr, text ": %f,%f,%f\n",(v)[0],(v)[1],(v)[2])
#endif

extern int bug;
