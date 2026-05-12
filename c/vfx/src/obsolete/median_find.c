#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define TEST_INT
#define TEST_INTPTR

#ifdef TEST_INTPTR
typedef int* intptr_;
#define MD_TYPE intptr_
#define MD_LT(a,b) ((*(a))<(*(b)))
#define MD_GT(a,b) ((*(a))>(*(b)))
static MD_TYPE md_smallest = 0;
static int md_smallest2 = 0x80000000;
static void md_init() {
  md_smallest = &md_smallest2;
}
#else
#define MD_TYPE int
#define MD_LT(a,b) ((a)<(b))
#define MD_GT(a,b) ((a)>(b))
static MD_TYPE md_smallest = 0x80000000;
static void md_init() {
}

#endif

void swap(MD_TYPE* a, MD_TYPE* b) {
	MD_TYPE temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

MD_TYPE median_1(MD_TYPE a[]) {
	return a[0];
}

MD_TYPE median_2(MD_TYPE a[]) {
	return a[1];
}

MD_TYPE median_3(MD_TYPE a[]) {
	MD_TYPE temp;
	if (MD_LT(a[0], a[1])) {
		swap(&a[0], &a[1]);
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

MD_TYPE median_4(MD_TYPE a[]) {
	MD_TYPE temp1, temp2;
	if (MD_LT(a[0], a[1])) {
		swap(&a[0], &a[1]);
	}
	if (MD_GT(a[2], a[3])) {
		swap(&a[1], &a[2]);
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
MD_TYPE median_5(MD_TYPE a[]) {
	MD_TYPE temp1, temp2;
	if (MD_LT(a[0], a[1])) {
		swap(&a[0], &a[1]);
	} 
	if (MD_GT(a[2], a[3])) {
		swap(&a[1], &a[2]);
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
		swap(&a[1], &a[2]);
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

MD_TYPE median(MD_TYPE a[], int n){
	switch(n) {
		case 1: 
		return median_1(a);
		break;
		case 2:
		return median_2(a);
		break;
		case 3:
		return median_3(a);
		break;
		case 4:
		return median_4(a);
		break;
		case 5:
		return median_5(a);
		break;
		default:
		break;
	}
	return md_smallest;
}

// select method returns the k-th smallest element
// median is the n/2 th smallest element
// This function runs in linear time
MD_TYPE select(int k, MD_TYPE *a, int n) {
	int i, j, l, m;

	if (n == 1) {
		return a[0];
	}

	int columns = ceil(n / 5.0);

	MD_TYPE** subarrays = (MD_TYPE**)malloc(sizeof(MD_TYPE*)*columns);
	for (i=0;i<columns;i++){
		subarrays[i] = (MD_TYPE*)malloc(sizeof(MD_TYPE)*5);
	}

	MD_TYPE *medians = (MD_TYPE*) malloc(sizeof(MD_TYPE) * columns);

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

	// calculate local medians
	for (i = 0; i < columns; i++) {
		if (n % 5 != 0 && i == columns - 1) {
			medians[i] = median(subarrays[i], n % 5);
		} else {
			medians[i] = median(subarrays[i], 5);
		}
	}

  MD_TYPE pivot;
	if (columns <= 5) {
		pivot = median(medians, columns);
	} else {
		pivot = select(columns/2, medians, columns);
	}

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

	MD_TYPE *left = (MD_TYPE*) malloc(sizeof(MD_TYPE) * len_l);
	MD_TYPE *right = (MD_TYPE*) malloc(sizeof(MD_TYPE) * len_r);

	l = 0, m = 0;
	for (i = 0; i < n; i++) {
		if (MD_GT(a[i], pivot)){
			right[l] = a[i];
			l++;
		} 
		if (MD_LT(a[i], pivot)) {
			left[m] = a[i];
			m++;
		}

	}

  MD_TYPE r;

	if (len_l == k - 1) {
		r = a[0];
	} else if (len_l > k - 1) {
		r = select(k, left, len_l);
	} else if (len_l < k - 1) {
		r = select(k - len_l - 1, right, len_r);
	} else {
    r = md_smallest;
  }

  free(left);
  free(right);
  free(medians);
	for (i=0;i<columns;i++)	free(subarrays[i]);
  free(subarrays);

	return r;
}

int main() {
  md_init();
#define NINTS 20
#ifdef TEST_INTPTR
	int test_ints[] = {8,10,12,13,3,4,5,6,19,20,9,7,11,14,15,16,17,18,1,2};
 	MD_TYPE xs[NINTS];
  for (int i = 0; i < NINTS; i++) {
    xs[i] = &test_ints[i];
  }
  MD_TYPE m = select(10, xs, NINTS);
	printf("Median is %d=%d\n", (int)(m-test_ints), *m);
  printf("%d\n", *md_smallest);
#else
	int xs[] = {8,10,12,13,3,4,5,6,19,20,9,7,11,14,15,16,17,18,1,2};

  MD_TYPE m = select(10, xs, NINTS);
	printf("Median is %d\n", m);
#endif
}
