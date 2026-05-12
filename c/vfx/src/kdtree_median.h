#MD_GT(a,b) MD_LT((b),(a))

inline void md_swap(MD_TYPE* a, MD_TYPE* b) {
	MD_TYPE temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

inline MD_TYPE md_median_1(MD_TYPE *a) {
	return a[0];
}

inline MD_TYPE md_median_2(MD_TYPE *a) {
	return a[1];
}

inline MD_TYPE md_median_3(MD_TYPE *a) {
	MD_TYPE temp;
	if (MD_LT(a[0], a[1])) {
		md_swap(&a[0], &a[1]);
	}
	if (MD_GT(a[0], a[2])) {
		a[0] = a[1];
		a[1] = a[2];
	} else {
		return a[0];
	}
	if (MD_GT(a[0], a[1])) {
		return a[0];
	} else {
		return a[1];
	}
}

inline MD_TYPE md_median_4(MD_TYPE *a) {
	MD_TYPE temp1, temp2;
	if (MD_LT(a[0], a[1])) {
		md_swap(&a[0], &a[1]);
	}
	if (MD_GT(a[2], a[3])) {
		md_swap(&a[1], &a[2]);
	} else {
		temp1 = a[1];
		temp2 = a[2];
		a[1] = a[3];
		a[2] = temp1;
		a[3] = temp2;
	}
	if (MD_GT(a[0], a[1])) {
		a[0] = a[1];
		a[1] = a[2];
	} else {
		a[1] = a[3];
	}
	if (MD_GT(a[0], a[1])) {
		return a[0];
	} else {
		return a[1];
	}
}

// six comparisions
inline MD_TYPE md_median_5(MD_TYPE *a) {
	MD_TYPE temp1, temp2;
	if (MD_LT(a[0], a[1])) {
		md_swap(&a[0], &a[1]);
	} 
	if (MD_GT(a[2], a[3])) {
		md_swap(&a[1], &a[2]);
	} else {
		temp1 = a[1];
		temp2 = a[2];
		a[1] = a[3];
		a[2] = temp1;
		a[3] = temp2;
	}
	if (MD_GT(a[0], a[1])) {
		a[0] = a[1];
		a[1] = a[3];
		a[3] = a[4];
	} else {
		a[1] = a[2];
		a[2] = a[3];
		a[3] = a[4];
	}
	if (MD_GT(a[2], a[3])) {
		md_swap(&a[1], &a[2]);
	} else {
		temp1 = a[1];
		temp2 = a[2];
		a[1] = a[3];
		a[2] = temp1;
		a[3] = temp2;
	}
	if (MD_GT(a[0], a[1])) {
		a[0] = a[1];
		a[1] = a[2];
	} else {
		a[1] = a[3]; 
	}
	if (MD_GT(a[0], a[1])) {
		return a[0];
	} else {
		return a[1];
	}
}

inline MD_TYPE md_median(MD_TYPE *a, int n){
	switch(n) {
	case 1: return md_median_1(a);
	case 2: return md_median_2(a);
	case 3: return md_median_3(a);
	case 4: return md_median_4(a);
	case 5: return md_median_5(a);
	}
	return 0;
}

static MD_TYPE **md_subarrays;
static int md_subarrays_cols;

static void md_free_subarrays() {
  int i;
  for (i = 0; i < md_subarrays_cols; i++) free(md_subarrays[i]);
  free(md_subarrays);
  md_subarrays = 0;
  md_subarrays_cols = 0;
}

// select method returns the k-th smallest element
// median is the n/2 th smallest element
// This function runs in linear time
static MD_TYPE md_select(int k, MD_TYPE *a, int n) {
	int i, j, l;

	if (n == 1) return a[0];

	int columns = ceil(n / 5.0);
  
  if (md_subarrays_cols < columns) {
    if (md_subarrays) md_free_subarrays();
    md_subarrays = (MD_TYPE**)malloc(sizeof(MD_TYPE*)*columns);
  	for (i = 0; i < columns; i++){
  		md_subarrays[i] = (MD_TYPE*)malloc(sizeof(MD_TYPE)*5);
  	}
    md_subarrays_cols = columns;
  }

	MD_TYPE** subarrays = md_subarrays;

	for (i = 0, j = 0; i < n; i = i + 5, j++) {
		if (j == columns) {
			for (l = 0; l < n % 5; l++) {
				subarrays[j][l] = a[i + l];
			}
		} else {
			for (l = 0; l < 5; l++) {	
				subarrays[j][l] = a[i + l];
			}
		}
	}

	MD_TYPE *medians = (MD_TYPE*) malloc(sizeof(MD_TYPE) * columns);

	// calculate local medians
	for (i = 0; i < columns; i++) {
		if (n % 5 != 0 && i == columns - 1) {
			medians[i] = md_median(subarrays[i], n % 5);
		} else {
			medians[i] = md_median(subarrays[i], 5);
		}
	}

  // find the median of medians
  MD_TYPE pivot;
	if (columns <= 5) pivot = md_median(medians, columns);
	else pivot = md_select(columns/2, medians, columns);

  int len_l = 0, len_r = 0;
	// create right and left array
	for (i = 0; i < n; i++) {
		if (MD_GT(a[i], pivot)) {
			len_r++;
		} 
		if (MD_LT(a[i], pivot)) {
			len_l++;
		}
	}

  free(medians);

  if (len_l == k - 1) return pivot; //median is the pivot
  
  MD_TYPE r;
  MD_TYPE *ys;
  if (len_l < k - 1) {//median is on the right?
    if (!len_r) return pivot; //happens when several elements equal pivot
    ys = (MD_TYPE*)malloc(sizeof(MD_TYPE) * len_r);
    l = 0;
  	for (i = 0; i < n; i++) if (MD_GT(a[i], pivot)) ys[l++] = a[i];
    r = md_select(k - len_l - 1, ys, len_r);
  } else { //the median is on the left
    if (!len_l) return pivot; //happens when several elements equal pivot
    ys = (MD_TYPE*)malloc(sizeof(MD_TYPE) * len_l);
    l = 0;
  	for (i = 0; i < n; i++) if (MD_LT(a[i], pivot)) ys[l++] = a[i];
    r = md_select(k, ys, len_l);
  }
  free(ys);
  return r;
}
