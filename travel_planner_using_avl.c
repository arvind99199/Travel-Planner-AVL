#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ANSI colours ─────────────────────────────────────────── */
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

#define MAX_STR 100

/* ============================================================
   SAFE STRING INPUT HELPER
   Reads a full line (including spaces), strips newline.
   ============================================================ */
static void readLine(const char *prompt, char *buf, int maxLen)
{
    printf("%s", prompt);
    fflush(stdout);
    /* flush leftover newline from previous scanf */
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
    if (fgets(buf, maxLen, stdin))
    {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
    }
}

/* ============================================================
   ACTIVITY TYPE
   ============================================================ */
typedef enum
{
    FLIGHT,
    HOTEL,
    TOURIST,
    TRANSPORT
} ActivityType;

const char *activityName(ActivityType t)
{
    switch (t)
    {
    case FLIGHT:
        return "Flight";
    case HOTEL:
        return "Hotel Stay";
    case TOURIST:
        return "Tourist Visit";
    case TRANSPORT:
        return "Transport";
    }
    return "Unknown";
}

/* ============================================================
   DATE-TIME HELPERS
   ============================================================ */
typedef struct
{
    int year, month, day, hour, minute;
} DateTime;

/* Returns negative / 0 / positive like strcmp */
static int dt_cmp(DateTime a, DateTime b)
{
    if (a.year != b.year)
        return a.year - b.year;
    if (a.month != b.month)
        return a.month - b.month;
    if (a.day != b.day)
        return a.day - b.day;
    if (a.hour != b.hour)
        return a.hour - b.hour;
    return a.minute - b.minute;
}

static void dt_print(DateTime dt)
{
    printf("%04d-%02d-%02d %02d:%02d",
           dt.year, dt.month, dt.day, dt.hour, dt.minute);
}

/* ============================================================
   NAVIGATION AVL TREE
   Key  = stepID  (unique integer, auto-assigned)
   ============================================================ */

typedef struct NavNode
{
    int stepID;
    char direction[MAX_STR];
    double distance; /* km for this step */
    int height;
    struct NavNode *left, *right;
} NavNode;

/* ── AVL utilities (Nav) ─────────────────────────────────── */

static int nav_height(NavNode *n) 
{ 
    return n ? n->height : 0; }

static int nav_bf(NavNode *n)
{
    return n ? nav_height(n->left) - nav_height(n->right) : 0;
}

static void nav_upd(NavNode *n)
{
    if (n)
    {
        int l = nav_height(n->left), r = nav_height(n->right);
        n->height = 1 + (l > r ? l : r);
    }
}

static NavNode *nav_rotR(NavNode *y)
{
    NavNode *x = y->left, *T = x->right;
    x->right = y;
    y->left = T;
    nav_upd(y);
    nav_upd(x);
    return x;
}

static NavNode *nav_rotL(NavNode *x)
{
    NavNode *y = x->right, *T = y->left;
    y->left = x;
    x->right = T;
    nav_upd(x);
    nav_upd(y);
    return y;
}

static NavNode *nav_balance(NavNode *n)
{
    nav_upd(n);
    int bf = nav_bf(n);
    if (bf > 1)
    {
        if (nav_bf(n->left) < 0)
            n->left = nav_rotL(n->left);
        return nav_rotR(n);
    }
    if (bf < -1)
    {
        if (nav_bf(n->right) > 0)
            n->right = nav_rotR(n->right);
        return nav_rotL(n);
    }
    return n;
}

/* ── Nav AVL tree type ───────────────────────────────────── */
typedef struct
{
    NavNode *root;
    int nextID; /* always >= highest ID + 1 */
} NavTree;

void nav_init(NavTree *t)
{
    t->root = NULL;
    t->nextID = 1;
}

/* Internal insert; returns new root */
static NavNode *nav_insert_node(NavNode *node, int id,
                                const char *dir, double dist)
{
    if (!node)
    {
        NavNode *n = (NavNode *)calloc(1, sizeof(NavNode));
        if (!n)
        {
            fprintf(stderr, "[Error] Out of memory\n");
            exit(1);
        }
        n->stepID = id;
        snprintf(n->direction, MAX_STR, "%s", dir);
        n->direction[MAX_STR - 1] = '\0';
        n->direction[MAX_STR - 1] = '\0';
        n->distance = dist;
        n->height = 1;
        return n;
    }
    if (id < node->stepID)
        node->left = nav_insert_node(node->left, id, dir, dist);
    else if (id > node->stepID)
        node->right = nav_insert_node(node->right, id, dir, dist);
    else
    {
        /* duplicate stepID – update in place */
        snprintf(node->direction, MAX_STR, "%s", dir);
        node->direction[MAX_STR - 1] = '\0';
        node->direction[MAX_STR - 1] = '\0';
        node->distance = dist;
        return node;
    }
    return nav_balance(node);
}

void nav_addDirection(NavTree *t, const char *dir, double dist)
{
    int id = t->nextID++;
    t->root = nav_insert_node(t->root, id, dir, dist);
    printf("  [Nav] Direction added: Step %d – %s\n", id, dir);
}

/* Flat buffer used during insertion rebuild – max 1024 steps */
#define NAV_MAX 1024
typedef struct
{
    int id;
    char dir[MAX_STR];
    double dist;
} NavFlat;

static void nav_collect_inorder(NavNode *n, NavFlat *arr, int *cnt)
{
    if (!n)
        return;
    nav_collect_inorder(n->left, arr, cnt);
    arr[*cnt].id = n->stepID;
    snprintf(arr[*cnt].dir, MAX_STR, "%s", n->direction);
    arr[*cnt].dir[MAX_STR - 1] = '\0';
    arr[*cnt].dir[MAX_STR - 1] = '\0';
    arr[*cnt].dist = n->distance;
    (*cnt)++;
    nav_collect_inorder(n->right, arr, cnt);
}

static void nav_freeAll(NavNode *n)
{
    if (!n)
        return;
    nav_freeAll(n->left);
    nav_freeAll(n->right);
    free(n);
}

int nav_insertDirection(NavTree *t, int afterID,
                        const char *dir, double dist)
{
    /* 1. Collect all current nodes */
    NavFlat *arr = (NavFlat *)malloc(sizeof(NavFlat) * (NAV_MAX + 1));
    if (!arr)
    {
        fprintf(stderr, "[Error] OOM\n");
        exit(1);
    }
    int cnt = 0;
    nav_collect_inorder(t->root, arr, &cnt);

    /* 2. Check afterID exists */
    int found = 0;
    for (int i = 0; i < cnt; i++)
        if (arr[i].id == afterID)
        {
            found = 1;
            break;
        }

    if (!found)
    {
        printf("  [Nav] Step %d not found.\n", afterID);
        free(arr);
        return 0;
    }

    /* 3. Shift all IDs > afterID up by 1 */
    for (int i = 0; i < cnt; i++)
        if (arr[i].id > afterID)
            arr[i].id++;

    /* 4. Make room for the new entry and insert at position afterID+1 */
    arr[cnt].id = afterID + 1;
    snprintf(arr[cnt].dir, MAX_STR, "%s", dir);
    arr[cnt].dir[MAX_STR - 1] = '\0';
    arr[cnt].dir[MAX_STR - 1] = '\0';
    arr[cnt].dist = dist;
    cnt++;

    /* 5. Rebuild the tree from scratch */
    nav_freeAll(t->root);
    t->root = NULL;
    t->nextID = 1;
    for (int i = 0; i < cnt; i++)
    {
        t->root = nav_insert_node(t->root, arr[i].id,
                                  arr[i].dir, arr[i].dist);
        if (arr[i].id >= t->nextID)
            t->nextID = arr[i].id + 1;
    }

    free(arr);
    printf("  [Nav] Inserted step %d after step %d: %s\n",
           afterID + 1, afterID, dir);
    return 1;
}

/* Find min node (for deletion) */
static NavNode *nav_minNode(NavNode *n)
{
    while (n->left)
        n = n->left;
    return n;
}

static NavNode *nav_delete_node(NavNode *node, int id)
{
    if (!node)
        return NULL;
    if (id < node->stepID)
        node->left = nav_delete_node(node->left, id);
    else if (id > node->stepID)
        node->right = nav_delete_node(node->right, id);
    else
    {
        if (!node->left || !node->right)
        {
            NavNode *tmp = node->left ? node->left : node->right;
            free(node);
            return tmp;
        }
        NavNode *succ = nav_minNode(node->right);
        node->stepID = succ->stepID;
        snprintf(node->direction, MAX_STR, "%s", succ->direction);
        node->direction[MAX_STR - 1] = '\0';
        node->distance = succ->distance;
        node->right = nav_delete_node(node->right, succ->stepID);
    }
    return nav_balance(node);
}

int nav_deleteDirection(NavTree *t, int stepID)
{
    /* Verify existence first */
    NavNode *cur = t->root;
    while (cur)
    {
        if (cur->stepID == stepID)
            break;
        cur = (stepID < cur->stepID) ? cur->left : cur->right;
    }
    if (!cur)
    {
        printf("  [Nav] Step %d not found.\n", stepID);
        return 0;
    }

    t->root = nav_delete_node(t->root, stepID);
    printf("  [Nav] Step %d deleted.\n", stepID);
    return 1;
}

int nav_updateDirection(NavTree *t, int stepID,
                        const char *newDir, double newDist)
{
    NavNode *cur = t->root;
    while (cur)
    {
        if (cur->stepID == stepID)
        {
            snprintf(cur->direction, MAX_STR, "%s", newDir);
            cur->direction[MAX_STR - 1] = '\0';
            cur->direction[MAX_STR - 1] = '\0';
            cur->distance = newDist;
            printf("  [Nav] Step %d updated: %s (%.1f km)\n",
                   stepID, newDir, newDist);
            return 1;
        }
        cur = (stepID < cur->stepID) ? cur->left : cur->right;
    }
    printf("  [Nav] Step %d not found.\n", stepID);
    return 0;
}

/* Search: does 'target' appear after 'source' in in-order sequence?
   Uses a small state machine: 0=seeking src, 1=seeking tgt, 2=found */
typedef struct
{
    int mode;
} NavSrchCtx;

static void nav_inorderSearch(NavNode *n, const char *src,
                              const char *tgt, NavSrchCtx *ctx)
{
    if (!n || ctx->mode == 2)
        return;
    nav_inorderSearch(n->left, src, tgt, ctx);
    if (ctx->mode == 0 && strcmp(n->direction, src) == 0)
        ctx->mode = 1;
    else if (ctx->mode == 1 && strcmp(n->direction, tgt) == 0)
        ctx->mode = 2;
    nav_inorderSearch(n->right, src, tgt, ctx);
}

int nav_searchDirection(NavTree *t, const char *src, const char *tgt)
{
    NavSrchCtx ctx = {0};
    nav_inorderSearch(t->root, src, tgt, &ctx);
    return ctx.mode == 2;
}

/* In-order display */
static void nav_inorderDisplay(NavNode *n)
{
    if (!n)
        return;
    nav_inorderDisplay(n->left);
    printf("      [Step %2d] %-38s (%.1f km)\n",
           n->stepID, n->direction, n->distance);
    nav_inorderDisplay(n->right);
}

void nav_display(NavTree *t)
{
    if (!t->root)
    {
        printf("      (No navigation steps)\n");
        return;
    }
    nav_inorderDisplay(t->root);
}

static double nav_totalDist(NavNode *n)
{
    if (!n)
        return 0.0;
    return n->distance + nav_totalDist(n->left) + nav_totalDist(n->right);
}

double nav_totalDistance(NavTree *t) 
{ 
    return nav_totalDist(t->root);
}

void nav_free(NavTree *t)
{
    nav_freeAll(t->root);
    t->root = NULL;
    t->nextID = 1;
}

/* ============================================================
   TRIP AVL TREE
   Key  = travelDate (chronological); tie-break by activityID.
   ============================================================ */

typedef struct TripNode
{
    int activityID;
    ActivityType type;
    char location[MAX_STR];
    DateTime travelDate;
    char description[MAX_STR];

    /* Flight specific */
    char flightNumber[20];
    char airline[MAX_STR];

    /* Hotel specific */
    char hotelName[MAX_STR];
    double hotelChargePerNight;
    int nights;

    /* Navigation AVL tree to reach THIS location */
    NavTree navRoute;

    int height;
    struct TripNode *left, *right;
}TripNode;

typedef struct
{
    TripNode *root;
    int nextID;
} TripTree;

/* ── AVL utilities (Trip) ────────────────────────────────── */

static int trip_height(TripNode *n) 
{
    return n ? n->height : 0;     
}

static int trip_bf(TripNode *n)
{
    return n ? trip_height(n->left) - trip_height(n->right) : 0;
}

static void trip_upd(TripNode *n)
{
    if (n)
    {
        int l = trip_height(n->left), r = trip_height(n->right);
        n->height = 1 + (l > r ? l : r);
    }
}

static TripNode *trip_rotR(TripNode *y)
{
    TripNode *x = y->left, *T = x->right;
    x->right = y;
    y->left = T;
    trip_upd(y);
    trip_upd(x);
    return x;
}

static TripNode *trip_rotL(TripNode *x)
{
    TripNode *y = x->right, *T = y->left;
    y->left = x;
    x->right = T;
    trip_upd(x);
    trip_upd(y);
    return y;
}

static TripNode *trip_balance(TripNode *n)
{
    trip_upd(n);
    int bf = trip_bf(n);
    if (bf > 1)
    {
        if (trip_bf(n->left) < 0)
            n->left = trip_rotL(n->left);
        return trip_rotR(n);
    }
    if (bf < -1)
    {
        if (trip_bf(n->right) > 0)
            n->right = trip_rotR(n->right);
        return trip_rotL(n);
    }
    return n;
}

/* Compare two TripNodes by date then activityID */
static int trip_cmpNode(TripNode *a, TripNode *b)
{
    int c = dt_cmp(a->travelDate, b->travelDate);
    if (c != 0)
        return c;
    return a->activityID - b->activityID;
}

/* ── Trip tree helpers ───────────────────────────────────── */

/* Iterative post-order free */
static void trip_freeTree(TripNode *root)
{
    if (!root)
        return;
    /* Use an explicit stack for iterative traversal */
    TripNode **stack = (TripNode **)malloc(sizeof(TripNode *) * 4096);
    if (!stack)
    {
        fprintf(stderr, "[Error] OOM\n");
        exit(1);
    }
    int top = 0;
    stack[top++] = root;
    while (top > 0)
    {
        TripNode *cur = stack[--top];
        if (cur->left)
            stack[top++] = cur->left;
        if (cur->right)
            stack[top++] = cur->right;
        nav_free(&cur->navRoute);
        free(cur);
    }
    free(stack);
}

void trip_init(TripTree *t)
{
    trip_freeTree(t->root);
    t->root = NULL;
    t->nextID = 1;
    printf("[Trip] New trip itinerary created.\n");
}

TripNode *trip_newNode(TripTree *t, ActivityType tp,
                       const char *loc, DateTime dt, const char *desc)
{
    TripNode *n = (TripNode *)calloc(1, sizeof(TripNode));
    if (!n)
    {
        fprintf(stderr, "[Error] OOM\n");
        exit(1);
    }
    n->activityID = t->nextID++;
    n->type = tp;
    snprintf(n->location, MAX_STR, "%s", loc);
    n->location[MAX_STR - 1] = '\0';
    snprintf(n->description, MAX_STR, "%s", desc);
    n->description[MAX_STR - 1] = '\0';
    n->description[MAX_STR - 1] = '\0';
    n->travelDate = dt;
    nav_init(&n->navRoute);
    n->height = 1;
    return n;
}

/* Internal AVL insert using date+id as key */
static TripNode * trip_avl_insert(TripNode *root, TripNode *node)
{
    if (!root)
        return node;
    int c = trip_cmpNode(node, root);
    if (c < 0)
        root->left = trip_avl_insert(root->left, node);
    else
        root->right = trip_avl_insert(root->right, node);
    return trip_balance(root);
}

void trip_addActivity(TripTree *t, TripNode *node)
{
    t->root = trip_avl_insert(t->root, node);
    printf("[Trip] Added: [%d] %s @ %s on ",
           node->activityID, activityName(node->type), node->location);
    dt_print(node->travelDate);
    printf("\n");
}

void trip_insertActivity(TripTree *t, TripNode *node)
{
    t->root = trip_avl_insert(t->root, node);
    printf("[Trip] Inserted (chrono): [%d] %s @ %s on ",
           node->activityID, activityName(node->type), node->location);
    dt_print(node->travelDate);
    printf("\n");
}

/* Find by activityID (pre-order scan – key is date, not ID) */
static TripNode *trip_findID(TripNode *root, int id)
{
    if (!root)
        return NULL;
    if (root->activityID == id)
        return root;
    TripNode *l = trip_findID(root->left, id);
    return l ? l : trip_findID(root->right, id);
}

TripNode *trip_findByID(TripTree *t, int id)
{
    return trip_findID(t->root, id);
}

/* Min-node for trip deletion */
static TripNode *trip_minNode(TripNode *n)
{
    while (n->left)
        n = n->left;
    return n;
}

static TripNode *trip_avl_deleteByID(TripNode *root, int id, int *deleted)
{
    if (!root)
        return NULL;

    if (root->activityID == id)
    {
        *deleted = 1;
        if (!root->left || !root->right)
        {
            TripNode *tmp = root->left ? root->left : root->right;
            nav_free(&root->navRoute);
            free(root);
            return tmp;
        }
        /* Two children: copy successor data into current node */
        TripNode *succ = trip_minNode(root->right);
        root->activityID = succ->activityID;
        root->type = succ->type;
        root->travelDate = succ->travelDate;
        root->hotelChargePerNight = succ->hotelChargePerNight;
        root->nights = succ->nights;
        snprintf(root->location, MAX_STR, "%s", succ->location);
        snprintf(root->description, MAX_STR, "%s", succ->description);
        root->description[MAX_STR - 1] = '\0';
        snprintf(root->flightNumber, 20, "%s", succ->flightNumber);
        snprintf(root->airline, MAX_STR, "%s", succ->airline);
        snprintf(root->hotelName, MAX_STR, "%s", succ->hotelName);
        /* Transfer nav tree ownership */
        nav_free(&root->navRoute);
        root->navRoute = succ->navRoute;
        succ->navRoute.root = NULL; /* prevent double-free in successor */
        /* Now delete the successor (which has at most one child) */
        int dummy = 0;
        root->right = trip_avl_deleteByID(root->right,
                                          succ->activityID, &dummy);
    }
    else
    {
        /* Search both subtrees since AVL is ordered by date, not ID */
        root->left = trip_avl_deleteByID(root->left, id, deleted);
        root->right = trip_avl_deleteByID(root->right, id, deleted);
    }
    return trip_balance(root);
}

int trip_deleteActivity(TripTree *t, int id)
{
    int deleted = 0;
    t->root = trip_avl_deleteByID(t->root, id, &deleted);
    if (deleted)
        printf("[Trip] Activity %d deleted.\n", id);
    else
        printf("[Trip] Activity %d not found.\n", id);
    return deleted;
}

/* ── Display (in-order = chronological) ─────────────────── */

static void trip_inorderDisplay(TripNode *n, int *stepNum)
{
    if (!n)
        return;
    trip_inorderDisplay(n->left, stepNum);

    printf("\nStep %-2d | Activity #%d | %s\n",
           ++(*stepNum), n->activityID, activityName(n->type));
    printf("  Location    : %s\n", n->location);
    printf("  Date/Time   : ");
    dt_print(n->travelDate);
    printf("\n");
    if (strlen(n->description))
        printf("  Info        : %s\n", n->description);
    if (n->type == FLIGHT)
        printf("  Flight      : %s (%s)\n", n->flightNumber, n->airline);
    if (n->type == HOTEL)
        printf("  Hotel       : %s | Rs %.0f/night x %d nights = Rs %.0f\n",
               n->hotelName,
               n->hotelChargePerNight,
               n->nights,
               n->hotelChargePerNight * n->nights);
    printf("  Navigation (%.1f km total):\n",
           nav_totalDistance(&n->navRoute));
    nav_display(&n->navRoute);

    trip_inorderDisplay(n->right, stepNum);
}

void trip_display(TripTree *t)
{
    if (!t->root)
    {
        printf("[Trip] Itinerary is empty.\n");
        return;
    }
    printf("\n==================================================\n");
    printf("              TRIP ITINERARY\n");
    printf("==================================================\n");
    int stepNum = 0;
    trip_inorderDisplay(t->root, &stepNum);
    printf("==================================================\n\n");
}

/* ============================================================
   RANGE SEARCH
   Display all activities with D1 <= travelDate <= D2
   ============================================================ */

static void trip_rangeSearch(TripNode *n, DateTime d1, DateTime d2,
                             int *count)
{
    if (!n)
        return;

    int cmpLeft = dt_cmp(n->travelDate, d1);  /* < 0 means node < d1 */
    int cmpRight = dt_cmp(n->travelDate, d2); /* > 0 means node > d2 */

    /* Prune: if node date < d1, only right subtree can qualify */
    if (cmpLeft < 0)
    {
        trip_rangeSearch(n->right, d1, d2, count);
        return;
    }
    /* Prune: if node date > d2, only left subtree can qualify */
    if (cmpRight > 0)
    {
        trip_rangeSearch(n->left, d1, d2, count);
        return;
    }

    /* Node is in range: in-order left → this → right */
    trip_rangeSearch(n->left, d1, d2, count);

    printf("\n  [%d] %s @ %s\n",
           n->activityID, activityName(n->type), n->location);
    printf("       Date/Time : ");
    dt_print(n->travelDate);
    printf("\n");
    if (strlen(n->description))
        printf("       Info      : %s\n", n->description);
    if (n->type == FLIGHT)
        printf("       Flight    : %s (%s)\n", n->flightNumber, n->airline);
    if (n->type == HOTEL)
        printf("       Hotel     : %s | Rs %.0f/night x %d nights = Rs %.0f\n",
               n->hotelName,
               n->hotelChargePerNight,
               n->nights,
               n->hotelChargePerNight * n->nights);
    (*count)++;

    trip_rangeSearch(n->right, d1, d2, count);
}

void trip_printRange(TripTree *t, DateTime d1, DateTime d2)
{
    printf("\n[Range Search] Activities between ");
    dt_print(d1);
    printf("  and  ");
    dt_print(d2);
    printf("\n");
    int count = 0;
    trip_rangeSearch(t->root, d1, d2, &count);
    if (!count)
        printf("  No activities found in this date range.\n");
    else
        printf("\n  Total activities found: %d\n", count);
}

/* ============================================================
   C-1  Path exists from source to destination?
   ============================================================ */

/* Dynamically allocated in-order collection */
static void trip_collectInOrder_r(TripNode *n,
                                  TripNode ***arr, int *cnt, int *cap)
{
    if (!n)
        return;
    trip_collectInOrder_r(n->left, arr, cnt, cap);
    if (*cnt == *cap)
    {
        *cap *= 2;
        *arr = (TripNode **)realloc(*arr, sizeof(TripNode *) * (*cap));
        if (!*arr)
        {
            fprintf(stderr, "[Error] OOM\n");
            exit(1);
        }
    }
    (*arr)[(*cnt)++] = n;
    trip_collectInOrder_r(n->right, arr, cnt, cap);
}

void trip_printPath(TripTree *t, const char *src, const char *dst)
{
    printf("\n[Path] %s  ->  %s\n", src, dst);

    int cap = 64, cnt = 0;
    TripNode **arr = (TripNode **)malloc(sizeof(TripNode *) * cap);
    if (!arr)
    {
        fprintf(stderr, "[Error] OOM\n");
        exit(1);
    }
    trip_collectInOrder_r(t->root, &arr, &cnt, &cap);

    int si = -1, di = -1;
    for (int i = 0; i < cnt; i++)
    {
        if (si == -1 && strcmp(arr[i]->location, src) == 0)
            si = i;
        if (si != -1 && strcmp(arr[i]->location, dst) == 0)
        {
            di = i;
            break;
        }
    }

    if (si == -1 || di == -1)
    {
        printf("  No path found from '%s' to '%s'.\n", src, dst);
    }
    else
    {
        printf("  Path EXISTS. Travel details:\n");
        for (int i = si; i <= di; i++)
        {
            TripNode *node = arr[i];
            printf("  > %s @ %s  [",
                   activityName(node->type), node->location);
            dt_print(node->travelDate);
            printf("]\n");
            if (node->type == FLIGHT)
                printf("      Flight: %s (%s)\n",
                       node->flightNumber, node->airline);
            if (node->type == HOTEL)
                printf("      Hotel: %s | Rs %.0f x %d nights\n",
                       node->hotelName,
                       node->hotelChargePerNight, node->nights);
            printf("    Navigation steps:\n");
            nav_display(&node->navRoute);
        }
    }
    free(arr);
}

/* ============================================================
   C-2  Detect locations visited multiple times
   ============================================================ */

static void trip_collectLocs(TripNode *n, char locs[][MAX_STR],
                             int freq[], int *uCount)
{
    if (!n)
        return;
    trip_collectLocs(n->left, locs, freq, uCount);

    int matched = 0;
    for (int i = 0; i < *uCount; i++)
    {
        if (strcmp(locs[i], n->location) == 0)
        {
            freq[i]++;
            matched = 1;
            break;
        }
    }
    if (!matched)
    {
        snprintf(locs[*uCount], MAX_STR, "%s", n->location);
        locs[*uCount][MAX_STR - 1] = '\0';
        locs[*uCount][MAX_STR - 1] = '\0';
        freq[(*uCount)++] = 1;
    }

    trip_collectLocs(n->right, locs, freq, uCount);
}

void trip_detectDuplicates(TripTree *t)
{
    /* Up to 200 unique locations */
    char locs[200][MAX_STR];
    int freq[200], uCount = 0;
    trip_collectLocs(t->root, locs, freq, &uCount);

    printf("\n[Duplicate Locations]\n");
    int any = 0;
    for (int i = 0; i < uCount; i++)
    {
        if (freq[i] > 1)
        {
            printf("  %-30s – visited %d times\n", locs[i], freq[i]);
            any = 1;
        }
    }
    if (!any)
        printf("  No location visited more than once.\n");
}

/* ============================================================
   C-3  Sort by hotel charge descending
        Strategy: collect nodes in-order → sort array → display
   ============================================================ */

typedef struct
{
    int activityID;
    ActivityType type;
    char location[MAX_STR];
    DateTime travelDate;
    char description[MAX_STR];
    char flightNumber[20];
    char airline[MAX_STR];
    char hotelName[MAX_STR];
    double hotelChargePerNight;
    int nights;
} FlatActivity;

static void trip_flatCollect_r(TripNode *n,
                               FlatActivity **arr, int *cnt, int *cap)
{
    if (!n)
        return;
    trip_flatCollect_r(n->left, arr, cnt, cap);

    if (*cnt == *cap)
    {
        *cap *= 2;
        *arr = (FlatActivity *)realloc(*arr, sizeof(FlatActivity) * (*cap));
        if (!*arr)
        {
            fprintf(stderr, "[Error] OOM\n");
            exit(1);
        }
    }
    FlatActivity *f = &(*arr)[(*cnt)++];
    f->activityID = n->activityID;
    f->type = n->type;
    f->travelDate = n->travelDate;
    f->hotelChargePerNight = n->hotelChargePerNight;
    f->nights = n->nights;
    snprintf(f->location, MAX_STR, "%s", n->location);
    snprintf(f->description, MAX_STR, "%s", n->description);
    f->description[MAX_STR - 1] = '\0';
    snprintf(f->flightNumber, 20, "%s", n->flightNumber);
    snprintf(f->airline, MAX_STR, "%s", n->airline);
    snprintf(f->hotelName, MAX_STR, "%s", n->hotelName);

    trip_flatCollect_r(n->right, arr, cnt, cap);
}

static int cmpHotelDesc(const void *a, const void *b)
{
    const FlatActivity *fa = (const FlatActivity *)a;
    const FlatActivity *fb = (const FlatActivity *)b;
    double ca = (fa->type == HOTEL) ? fa->hotelChargePerNight * fa->nights : 0.0;
    double cb = (fb->type == HOTEL) ? fb->hotelChargePerNight * fb->nights : 0.0;
    if (cb > ca)
        return 1;
    if (cb < ca)
        return -1;
    return 0;
}

void trip_sortByHotelChargeDesc(TripTree *t)
{
    int cap = 64, cnt = 0;
    FlatActivity *arr = (FlatActivity *)malloc(sizeof(FlatActivity) * cap);
    if (!arr)
    {
        fprintf(stderr, "[Error] OOM\n");
        exit(1);
    }

    trip_flatCollect_r(t->root, &arr, &cnt, &cap);
    qsort(arr, cnt, sizeof(FlatActivity), cmpHotelDesc);

    printf("\n[Sort] Trip sorted by total hotel charge (descending):\n");
    printf("==================================================\n");
    for (int i = 0; i < cnt; i++)
    {
        FlatActivity *f = &arr[i];
        double totalCharge = (f->type == HOTEL)
                                 ? f->hotelChargePerNight * f->nights
                                 : 0.0;
        printf("  [%d] %-14s @ %-22s ",
               f->activityID, activityName(f->type), f->location);
        dt_print(f->travelDate);
        if (f->type == HOTEL)
            printf("  | %s: Rs %.0f x %d = Rs %.0f",
                   f->hotelName,
                   f->hotelChargePerNight, f->nights, totalCharge);
        printf("\n");
    }
    printf("==================================================\n");
    free(arr);
}

/* ============================================================
   INPUT HELPERS
   ============================================================ */

static void readActivityDetails(TripTree *t, ActivityType type,
                                const char *loc, DateTime dt,
                                const char *desc)
{
    TripNode *node = trip_newNode(t, type, loc, dt, desc);

    if (type == FLIGHT)
    {
        readLine("Enter Flight Number : ", node->flightNumber, 20);
        readLine("Enter Airline       : ", node->airline, MAX_STR);
    }
    else if (type == HOTEL)
    {
        readLine("Enter Hotel Name    : ", node->hotelName, MAX_STR);
        printf("Enter Charge Per Night (Rs): ");
        if (scanf("%lf", &node->hotelChargePerNight) != 1)
            node->hotelChargePerNight = 0.0;
        printf("Enter Number of Nights    : ");
        if (scanf("%d", &node->nights) != 1)
            node->nights = 0;
    }

    printf(YELLOW "Enter Navigation directions"
                  " (direction  distance_km; type 'done' to finish):\n" RESET);
    while (1)
    {
        char dir[MAX_STR];
        double dist;
        printf("  Direction (or 'done'): ");
        if (scanf(" %99[^\n]", dir) != 1)
            break;
        if (strcmp(dir, "done") == 0)
            break;
        printf("  Distance (km): ");
        if (scanf("%lf", &dist) != 1)
            dist = 0.0;
        nav_addDirection(&node->navRoute, dir, dist);
    }

    trip_insertActivity(t, node); /* inserts in chronological order */
}


int isLeap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int isValidDate(DateTime dt)
{
    if (dt.month < 1 || dt.month > 12)
        return 0;

    if (dt.day < 1)
        return 0;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30,
                         31, 31, 30, 31, 30, 31};

    // Leap year adjustment
    if (isLeap(dt.year))
        daysInMonth[1] = 29;

    if (dt.day > daysInMonth[dt.month - 1])
        return 0;

    // Time validation
    if (dt.hour < 0 || dt.hour > 23)
        return 0;

    if (dt.minute < 0 || dt.minute > 59)
        return 0;

    return 1;
}


/* ============================================================
   DRIVER / MAIN
   ============================================================ */

int main(void)
{
    TripTree trip = {NULL, 1};
    int choice;

    while (1)
    {
        printf(BOLD CYAN "\n============================================\n" RESET);
        printf(BOLD CYAN "       TRAVEL PLANNER SYSTEM (AVL)          \n" RESET);
        printf(BOLD CYAN "============================================\n" RESET);

        printf(BOLD "\nTRIP OPERATIONS\n" RESET);
        printf("  1.  Create Trip\n");
        printf("  2.  Add Activity (chronological)\n");
        printf("  3.  Insert Activity (chronological)\n");
        printf("  4.  Delete Activity\n");
        printf("  5.  Display Trip\n");

        printf(BOLD "\nNAVIGATION OPERATIONS\n" RESET);
        printf("  6.  Add Direction to Activity\n");
        printf("  7.  Insert Direction (After Step)\n");
        printf("  8.  Delete Direction\n");
        printf("  9.  Update Direction\n");
        printf(" 10.  Search Direction (Source to Target)\n");
        printf(" 14.  Display Navigation Route\n");

        printf(BOLD "\nOTHER FEATURES\n" RESET);
        printf(" 11.  Find Path (Source -> Destination)\n");
        printf(" 12.  Detect Duplicate Locations\n");
        printf(" 13.  Sort by Hotel Charges (Descending)\n");
        printf(" 15.  Range Search (Date Range)\n");

        printf("\n  0.  Exit\n");
        printf(CYAN "\nEnter Choice: " RESET);
        if (scanf("%d", &choice) != 1)
        {
            choice = -1;
        }

        if (choice == 0)
        {
            printf(GREEN "Exiting. Freeing memory...\n" RESET);
            trip_init(&trip); /* frees all nodes */
            break;
        }

        switch (choice)
        {

        /* ---- Trip Operations -------------------------------- */
        case 1:
            trip_init(&trip);
            break;

        case 2:
        case 3:
        {
            char loc[MAX_STR], desc[MAX_STR];
            int type;
            DateTime dt;

            readLine("Enter Location    : ", loc, MAX_STR);
            readLine("Enter Description : ", desc, MAX_STR);
            printf("Activity Type (0:Flight  1:Hotel  2:Tourist  3:Transport): ");
            if (scanf("%d", &type) != 1 || type < 0 || type > 3)
            {
                printf(RED "Invalid type.\n" RESET);
                break;
            }
            printf("Enter Date/Time (YYYY MM DD HH MM): ");
            if (scanf("%d %d %d %d %d",
                        &dt.year, &dt.month, &dt.day,
                        &dt.hour, &dt.minute) != 5)
            {
                printf("Invalid input format.\n");
                break;
            }
            else if (!isValidDate(dt))
            {
                printf("Invalid date/time.\n");
                break;
            }
            else
            {
                printf("Valid Date!\n");
            }
            readActivityDetails(&trip, (ActivityType)type, loc, dt, desc);
            break;
        }

        case 4:
        {
            int id;
            printf("Enter Activity ID to delete: ");
            if (scanf("%d", &id) == 1)
                trip_deleteActivity(&trip, id);
            break;
        }

        case 5:
            trip_display(&trip);
            break;

        /* ---- Navigation Operations -------------------------- */
        case 6:
        {
            int id;
            printf("Enter Activity ID: ");
            if (scanf("%d", &id) != 1)
                break;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
            {
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
                break;
            }
            char dir[MAX_STR];
            double dist;
            readLine("Enter Direction : ", dir, MAX_STR);
            printf("Enter Distance (km): ");
            if (scanf("%lf", &dist) != 1)
                dist = 0.0;
            nav_addDirection(&n->navRoute, dir, dist);
            break;
        }

        case 7:
        {
            int id, after;
            printf("Enter Activity ID     : ");
            if (scanf("%d", &id) != 1)
                break;
            printf("Insert after Step ID  : ");
            if (scanf("%d", &after) != 1)
                break;
            char dir[MAX_STR];
            double dist;
            readLine("Enter Direction : ", dir, MAX_STR);
            printf("Enter Distance (km)   : ");
            if (scanf("%lf", &dist) != 1)
                dist = 0.0;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
            else
                nav_insertDirection(&n->navRoute, after, dir, dist);
            break;
        }

        case 8:
        {
            int id, step;
            printf("Enter Activity ID     : ");
            if (scanf("%d", &id) != 1)
                break;
            printf("Enter Step ID to delete: ");
            if (scanf("%d", &step) != 1)
                break;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
            else
                nav_deleteDirection(&n->navRoute, step);
            break;
        }

        case 9:
        {
            int id, step;
            printf("Enter Activity ID      : ");
            if (scanf("%d", &id) != 1)
                break;
            printf("Enter Step ID to update: ");
            if (scanf("%d", &step) != 1)
                break;
            char dir[MAX_STR];
            double dist;
            readLine("Enter new Direction : ", dir, MAX_STR);
            printf("Enter new Distance (km): ");
            if (scanf("%lf", &dist) != 1)
                dist = 0.0;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
            else
                nav_updateDirection(&n->navRoute, step, dir, dist);
            break;
        }

        case 10:
        {
            int id;
            printf("Enter Activity ID       : ");
            if (scanf("%d", &id) != 1)
                break;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
            {
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
                break;
            }
            char src[MAX_STR], tgt[MAX_STR];
            printf("Enter Source direction : ");
            scanf("%s", src);
            printf("Enter Source direction : ");
            scanf("%s", tgt);
            int f = nav_searchDirection(&n->navRoute, src, tgt);
            printf(f
                       ? GREEN "  Found: '%s' appears after '%s'.\n" RESET
                       : RED "  Not Found: '%s' does not appear after '%s'.\n" RESET,
                   tgt, src);
            break;
        }

        case 14:
        {
            int id;
            printf("Enter Activity ID: ");
            if (scanf("%d", &id) != 1)
                break;
            TripNode *n = trip_findByID(&trip, id);
            if (!n)
                printf(RED "[Trip] Activity %d not found.\n" RESET, id);
            else
            {
                printf("\n[Navigation Route for Activity #%d %s @ %s]\n",
                       n->activityID, activityName(n->type), n->location);
                nav_display(&n->navRoute);
                printf("  Total distance: %.1f km\n",
                       nav_totalDistance(&n->navRoute));
            }
            break;
        }

        /* ---- Other Features --------------------------------- */
        case 11:
        {
            char src[MAX_STR], dst[MAX_STR];
            printf("Enter Source location      : ");
            scanf(" %s",src);
            printf("Enter Destination location : ");
            scanf("%s",dst);
            trip_printPath(&trip, src, dst);
            break;
        }

        case 12:
            trip_detectDuplicates(&trip);
            break;

        case 13:
            trip_sortByHotelChargeDesc(&trip);
            break;

        case 15:
        {
            DateTime d1, d2;
            printf("Enter Start Date (YYYY MM DD HH MM): ");
            if (scanf("%d %d %d %d %d",
                      &d1.year, &d1.month, &d1.day,
                      &d1.hour, &d1.minute) != 5)
            {
                printf(RED "Invalid date.\n" RESET);
                break;
            }
            printf("Enter End   Date (YYYY MM DD HH MM): ");
            if (scanf("%d %d %d %d %d",
                      &d2.year, &d2.month, &d2.day,
                      &d2.hour, &d2.minute) != 5)
            {
                printf(RED "Invalid date.\n" RESET);
                break;
            }
            trip_printRange(&trip, d1, d2);
            break;
        }

        default:
            printf(RED "Invalid Choice! Please enter 0-15.\n" RESET);
        }
    }
    return 0;
}