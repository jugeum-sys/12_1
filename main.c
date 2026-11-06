#define _CRT_SECURE_NO_WARNINGS   
#include <stdio.h>
#include <time.h>

#define MAX_N      20000
#define MAX_VALUE 100000      
#define BONUS_N    10000      
#define REPEAT       500      
#define DEFAULT_SEED 12345

#define PIVOT_FIRST  0
#define PIVOT_RANDOM 1
#define PIVOT_MEDIAN 2


static unsigned long rngState = DEFAULT_SEED;
static void rng_seed(unsigned long seed) { rngState = seed; }
static int rng_next(int bound) {
    rngState = rngState * 1103515245UL + 12345UL;
    return (int)((rngState >> 16) % (unsigned long)bound);
}


static int original[MAX_N], work[MAX_N];

static void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }

static void make_random(int a[], int n) {
    int i;
    for (i = 0; i < n; i++) a[i] = rng_next(MAX_VALUE);
}

static void copy_array(const int src[], int dst[], int n) {
    int i;
    for (i = 0; i < n; i++) dst[i] = src[i];
}

static int is_sorted(const int a[], int n) {
    int i;
    for (i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}


static int median_of_three(const int a[], int l, int m, int r, long long* cmp) {
    (*cmp)++;
    if (a[l] > a[m]) {
        (*cmp)++;
        if (a[m] > a[r]) return m;             
        (*cmp)++;
        return (a[l] > a[r]) ? r : l;
    }
    else {
        (*cmp)++;
        if (a[l] > a[r]) return l;             
        (*cmp)++;
        return (a[m] > a[r]) ? r : m;
    }
}

static int choose_pivot(const int a[], int left, int right, int method, long long* cmp) {
    if (method == PIVOT_FIRST)  return left;
    if (method == PIVOT_RANDOM) return left + rng_next(right - left + 1);
    return median_of_three(a, left, (left + right) / 2, right, cmp);
}


static int partition(int a[], int left, int right, int method, long long* cmp) {
    int p, pivot, i, j;

    p = choose_pivot(a, left, right, method, cmp);
    swap(&a[p], &a[right]);                     
    pivot = a[right];

    i = left - 1;
    for (j = left; j < right; j++) {
        (*cmp)++;                              
        if (a[j] <= pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    swap(&a[i + 1], &a[right]);                
    return i + 1;
}


static void quick_recursive(int a[], int left, int right, int method, long long* cmp) {
    int p;
    if (left >= right) return;
    p = partition(a, left, right, method, cmp);
    quick_recursive(a, left, p - 1, method, cmp);   
    quick_recursive(a, p + 1, right, method, cmp);   
}


typedef struct { int left, right; } Range;


static void quick_iterative(int a[], int n, int method, long long* cmp) {
    static Range stack[MAX_N];
    int top = -1, left, right, p;

    if (n < 2) return;
    top++;
    stack[top].left = 0;
    stack[top].right = n - 1;

    while (top >= 0) {
        left = stack[top].left;
        right = stack[top].right;
        top--;
        if (left >= right) continue;

        p = partition(a, left, right, method, cmp);

        if (p - left > right - p) {             
            if (left < p - 1) { top++; stack[top].left = left;  stack[top].right = p - 1; }
            if (p + 1 < right) { top++; stack[top].left = p + 1; stack[top].right = right; }
        }
        else {
            if (p + 1 < right) { top++; stack[top].left = p + 1; stack[top].right = right; }
            if (left < p - 1) { top++; stack[top].left = left;  stack[top].right = p - 1; }
        }
    }
}


static unsigned long ask_seed(void) {
    char line[64];
    unsigned long value = 0;
    int i = 0, digits = 0;

    printf("난수 시드를 입력하세요 (그냥 Enter를 누르면 %d): ", DEFAULT_SEED);
    if (fgets(line, sizeof(line), stdin) == NULL) return DEFAULT_SEED;
    while (line[i] == ' ' || line[i] == '\t') i++;
    while (line[i] >= '0' && line[i] <= '9') {
        value = value * 10 + (unsigned long)(line[i] - '0');
        digits++; i++;
    }
    return (digits > 0) ? value : DEFAULT_SEED;
}


int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    int caseCount = (int)(sizeof(sizes) / sizeof(sizes[0]));
    long long fstCmp[8], rndCmp[8], medCmp[8], cmp;
    int ok = 1, k, n, r;
    clock_t start, end;
    double tRec, tIter, tCopy;

    rng_seed(ask_seed());

    for (k = 0; k < caseCount; k++) {
        n = sizes[k];
        make_random(original, n);

        copy_array(original, work, n);
        cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_FIRST, &cmp);
        fstCmp[k] = cmp;
        if (!is_sorted(work, n)) ok = 0;

        copy_array(original, work, n);
        cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_RANDOM, &cmp);
        rndCmp[k] = cmp;
        if (!is_sorted(work, n)) ok = 0;

        copy_array(original, work, n);
        cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_MEDIAN, &cmp);
        medCmp[k] = cmp;
        if (!is_sorted(work, n)) ok = 0;

        printf("\nData Size : %d\n\n", n);
        printf("Quick Sort (First)    : %lld comparisons\n", fstCmp[k]);
        printf("Quick Sort (Random)   : %lld comparisons\n", rndCmp[k]);
        printf("Quick Sort (Median-3) : %lld comparisons\n", medCmp[k]);
    }

    printf("\n------------------------------------\n");
    printf("\n[요약] n log2 n 과 비교\n");
    printf("     n        First       Random    Median-3     n log2 n   First/nlogn  Med3/nlogn\n");
    for (k = 0; k < caseCount; k++) {
        double nn = (double)sizes[k], nlogn;
        int b = 0, t = sizes[k];
        while (t > 1) { t /= 2; b++; }                       
        nlogn = nn * b;
        printf("%6d  %11lld  %11lld  %11lld  %11.0f  %10.2f  %10.2f\n",
            sizes[k], fstCmp[k], rndCmp[k], medCmp[k], nlogn,
            (double)fstCmp[k] / nlogn, (double)medCmp[k] / nlogn);
    }

    printf("\n[데이터가 2배가 될 때 비교 횟수의 증가 배율]\n");
    printf("        n           First     Random   Median-3\n");
    for (k = 1; k < caseCount; k++) {
        printf("%6d -> %6d   %7.2f배  %7.2f배  %7.2f배\n",
            sizes[k - 1], sizes[k],
            (double)fstCmp[k] / (double)fstCmp[k - 1],
            (double)rndCmp[k] / (double)rndCmp[k - 1],
            (double)medCmp[k] / (double)medCmp[k - 1]);
    }
    printf("\n정렬 결과 확인 : %s\n", ok ? "모든 경우 오름차순 정렬 성공" : "실패한 경우 있음");

    {
        int TRIALS = 30, t;
        long long sf, sr, sm;
        printf("\n------------------------------------\n");
        printf("\n[추가 실험] 크기별 %d회 평균 비교 횟수\n", TRIALS);
        printf("     n        First       Random    Median-3   First/Med3  Random/Med3\n");
        for (k = 0; k < caseCount; k++) {
            n = sizes[k];
            sf = sr = sm = 0;
            for (t = 0; t < TRIALS; t++) {
                make_random(original, n);
                copy_array(original, work, n);
                cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_FIRST, &cmp);  sf += cmp;
                copy_array(original, work, n);
                cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_RANDOM, &cmp); sr += cmp;
                copy_array(original, work, n);
                cmp = 0; quick_recursive(work, 0, n - 1, PIVOT_MEDIAN, &cmp); sm += cmp;
                if (!is_sorted(work, n)) ok = 0;
            }
            printf("%6d  %11.0f  %11.0f  %11.0f  %10.3f  %11.3f\n",
                n, (double)sf / TRIALS, (double)sr / TRIALS, (double)sm / TRIALS,
                (double)sf / (double)sm, (double)sr / (double)sm);
        }
    }

    {
        int m = 2000, i, j, t;
        make_random(original, m);
        for (i = 0; i < m - 1; i++)                 
            for (j = i + 1; j < m; j++)
                if (original[j] < original[i]) { t = original[i]; original[i] = original[j]; original[j] = t; }

        printf("\n------------------------------------\n");
        printf("\n[추가 실험] 이미 정렬된 데이터 %d개\n\n", m);

        copy_array(original, work, m);
        cmp = 0; quick_recursive(work, 0, m - 1, PIVOT_FIRST, &cmp);
        printf("Quick Sort (First)    : %lld comparisons\n", cmp);

        copy_array(original, work, m);
        cmp = 0; quick_recursive(work, 0, m - 1, PIVOT_RANDOM, &cmp);
        printf("Quick Sort (Random)   : %lld comparisons\n", cmp);

        copy_array(original, work, m);
        cmp = 0; quick_recursive(work, 0, m - 1, PIVOT_MEDIAN, &cmp);
        printf("Quick Sort (Median-3) : %lld comparisons\n", cmp);
    }

    make_random(original, BONUS_N);

    start = clock();
    for (r = 0; r < REPEAT; r++) copy_array(original, work, BONUS_N);
    end = clock();
    tCopy = (double)(end - start);

    start = clock();
    for (r = 0; r < REPEAT; r++) {
        copy_array(original, work, BONUS_N);
        cmp = 0;
        quick_recursive(work, 0, BONUS_N - 1, PIVOT_MEDIAN, &cmp);
    }
    end = clock();
    tRec = (double)(end - start) - tCopy;
    if (!is_sorted(work, BONUS_N)) ok = 0;

    start = clock();
    for (r = 0; r < REPEAT; r++) {
        copy_array(original, work, BONUS_N);
        cmp = 0;
        quick_iterative(work, BONUS_N, PIVOT_MEDIAN, &cmp);
    }
    end = clock();
    tIter = (double)(end - start) - tCopy;
    if (!is_sorted(work, BONUS_N)) ok = 0;

    printf("\n------------------------------------\n");
    printf("\n[보너스] 재귀 퀵 정렬과 반복 퀵 정렬\n\n");
    printf("Data Size : %d\n", BONUS_N);
    printf("Pivot     : Median-of-Three\n\n");
    printf("Recursive Quick Sort : %.4f ms\n", tRec * 1000.0 / CLOCKS_PER_SEC / REPEAT);
    printf("Iterative Quick Sort : %.4f ms\n", tIter * 1000.0 / CLOCKS_PER_SEC / REPEAT);
    printf("(%d회 반복의 평균, 배열 복사 시간은 빼고 측정)\n", REPEAT);
    printf("비교 횟수는 두 방법이 같다 : %lld\n", cmp);
    printf("\n정렬 결과 확인 : %s\n", ok ? "모든 경우 오름차순 정렬 성공" : "실패한 경우 있음");
    return 0;
}
