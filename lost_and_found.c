/*
 * Smart Lost & Found System  (C version)
 * Data Structures project - all structures implemented manually.
 *
 * Hash table (chaining) | Linked lists | BST | Max-heap | Queue | Stack
 * Merge sort | Sequential + Binary search | Graph + Dijkstra | String processing
 *
 * Compile: gcc -std=c99 -Wall lost_and_found.c -o lostfound
 * Run:     ./lostfound
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE    13
#define NUM_CAT       5
#define NUM_PLACES    8
#define MAX_ITEMS  1000
#define MAX_MATCHES 1000
#define MAX_KW       30
#define NAME_LEN     50
#define DESC_LEN    100
#define DATE_LEN     11
#define KW_LEN       20
#define INF  1000000000

static const char *CATS[NUM_CAT]      = {"Electronics", "Books", "IDCards", "Wallets", "Others"};
static const char *PLACES[NUM_PLACES] = {"Main Gate", "Library", "Canteen", "Hostel",
                                         "Admin Block", "CS Dept", "Sports Ground", "Auditorium"};
static const int PRIORITY[NUM_CAT]    = {4, 2, 5, 5, 1};

enum { LOST, FOUND };
enum { OPEN, RETURNED, REMOVED };
enum { ACT_REPORT, ACT_CLAIM };

/* ---------------------------------------------------------------- */
/*  Item                                                             */
/* ---------------------------------------------------------------- */
typedef struct Item {
    int  id, type, category, place, priority, status;
    char name[NAME_LEN], date[DATE_LEN], desc[DESC_LEN];
    char kw[MAX_KW][KW_LEN];
    int  nkw;
} Item;

static Item *allItems[MAX_ITEMS];   /* master array (for sorting / binary search) */
static int   itemCount = 0;
static int   nextId = 1;

/* ---------------------------------------------------------------- */
/*  Hash table with chaining                                         */
/* ---------------------------------------------------------------- */
typedef struct HNode { Item *item; struct HNode *next; } HNode;
static HNode *table[HASH_SIZE];
static int collisions = 0;

static int hashFn(int id) { return id % HASH_SIZE; }

static void hashInsert(Item *it) {
    int h = hashFn(it->id);
    HNode *n = malloc(sizeof(HNode));
    n->item = it;
    if (table[h] != NULL) collisions++;
    n->next = table[h];
    table[h] = n;
}

static Item *hashSearch(int id, int *steps) {
    HNode *p = table[hashFn(id)];
    *steps = 0;
    while (p) {
        (*steps)++;
        if (p->item->id == id) return p->item;
        p = p->next;
    }
    return NULL;
}

static int longestChain(void) {
    int best = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        int len = 0;
        for (HNode *p = table[i]; p; p = p->next) len++;
        if (len > best) best = len;
    }
    return best;
}

/* ---------------------------------------------------------------- */
/*  Category linked lists                                            */
/* ---------------------------------------------------------------- */
typedef struct LNode { Item *item; struct LNode *next; } LNode;
static LNode *catList[NUM_CAT];

static void catInsert(Item *it) {
    LNode *n = malloc(sizeof(LNode));
    n->item = it;
    n->next = catList[it->category];   /* O(1) insert at head */
    catList[it->category] = n;
}

/* ---------------------------------------------------------------- */
/*  String helpers                                                   */
/* ---------------------------------------------------------------- */
static int ciCompare(const char *a, const char *b) {
    while (*a && *b) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y) return x - y;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static int readLine(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    if (strchr(buf, '\n') == NULL) {            /* line was longer than buffer: */
        int ch;                                  /* discard the rest so it does  */
        while ((ch = getchar()) != '\n' && ch != EOF) {}   /* not leak into the next prompt */
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

static int readInt(void) {
    char buf[32], *end;
    if (!readLine(buf, sizeof buf)) exit(0);
    long v = strtol(buf, &end, 10);
    if (end == buf) return -1;
    return (int)v;
}

static int validDate(const char *s) {
    int y, m, d;
    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-') return 0;
    if (sscanf(s, "%4d-%2d-%2d", &y, &m, &d) != 3) return 0;
    return m >= 1 && m <= 12 && d >= 1 && d <= 31;
}

static int isStopWord(const char *w) {
    static const char *stop[] = {"the", "and", "with", "for", "from", "that", "this",
                                 "has", "have", "was", "near", "are", "its", "got"};
    for (size_t i = 0; i < sizeof stop / sizeof stop[0]; i++)
        if (strcmp(w, stop[i]) == 0) return 1;
    return 0;
}

/* lower-case, split into words, drop filler + very short words, remove duplicates */
static void extractKeywords(Item *it) {
    char text[NAME_LEN + DESC_LEN + 2];
    snprintf(text, sizeof text, "%s %s", it->name, it->desc);
    it->nkw = 0;
    char word[KW_LEN];
    int len = 0;
    for (size_t i = 0;; i++) {
        char c = text[i];
        if (isalnum((unsigned char)c)) {
            if (len < KW_LEN - 1) word[len++] = (char)tolower((unsigned char)c);
        } else {
            if (len >= 3) {
                word[len] = '\0';
                int dup = 0;
                for (int k = 0; k < it->nkw; k++)
                    if (strcmp(it->kw[k], word) == 0) { dup = 1; break; }
                if (!dup && !isStopWord(word) && it->nkw < MAX_KW)
                    strcpy(it->kw[it->nkw++], word);
            }
            len = 0;
        }
        if (c == '\0') break;
    }
}

/* ---------------------------------------------------------------- */
/*  Binary Search Tree (by name)                                     */
/* ---------------------------------------------------------------- */
typedef struct BNode { Item *item; struct BNode *left, *right; } BNode;
static BNode *bstRoot = NULL;

static BNode *bstInsert(BNode *root, Item *it) {
    if (!root) {
        BNode *n = malloc(sizeof(BNode));
        n->item = it; n->left = n->right = NULL;
        return n;
    }
    if (ciCompare(it->name, root->item->name) < 0) root->left  = bstInsert(root->left, it);
    else                                           root->right = bstInsert(root->right, it);
    return root;
}

static Item *bstSearch(BNode *root, const char *name, int *cmps) {
    *cmps = 0;
    while (root) {
        (*cmps)++;
        int c = ciCompare(name, root->item->name);
        if (c == 0) return root->item;
        root = (c < 0) ? root->left : root->right;
    }
    return NULL;
}

/* ---------------------------------------------------------------- */
/*  Printing                                                         */
/* ---------------------------------------------------------------- */
static void printItem(const Item *it) {
    static const char *stat[] = {"Open", "Returned", "Removed"};
    printf("  #%d [%s] %s | %s | %s | %s | Priority %d | %s\n",
           it->id, it->type == LOST ? "LOST " : "FOUND", it->name,
           CATS[it->category], PLACES[it->place], it->date, it->priority, stat[it->status]);
}

static void bstInorder(BNode *root) {
    if (!root) return;
    bstInorder(root->left);
    if (root->item->status != REMOVED) printItem(root->item);
    bstInorder(root->right);
}

static void freeBst(BNode *r) {
    if (!r) return;
    freeBst(r->left); freeBst(r->right); free(r);
}

/* ---------------------------------------------------------------- */
/*  Merge sort                                                       */
/* ---------------------------------------------------------------- */
typedef int (*Cmp)(const Item *, const Item *);

static int cmpName(const Item *a, const Item *b)     { return ciCompare(a->name, b->name); }
static int cmpDate(const Item *a, const Item *b)     { return strcmp(a->date, b->date); }
static int cmpPriority(const Item *a, const Item *b) { return b->priority - a->priority; } /* high first */

static void merge(Item **a, Item **tmp, int l, int m, int r, Cmp cmp) {
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r)
        tmp[k++] = (cmp(a[i], a[j]) <= 0) ? a[i++] : a[j++];   /* <= keeps it stable */
    while (i <= m) tmp[k++] = a[i++];
    while (j <= r) tmp[k++] = a[j++];
    for (k = l; k <= r; k++) a[k] = tmp[k];
}

static void mergeSort(Item **a, Item **tmp, int l, int r, Cmp cmp) {
    if (l >= r) return;
    int m = (l + r) / 2;
    mergeSort(a, tmp, l, m, cmp);
    mergeSort(a, tmp, m + 1, r, cmp);
    merge(a, tmp, l, m, r, cmp);
}

/* copy non-removed items into arr, return count */
static int collectItems(Item **arr) {
    int n = 0;
    for (int i = 0; i < itemCount; i++)
        if (allItems[i]->status != REMOVED) arr[n++] = allItems[i];
    return n;
}

/* ---------------------------------------------------------------- */
/*  Max-heap of matches                                              */
/* ---------------------------------------------------------------- */
typedef struct { int score, lostId, foundId; } Match;
static Match heap[MAX_MATCHES + 1];
static int heapSize = 0;

static void heapPush(Match *h, int *n, Match m) {
    if (*n >= MAX_MATCHES) return;
    int i = ++(*n);
    h[i] = m;
    while (i > 1 && h[i / 2].score < h[i].score) {
        Match t = h[i]; h[i] = h[i / 2]; h[i / 2] = t;
        i /= 2;
    }
}

static Match heapPop(Match *h, int *n) {
    Match top = h[1];
    h[1] = h[(*n)--];
    int i = 1;
    for (;;) {
        int l = 2 * i, r = l + 1, big = i;
        if (l <= *n && h[l].score > h[big].score) big = l;
        if (r <= *n && h[r].score > h[big].score) big = r;
        if (big == i) break;
        Match t = h[i]; h[i] = h[big]; h[big] = t;
        i = big;
    }
    return top;
}

static Item *findById(int id) {
    int s;
    return hashSearch(id, &s);
}

/* ---------------------------------------------------------------- */
/*  Matching engine                                                  */
/* ---------------------------------------------------------------- */
static void runMatching(Item *it) {
    for (int i = 0; i < itemCount; i++) {
        Item *o = allItems[i];
        if (o == it || o->type == it->type || o->status != OPEN) continue;
        int score = 0;
        if (o->category == it->category) score += 20;
        if (o->place == it->place)       score += 10;
        for (int a = 0; a < it->nkw; a++)
            for (int b = 0; b < o->nkw; b++)
                if (strcmp(it->kw[a], o->kw[b]) == 0) { score += 15; break; }
        if (score >= 35) {
            score += it->priority * 3;
            Match m;
            m.score   = score;
            m.lostId  = (it->type == LOST) ? it->id : o->id;
            m.foundId = (it->type == FOUND) ? it->id : o->id;
            heapPush(heap, &heapSize, m);
        }
    }
}

static void showMatches(void) {
    Match copy[MAX_MATCHES + 1];
    int n = heapSize, shown = 0;
    memcpy(copy, heap, sizeof(Match) * (size_t)(heapSize + 1));
    printf("\n--- Top suggested matches (max-heap) ---\n");
    while (n > 0 && shown < 5) {
        Match m = heapPop(copy, &n);
        Item *l = findById(m.lostId), *f = findById(m.foundId);
        if (!l || !f || l->status != OPEN || f->status != OPEN) continue;
        printf("  Score %d:\n   LOST :", m.score);  printItem(l);
        printf("   FOUND:");                         printItem(f);
        shown++;
    }
    if (shown == 0) printf("  No matches yet.\n");
}

/* ---------------------------------------------------------------- */
/*  Queue (claims) - linked list with front and rear                 */
/* ---------------------------------------------------------------- */
typedef struct QNode { int itemId; char who[NAME_LEN]; struct QNode *next; } QNode;
static QNode *qFront = NULL, *qRear = NULL;
static int qCount = 0;

static void enqueue(int itemId, const char *who) {
    QNode *n = malloc(sizeof(QNode));
    n->itemId = itemId;
    snprintf(n->who, NAME_LEN, "%s", who);
    n->next = NULL;
    if (qRear) qRear->next = n; else qFront = n;
    qRear = n;
    qCount++;
}

static QNode *dequeue(void) {
    if (!qFront) return NULL;
    QNode *n = qFront;
    qFront = n->next;
    if (!qFront) qRear = NULL;
    qCount--;
    return n;   /* caller frees */
}

/* ---------------------------------------------------------------- */
/*  Stack (undo) - linked list                                       */
/* ---------------------------------------------------------------- */
typedef struct SNode { int type, itemId; struct SNode *next; } SNode;
static SNode *undoTop = NULL;

static void pushAction(int type, int itemId) {
    SNode *n = malloc(sizeof(SNode));
    n->type = type; n->itemId = itemId; n->next = undoTop;
    undoTop = n;
}

/* ---------------------------------------------------------------- */
/*  Graph + Dijkstra                                                 */
/* ---------------------------------------------------------------- */
typedef struct Edge { int to, w; struct Edge *next; } Edge;
static Edge *adj[NUM_PLACES];

static void addEdge(int a, int b, int w) {
    Edge *e = malloc(sizeof(Edge));
    e->to = b; e->w = w; e->next = adj[a]; adj[a] = e;
    e = malloc(sizeof(Edge));
    e->to = a; e->w = w; e->next = adj[b]; adj[b] = e;
}

static void buildMap(void) {
    /* 0 Main Gate, 1 Library, 2 Canteen, 3 Hostel, 4 Admin Block,
       5 CS Dept, 6 Sports Ground, 7 Auditorium  (distances in metres) */
    addEdge(0, 4, 150);
    addEdge(4, 1, 200);
    addEdge(0, 1, 400);
    addEdge(0, 2, 250);
    addEdge(1, 5, 180);
    addEdge(1, 2, 300);
    addEdge(2, 3, 200);
    addEdge(3, 6, 300);
    addEdge(4, 5, 220);
    addEdge(5, 7, 150);
    addEdge(6, 7, 250);
}

/* small min-heap for Dijkstra */
typedef struct { int d, v; } HE;
static HE mh[128];
static int mhSize = 0;

static void mhPush(int d, int v) {
    int i = ++mhSize;
    mh[i].d = d; mh[i].v = v;
    while (i > 1 && mh[i / 2].d > mh[i].d) {
        HE t = mh[i]; mh[i] = mh[i / 2]; mh[i / 2] = t;
        i /= 2;
    }
}

static HE mhPop(void) {
    HE top = mh[1];
    mh[1] = mh[mhSize--];
    int i = 1;
    for (;;) {
        int l = 2 * i, r = l + 1, small = i;
        if (l <= mhSize && mh[l].d < mh[small].d) small = l;
        if (r <= mhSize && mh[r].d < mh[small].d) small = r;
        if (small == i) break;
        HE t = mh[i]; mh[i] = mh[small]; mh[small] = t;
        i = small;
    }
    return top;
}

typedef struct PNode { int v; struct PNode *next; } PNode;   /* stack for printing the path */

static void shortestRoute(int src, int dst) {
    int dist[NUM_PLACES], prev[NUM_PLACES], done[NUM_PLACES];
    for (int i = 0; i < NUM_PLACES; i++) { dist[i] = INF; prev[i] = -1; done[i] = 0; }
    dist[src] = 0;
    mhSize = 0;
    mhPush(0, src);
    while (mhSize > 0) {
        HE cur = mhPop();
        if (done[cur.v]) continue;
        done[cur.v] = 1;
        for (Edge *e = adj[cur.v]; e; e = e->next) {
            if (dist[cur.v] + e->w < dist[e->to]) {
                dist[e->to] = dist[cur.v] + e->w;
                prev[e->to] = cur.v;
                mhPush(dist[e->to], e->to);
            }
        }
    }
    /* push the path from destination back to source, then pop to print in order */
    PNode *top = NULL;
    for (int v = dst; v != -1; v = prev[v]) {
        PNode *n = malloc(sizeof(PNode));
        n->v = v; n->next = top; top = n;
    }
    printf("  Route: ");
    while (top) {
        PNode *n = top; top = n->next;
        printf("%s%s", PLACES[n->v], top ? " -> " : "\n");
        free(n);
    }
    printf("  Total distance: %d m\n", dist[dst]);
}

/* ---------------------------------------------------------------- */
/*  Menu actions                                                     */
/* ---------------------------------------------------------------- */
static void reportItem(int type) {
    if (itemCount >= MAX_ITEMS) { printf("  System full.\n"); return; }
    Item *it = calloc(1, sizeof(Item));
    printf("\n--- Report %s item ---\n", type == LOST ? "LOST" : "FOUND");

    printf("Item name: ");
    readLine(it->name, NAME_LEN);
    if (it->name[0] == '\0') { printf("  Name cannot be empty.\n"); free(it); return; }

    int c;
    printf("Categories: 1=Electronics 2=Books 3=IDCards 4=Wallets 5=Others\nChoose category (1-5): ");
    c = readInt();
    if (c < 1 || c > NUM_CAT) { printf("  Invalid category.\n"); free(it); return; }
    it->category = c - 1;

    printf("Campus places: 1=Main Gate 2=Library 3=Canteen 4=Hostel 5=Admin Block "
           "6=CS Dept 7=Sports Ground 8=Auditorium\nChoose location number: ");
    c = readInt();
    if (c < 1 || c > NUM_PLACES) { printf("  Invalid location.\n"); free(it); return; }
    it->place = c - 1;

    printf("Date (YYYY-MM-DD): ");
    readLine(it->date, DATE_LEN);
    if (!validDate(it->date)) { printf("  Invalid date.\n"); free(it); return; }

    printf("Description (colour, brand, marks...): ");
    readLine(it->desc, DESC_LEN);

    it->id = nextId++;
    it->type = type;
    it->status = OPEN;
    it->priority = PRIORITY[it->category];
    extractKeywords(it);

    allItems[itemCount++] = it;
    hashInsert(it);
    catInsert(it);
    bstRoot = bstInsert(bstRoot, it);
    pushAction(ACT_REPORT, it->id);

    printf("  Saved with ID %d (priority %d).\n", it->id, it->priority);
    runMatching(it);
    printf("  Matching engine ran - check 'View suggested matches'.\n");
}

static void listSorted(void) {
    printf("\nSort by: 1=Name 2=Date 3=Priority\nChoice: ");
    int c = readInt();
    if (c == 1) {
        printf("  (BST in-order traversal)\n");
        bstInorder(bstRoot);
        return;
    }
    if (c != 2 && c != 3) { printf("  Invalid choice.\n"); return; }
    Item *arr[MAX_ITEMS], *tmp[MAX_ITEMS];
    int n = collectItems(arr);
    if (n == 0) { printf("  No items.\n"); return; }
    mergeSort(arr, tmp, 0, n - 1, c == 2 ? cmpDate : cmpPriority);
    printf("  (merge sort by %s)\n", c == 2 ? "date" : "priority");
    for (int i = 0; i < n; i++) printItem(arr[i]);
}

static void viewByCategory(void) {
    printf("\nCategories: 1=Electronics 2=Books 3=IDCards 4=Wallets 5=Others\nChoice: ");
    int c = readInt();
    if (c < 1 || c > NUM_CAT) { printf("  Invalid category.\n"); return; }
    int any = 0;
    printf("  --- %s ---\n", CATS[c - 1]);
    for (LNode *p = catList[c - 1]; p; p = p->next)
        if (p->item->status != REMOVED) { printItem(p->item); any = 1; }
    if (!any) printf("  Nothing in this category.\n");
}

static void searchById(void) {
    printf("Enter item ID: ");
    int id = readInt(), steps;
    Item *it = hashSearch(id, &steps);
    if (it && it->status != REMOVED) {
        printItem(it);
        printf("  (found via hash table in %d step(s))\n", steps);
    } else printf("  Item not found.\n");
}

static void searchByName(void) {
    char name[NAME_LEN];
    printf("Enter exact item name: ");
    readLine(name, NAME_LEN);

    Item *arr[MAX_ITEMS], *tmp[MAX_ITEMS];
    int n = collectItems(arr);
    if (n == 0) { printf("  No items.\n"); return; }

    /* 1. sequential search */
    int seq = 0;
    Item *found = NULL;
    for (int i = 0; i < n; i++) {
        seq++;
        if (ciCompare(arr[i]->name, name) == 0) { found = arr[i]; break; }
    }

    /* 2. binary search on a name-sorted array */
    mergeSort(arr, tmp, 0, n - 1, cmpName);
    int bin = 0, lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        bin++;
        int c = ciCompare(name, arr[mid]->name);
        if (c == 0) break;
        if (c < 0) hi = mid - 1; else lo = mid + 1;
    }

    /* 3. BST search */
    int bst;
    bstSearch(bstRoot, name, &bst);

    if (found) printItem(found); else printf("  Not found.\n");
    printf("  Comparisons -> Sequential: %d | Binary search: %d | BST: %d\n", seq, bin, bst);
}

static void submitClaim(void) {
    printf("ID of FOUND item you want to claim: ");
    int id = readInt();
    Item *it = findById(id);
    if (!it || it->type != FOUND || it->status != OPEN) {
        printf("  No open FOUND item with that ID.\n");
        return;
    }
    char who[NAME_LEN];
    printf("Your name: ");
    readLine(who, NAME_LEN);
    if (who[0] == '\0') { printf("  Name cannot be empty.\n"); return; }
    enqueue(id, who);
    printf("  Claim queued. Position in queue: %d\n", qCount);
}

static void processClaim(void) {
    QNode *q = dequeue();
    if (!q) { printf("  No pending claims.\n"); return; }
    Item *it = findById(q->itemId);
    if (!it || it->status != OPEN) {
        printf("  Item #%d is no longer available. Claim by %s dropped.\n", q->itemId, q->who);
    } else {
        it->status = RETURNED;
        pushAction(ACT_CLAIM, it->id);
        printf("  Item #%d (%s) handed to %s.\n", it->id, it->name, q->who);
    }
    free(q);
}

static void undoLast(void) {
    if (!undoTop) { printf("  Nothing to undo.\n"); return; }
    SNode *a = undoTop;
    undoTop = a->next;
    Item *it = findById(a->itemId);
    if (it) {
        if (a->type == ACT_CLAIM) {
            it->status = OPEN;
            printf("  Undid claim; item #%d is open again.\n", it->id);
        } else {
            it->status = REMOVED;
            printf("  Undid report; item #%d removed.\n", it->id);
        }
    }
    free(a);
}

static void directions(void) {
    printf("Item ID: ");
    int id = readInt();
    Item *it = findById(id);
    if (!it || it->status == REMOVED) { printf("  Item not found.\n"); return; }
    printf("Campus places: 1=Main Gate 2=Library 3=Canteen 4=Hostel 5=Admin Block "
           "6=CS Dept 7=Sports Ground 8=Auditorium\nYou are at (number): ");
    int c = readInt();
    if (c < 1 || c > NUM_PLACES) { printf("  Invalid location.\n"); return; }
    printf("  Item is at %s.\n", PLACES[it->place]);
    shortestRoute(c - 1, it->place);
}

static void statistics(void) {
    int open = 0, ret = 0, rem = 0;
    for (int i = 0; i < itemCount; i++) {
        if (allItems[i]->status == OPEN) open++;
        else if (allItems[i]->status == RETURNED) ret++;
        else rem++;
    }
    printf("\n--- Statistics ---\n");
    printf("  Total reports: %d (open %d, returned %d, removed %d)\n", itemCount, open, ret, rem);
    printf("  Hash table collisions so far: %d, longest chain: %d\n", collisions, longestChain());
    printf("  Pending claims: %d\n", qCount);
}

/* ---------------------------------------------------------------- */
/*  Cleanup + main                                                   */
/* ---------------------------------------------------------------- */
static void cleanup(void) {
    for (int i = 0; i < HASH_SIZE; i++)
        for (HNode *p = table[i]; p;) { HNode *n = p->next; free(p); p = n; }
    for (int i = 0; i < NUM_CAT; i++)
        for (LNode *p = catList[i]; p;) { LNode *n = p->next; free(p); p = n; }
    for (int i = 0; i < NUM_PLACES; i++)
        for (Edge *e = adj[i]; e;) { Edge *n = e->next; free(e); e = n; }
    for (QNode *q = qFront; q;) { QNode *n = q->next; free(q); q = n; }
    for (SNode *s = undoTop; s;) { SNode *n = s->next; free(s); s = n; }
    freeBst(bstRoot);
    for (int i = 0; i < itemCount; i++) free(allItems[i]);
}

static void printMenu(void) {
    printf("\n========== SMART LOST & FOUND ==========\n"
           "  1. Report LOST item\n"
           "  2. Report FOUND item\n"
           "  3. List all items (sorted)\n"
           "  4. View by category (linked list)\n"
           "  5. Search by ID (hash table)\n"
           "  6. Search by name (compare 3 methods)\n"
           "  7. View suggested matches (heap)\n"
           "  8. Submit claim (queue)\n"
           "  9. Process next claim\n"
           " 10. Undo last action (stack)\n"
           " 11. Directions to item (Dijkstra)\n"
           " 12. Statistics\n"
           "  0. Exit\n"
           "Choice: ");
}

int main(void) {
    buildMap();
    for (;;) {
        printMenu();
        switch (readInt()) {
            case 1:  reportItem(LOST);  break;
            case 2:  reportItem(FOUND); break;
            case 3:  listSorted();      break;
            case 4:  viewByCategory();  break;
            case 5:  searchById();      break;
            case 6:  searchByName();    break;
            case 7:  showMatches();     break;
            case 8:  submitClaim();     break;
            case 9:  processClaim();    break;
            case 10: undoLast();        break;
            case 11: directions();      break;
            case 12: statistics();      break;
            case 0:
                cleanup();
                printf("Goodbye!\n");
                return 0;
            default: printf("  Invalid choice.\n");
        }
    }
}
