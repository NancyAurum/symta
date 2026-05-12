#define TEST_SIZE 1024

void test_list_alloc() {
  int i, j, k;
  void *p = 0;
  void *q = 0;

  fprintf(stderr, "List alloc...\n");
  for (i = 1; i < TEST_SIZE; i++) {
    //fprintf(stderr, "i=%d\n", i);
    for (j = 0; j < TEST_SIZE; j++) {
      LIST(q, j);
      for (k = 0; k < j; k++) {
        LSET(q, k, FXN(k+666));
      }
      if (j == TEST_SIZE/2) {
        LIST(p, i);
        for (k = 0; k < i; k++) {
          LSET(p, k, FXN(k+123));
        }
      }
    }
    //ensure the p list allocated inside loop wasn't corrupted
    for (j = 0; j < i; j++) {
      if (LGET(p,j) != FXN(j+123)) {
        fprintf(stderr, "  FAILED\n");
        exit(-1);
      }
    }
  }
}

void test_text_alloc() {
  int i, j, k;
  dyn p = 0;
  dyn q = 0;
  dyn t = 0;
  char *tstr = "The quick brown fox jumps over the lazy dog";
  dyn ttxt = 0;
  TEXT(ttxt, tstr);

  fprintf(stderr, "Text alloc...\n");
  for (i = 1; i < TEST_SIZE; i++) {
    //fprintf(stderr, "i=%d\n", i);
    for (j = 0; j < TEST_SIZE; j++) {
      LIST(q, j);
      for (k = 0; k < j; k++) {
        LSET(q, k, FXN(k+666));
      }
      if (j == TEST_SIZE/2) {
        LIST(p, i);
        for (k = 0; k < i; k++) {
          TEXT(t, tstr);
          LSET(p, k, t);
        }
      }
    }
    //ensure the p list allocated inside loop wasn't corrupted
    for (j = 0; j < i; j++) {
      dyn a = LGET(p,j);
      dyn b = ttxt;
      if (!texts_equal(a,b)) {
        fprintf(stderr, "  FAILED\n");
        exit(-1);
      }
    }
  }
}

void run_tests() {
  test_list_alloc();
  test_text_alloc();
  exit(-1);
}

typedef volatile void (*ptest_fn)();

ptest_fn prun_tests = (ptest_fn)run_tests;
