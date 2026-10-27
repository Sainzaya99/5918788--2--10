#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N          10000     
#define TRIALS     100        
#define KEY_MAX    1000000  
#define STAB_N     1000       
#define STAB_RANGE 50
#define PARTIAL_SWAPS 500    

typedef struct {
    int key1;
    int key2;
    int order;
} Data;

long long g_cmp = 0;
long long g_mov = 0;

static unsigned long long g_seed = 1;

static unsigned int my_rand(void)
{
    g_seed ^= g_seed >> 12;
    g_seed ^= g_seed << 25;
    g_seed ^= g_seed >> 27;
    return (unsigned int)((g_seed * 2685821657736338717ULL) >> 32); 
}

static void my_srand(unsigned long long seed)
{
    g_seed = seed ? seed : 1;
    for (int i = 0; i < 20; i++) my_rand();   
}

static void swap_data(Data* a, Data* b)
{
    Data temp = *a;  g_mov++;
    *a = *b;         g_mov++;
    *b = temp;       g_mov++;
}

static int is_sorted(const Data* a, int n)
{
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 > a[i + 1].key1) return 0;
    return 1;
}

void selection_sort(Data* a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            g_cmp++;                         
            if (a[j].key1 < a[min].key1) min = j;
        }
        if (min != i) swap_data(&a[i], &a[min]);   
    }
}

void bubble_sort(Data* a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            g_cmp++;
            if (a[j].key1 > a[j + 1].key1) swap_data(&a[j], &a[j + 1]);
        }
    }
}

void bubble_sort_early(Data* a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            g_cmp++;
            if (a[j].key1 > a[j + 1].key1) {
                swap_data(&a[j], &a[j + 1]);
                swapped = 1;
            }
        }
        if (!swapped) break;                
    }
}

static void insertion_core(Data* a, int n, int use_ge)
{
    for (int i = 1; i < n; i++) {
        Data current = a[i];  g_mov++;     
        int j = i - 1;
        while (j >= 0) {
            g_cmp++;                       
            int shift = use_ge ? (a[j].key1 >= current.key1)
                : (a[j].key1 > current.key1);
            if (!shift) break;
            a[j + 1] = a[j];  g_mov++;       
            j--;
        }
        a[j + 1] = current;   g_mov++;       
    }
}
void insertion_sort_gt(Data* a, int n) { insertion_core(a, n, 0); }
void insertion_sort_ge(Data* a, int n) { insertion_core(a, n, 1); }

typedef void (*SortFn)(Data*, int);

typedef struct {
    long long cmp;
    long long mov;
    int ok;         
} Result;

static Data work[N]; 

static void run_sort(SortFn f, const Data* src, int n, Result* r)
{
    memcpy(work, src, sizeof(Data) * n);   
    g_cmp = 0;
    g_mov = 0;
    f(work, n);
    r->cmp = g_cmp;
    r->mov = g_mov;
    r->ok = is_sorted(work, n);
}

#define NUM_ALGO 4
static const char* ALGO_NAME[NUM_ALGO] = {
    "Selection Sort", "Bubble Sort", "Bubble Sort (Early Termination)", "Insertion Sort"
};
static SortFn ALGO_FN[NUM_ALGO] = {
    selection_sort, bubble_sort, bubble_sort_early, insertion_sort_gt
};

static void make_random(Data* a, int n, int range)
{
    for (int i = 0; i < n; i++) {
        a[i].key1 = my_rand() % (range + 1);
        a[i].key2 = my_rand() % (KEY_MAX + 1);
        a[i].order = i;
    }
}

static int cmp_prep(const void* x, const void* y)
{
    const Data* p = x, * q = y;
    if (p->key1 != q->key1) return (p->key1 > q->key1) - (p->key1 < q->key1);
    return (p->order > q->order) - (p->order < q->order);
}

static int count_stability_violations(const Data* a, int n)
{
    int bad = 0;
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i + 1].key1 && a[i].order > a[i + 1].order) bad++;
    return bad;
}

static void print_small(const char* title, const Data* a, int n)
{
    printf("  %-22s:", title);
    for (int i = 0; i < n; i++)
        printf(" (%d,%d)#%d", a[i].key1, a[i].key2, a[i].order);
    printf("\n");
}

static void stability_report(const char* name, SortFn f, const Data* src, int n)
{
    Result r;
    run_sort(f, src, n, &r);
    int bad = count_stability_violations(work, n);
    if (bad == 0) printf("%-21s: Stable (order violations = 0, sorted = %s)\n",
        name, r.ok ? "OK" : "FAIL");
    else          printf("%-21s: NOT Stable (order violations = %d, sorted = %s)\n",
        name, bad, r.ok ? "OK" : "FAIL");
}

int main(void)
{
    static Data base[N], sorted_d[N], reverse_d[N], partial_d[N];
    long long tot_cmp[NUM_ALGO] = { 0 }, tot_mov[NUM_ALGO] = { 0 };
    int all_ok = 1;

    printf("Data Count : %d\n\n", N);

    my_srand(12345);
    for (int t = 0; t < TRIALS; t++) {
        make_random(base, N, KEY_MAX);          
        for (int k = 0; k < NUM_ALGO; k++) {        
            Result r;
            run_sort(ALGO_FN[k], base, N, &r);
            tot_cmp[k] += r.cmp;
            tot_mov[k] += r.mov;
            if (!r.ok) all_ok = 0;
        }
    }
    printf("Random Data\nNumber of Trials : %d\n", TRIALS);
    printf("All results sorted correctly : %s\n\n", all_ok ? "YES" : "NO");
    for (int k = 0; k < NUM_ALGO; k++) {
        printf("%s\n", ALGO_NAME[k]);
        printf("Total Comparisons   : %lld\n", tot_cmp[k]);
        printf("Average Comparisons : %lld\n", tot_cmp[k] / TRIALS);
        printf("Average Movements   : %lld\n\n", tot_mov[k] / TRIALS);
    }

    my_srand(2024);
    make_random(base, N, KEY_MAX);
    memcpy(sorted_d, base, sizeof(base));
    qsort(sorted_d, N, sizeof(Data), cmp_prep);     
    for (int i = 0; i < N; i++) reverse_d[i] = sorted_d[N - 1 - i]; 
    memcpy(partial_d, sorted_d, sizeof(sorted_d));  
    for (int s = 0; s < PARTIAL_SWAPS; s++) {
        int i = my_rand() % N, j = my_rand() % N;
        Data tmp = partial_d[i]; partial_d[i] = partial_d[j]; partial_d[j] = tmp;
    }

    const char* state_name[3] = { "Sorted Data", "Reverse-Sorted Data", "Partially Sorted Data" };
    const Data* state_src[3] = { sorted_d, reverse_d, partial_d };
    for (int s = 0; s < 3; s++) {
        printf("------------------------------------\n\n%s\n\n", state_name[s]);
        for (int k = 0; k < NUM_ALGO; k++) {
            Result r;
            run_sort(ALGO_FN[k], state_src[s], N, &r);
            printf("%s\n", ALGO_NAME[k]);
            printf("Comparisons : %lld\n", r.cmp);
            printf("Movements   : %lld\n", r.mov);
            printf("Sorted      : %s\n\n", r.ok ? "OK" : "FAIL");
        }
    }

    printf("------------------------------------\n\n");
    printf("Stability Verification\n\n");

    Data ex[4] = { {527, 100, 0}, {527, 200, 1}, {125, 300, 2}, {892, 400, 3} };
    printf("[Small example] (key1,key2)#original_order\n");
    print_small("Original", ex, 4);
    {
        Data tmp[4];
        for (int k = 0; k < 4; k++) {
            SortFn fs[4] = { selection_sort, bubble_sort, insertion_sort_gt, insertion_sort_ge };
            const char* nm[4] = { "Selection Sort", "Bubble Sort", "Insertion Sort (>)", "Insertion Sort (>=)" };
            memcpy(tmp, ex, sizeof(ex));
            fs[k](tmp, 4);
            print_small(nm[k], tmp, 4);
        }
    }

    my_srand(777);
    static Data stab[STAB_N];
    make_random(stab, STAB_N, STAB_RANGE - 1);
    printf("\n[Test data] %d items, key1 in 0..%d (many duplicates)\n", STAB_N, STAB_RANGE - 1);
    stability_report("Selection Sort", selection_sort, stab, STAB_N);
    stability_report("Bubble Sort", bubble_sort, stab, STAB_N);
    stability_report("Bubble Sort (Early)", bubble_sort_early, stab, STAB_N);
    stability_report("Insertion Sort (>)", insertion_sort_gt, stab, STAB_N);
    stability_report("Insertion Sort (>=)", insertion_sort_ge, stab, STAB_N);

    {
        Result a, b;
        run_sort(insertion_sort_gt, stab, STAB_N, &a);
        run_sort(insertion_sort_ge, stab, STAB_N, &b);
        printf("\n[Insertion Sort on test data] (>) : cmp=%lld mov=%lld | (>=) : cmp=%lld mov=%lld\n",
            a.cmp, a.mov, b.cmp, b.mov);
    }
    return 0;
}