#define _CRT_SECURE_NO_WARNINGS   /* Visual Studio C4996 방지 */

/*
 * quick_compare.c
 * ---------------------------------------------------------------
 * 과제 12 - 퀵 정렬의 피벗 선택에 따른 성능 비교
 *
 *  무작위 정수 데이터를 n = 1000, 2000, 4000, 8000, 16000, 20000 개로 만들어
 *  같은 원본의 복사본을 세 가지 피벗 선택 방법으로 정렬하고 비교 횟수를 센다.
 *
 *    [1] 첫 번째 원소 (First)
 *    [2] 무작위 원소 (Random)       - 분할할 때마다 새로 고른다
 *    [3] 세 원소의 중앙값 (Median-3) - 첫·가운데·마지막 중 가운데 값
 *
 *  세 방법은 피벗을 고르는 부분만 다르고 분할 알고리즘은 완전히 같다.
 *
 *  [보너스] 재귀 퀵 정렬과 스택을 쓰는 반복 퀵 정렬의 실행시간 비교
 *
 * [세는 기준]
 *   분할 과정에서 데이터 값과 피벗 값을 비교한 횟수를 센다.
 *   Median-3에서 중앙값을 고르기 위한 비교도 포함한다.
 *   반복문 인덱스 검사, 무작위 위치 생성, 데이터 이동은 세지 않는다.
 *   데이터 생성과 배열 복사도 세지 않는다.
 *
 * [난수]
 *   rand()를 쓰지 않고 선형 합동법(LCG)으로 직접 만들었다.
 *   time.h는 보너스 과제의 실행시간 측정에만 사용한다.
 */

#include <stdio.h>
#include <time.h>

#define MAX_N      20000
#define MAX_VALUE 100000      /* 데이터 값 범위 0 ~ 99999 (중복 허용) */
#define BONUS_N    10000      /* 보너스 실험의 데이터 크기 */
#define REPEAT       500      /* 실행시간 측정 반복 횟수 */
#define DEFAULT_SEED 12345

#define PIVOT_FIRST  0
#define PIVOT_RANDOM 1
#define PIVOT_MEDIAN 2

 /* ---------------- 난수 생성기 ---------------- */

static unsigned long rngState = DEFAULT_SEED;
static void rng_seed(unsigned long seed) { rngState = seed; }
static int rng_next(int bound) {
    rngState = rngState * 1103515245UL + 12345UL;
    return (int)((rngState >> 16) % (unsigned long)bound);
}

/* ---------------- 공통 ---------------- */

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

/* ---------------- 피벗 선택 ---------------- */

/*
 * 세 값의 중앙값이 있는 위치를 돌려준다.
 * 산술평균이 아니라 크기순으로 가운데인 값이다. 비교는 2~3회 발생한다.
 */
static int median_of_three(const int a[], int l, int m, int r, long long* cmp) {
    (*cmp)++;
    if (a[l] > a[m]) {
        (*cmp)++;
        if (a[m] > a[r]) return m;              /* r < m < l */
        (*cmp)++;
        return (a[l] > a[r]) ? r : l;
    }
    else {
        (*cmp)++;
        if (a[l] > a[r]) return l;              /* r < l <= m */
        (*cmp)++;
        return (a[m] > a[r]) ? r : m;
    }
}

static int choose_pivot(const int a[], int left, int right, int method, long long* cmp) {
    if (method == PIVOT_FIRST)  return left;
    if (method == PIVOT_RANDOM) return left + rng_next(right - left + 1);
    return median_of_three(a, left, (left + right) / 2, right, cmp);
}

/* ---------------- 분할 (세 방법 공통) ---------------- */

/*
 * 고른 피벗을 맨 뒤로 보낸 뒤, 왼쪽부터 훑으며 피벗보다 작거나 같은 값을 앞쪽에 모은다.
 * 마지막에 피벗을 그 경계 자리에 돌려놓으면 피벗은 제자리를 찾은 것이다.
 * 이 함수의 비교 횟수는 구간 길이 - 1 로 일정하다.
 */
static int partition(int a[], int left, int right, int method, long long* cmp) {
    int p, pivot, i, j;

    p = choose_pivot(a, left, right, method, cmp);
    swap(&a[p], &a[right]);                     /* 피벗을 맨 뒤로 */
    pivot = a[right];

    i = left - 1;
    for (j = left; j < right; j++) {
        (*cmp)++;                               /* 데이터와 피벗 비교 */
        if (a[j] <= pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    swap(&a[i + 1], &a[right]);                 /* 피벗을 제자리로 */
    return i + 1;
}

/* ---------------- 재귀 퀵 정렬 ---------------- */

static void quick_recursive(int a[], int left, int right, int method, long long* cmp) {
    int p;
    if (left >= right) return;
    p = partition(a, left, right, method, cmp);
    quick_recursive(a, left, p - 1, method, cmp);    /* 피벗 왼쪽 */
    quick_recursive(a, p + 1, right, method, cmp);   /* 피벗 오른쪽 */
}

/* ---------------- 반복 퀵 정렬 (스택 사용) ---------------- */

typedef struct { int left, right; } Range;

/*
 * 재귀 호출 대신 "아직 정렬하지 않은 구간"을 스택에 쌓아 두고 하나씩 꺼내 처리한다.
 * 큰 쪽을 먼저 쌓고 작은 쪽을 나중에 쌓으면(= 작은 쪽부터 처리) 스택이 깊어지지 않는다.
 */
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

        if (p - left > right - p) {             /* 큰 쪽을 먼저 쌓는다 */
            if (left < p - 1) { top++; stack[top].left = left;  stack[top].right = p - 1; }
            if (p + 1 < right) { top++; stack[top].left = p + 1; stack[top].right = right; }
        }
        else {
            if (p + 1 < right) { top++; stack[top].left = p + 1; stack[top].right = right; }
            if (left < p - 1) { top++; stack[top].left = left;  stack[top].right = p - 1; }
        }
    }
}

/* ---------------- 시드 입력 ---------------- */

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

/* ---------------- main ---------------- */

int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    int caseCount = (int)(sizeof(sizes) / sizeof(sizes[0]));
    long long fstCmp[8], rndCmp[8], medCmp[8], cmp;
    int ok = 1, k, n, r;
    clock_t start, end;
    double tRec, tIter, tCopy;

    rng_seed(ask_seed());

    /* ---------- 1. 피벗 선택 방법별 비교 횟수 ---------- */
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
        while (t > 1) { t /= 2; b++; }                        /* log2 n (정수 부분) */
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

    /* ---------- 2. 추가 실험: 크기별 30회 평균 ---------- */
    /* 한 번만 재면 어느 방법이 유리한지 잡음에 묻히므로, 크기마다 새 데이터로 30회 반복해 평균을 낸다 */
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

    /* ---------- 3. 추가 실험: 이미 정렬된 데이터 ---------- */
    /* 피벗 선택이 왜 중요한지는 정렬된 입력에서 가장 분명하게 드러난다.
       첫 원소 피벗은 재귀 깊이가 n까지 깊어지므로 크기를 2,000으로 줄여 실험한다. */
    {
        int m = 2000, i, j, t;
        make_random(original, m);
        for (i = 0; i < m - 1; i++)                 /* 실험 입력 준비 (측정 제외) */
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

    /* ---------- 4. 보너스: 재귀 vs 반복 ---------- */
    make_random(original, BONUS_N);

    /* 배열 복사 시간을 따로 재서 빼면 정렬 시간만 남는다 */
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