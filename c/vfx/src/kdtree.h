#KD_TYPE double
//#KD_DIM 3

struct KDNode;
typedef struct KDNode KDNode;

typedef struct KDNode {
  KD_TYPE x[KD_DIM];
  KDNode *left, *right;
} KDNode;


inline KD_TYPE kd_dist(KDNode *a, KDNode *b) {
  KD_TYPE d = 0;
  for (int i = 0; i < KD_DIM; i++) {
    KD_TYPE t = a->x[i] - b->x[i];
    d += t * t;
  }
  return d;
}

inline void kd_swap(KDNode *a, KDNode *b) {
  for (int i = 0; i < KD_DIM; i++) {
    KD_TYPE t = a->x[i];
    a->x[i] = b->x[i];
    b->x[i] = t;
  }
}


static int md_axis;
#MD_LT(a,b) ((a)->x[md_axis]<(b)->x[md_axis])
typedef KDNode* KDNode_ptr;
#MD_TYPE KDNode_ptr

#:kdtree_median

static KDNode **kd_pxs;

inline KDNode *kd_median_sort(KDNode *xs, int len, int axis) {
#if 1
  if (len == 3) {
    KDNode *t = xs;
    if (t[1].x[axis] < t[0].x[axis]) kd_swap(&t[0],&t[1]);
    if (t[2].x[axis] < t[1].x[axis]) kd_swap(&t[1],&t[2]);
    if (t[1].x[axis] < t[0].x[axis]) kd_swap(&t[0],&t[1]);
    return t[1].x[axis] == t[0].x[axis] ? &t[0] : &t[1];
  }
#endif
  md_axis = axis;
  KDNode *end = xs+len;
  KDNode **ppxs = kd_pxs;
  KDNode *n;

  for (n = xs; n != end; n++) *ppxs++ = n;

  KDNode *m = md_select(len/2, kd_pxs, len);
  
  KD_TYPE pivot = m->x[axis];
  KDNode *ys = malloc(sizeof(KDNode)*len);
  KDNode *s = ys;
  KDNode *e = ys+len;
  for (n = xs; n < end; n++) {
    if (n->x[axis] < pivot) {
      *s++ = *n;
    } else {
      if (n == m) continue; //ensure pivot gets the middle place
      *--e = *n;
    }
  }
  *s = *m;
  m = xs + (s-ys);
  memcpy(xs, ys, sizeof(KDNode)*len);
  free(ys);
  return m;
}

static KDNode* kd_make2(KDNode *t, int len, int axis) {
  if (len == 0) return 0;
  if (len == 1) return t;
#if 1
  if (len == 2) {
    if (t[0].x[axis] < t[1].x[axis]) {
      t[1].left = &t[0];
      t[1].right = 0;
      return &t[1];
    }
    t[0].left = &t[1];
    t[0].right = 0;
    return &t[0];
  }
#endif
  KDNode *m = kd_median_sort(t, len, axis);

  if (++axis >= KD_DIM) axis = 0;
  int mlen = m - t;

  //fprintf(stdout, "%d/%d\n", mlen, len);
  m->left  = kd_make2(t, mlen, axis);
  m->right = kd_make2(m + 1, len-mlen-1, axis);
  return m;
}

inline KDNode* kd_make(KDNode *xs, int len) {
  kd_pxs = malloc(len*sizeof(KDNode *));
  KDNode *root = kd_make2(xs, len, 0);
  free(kd_pxs);
  return root;
}

static void kd_nearest2(KDNode *root, KDNode *pt, int i
                       ,KDNode **best, KD_TYPE *best_dist) {
  if (!root) return;

  KD_TYPE d = kd_dist(root, pt);
  KD_TYPE dx = root->x[i] - pt->x[i];
  KD_TYPE dx2 = dx * dx;

  if (!*best || d < *best_dist) {
    *best_dist = d;
    *best = root;
    if (!*best_dist) return; //exact match
  }

  if (++i >= KD_DIM) i = 0;

  kd_nearest2(dx > 0 ? root->left : root->right, pt, i, best, best_dist);
  if (dx2 >= *best_dist) return;
  kd_nearest2(dx > 0 ? root->right : root->left, pt, i, best, best_dist);
}

inline KDNode *kd_nearest(KDNode *root, KDNode *point) {
  KD_TYPE best_dist;
  KDNode *best = 0;
  kd_nearest2(root, point, 0, &best, &best_dist);
  return best;
}
