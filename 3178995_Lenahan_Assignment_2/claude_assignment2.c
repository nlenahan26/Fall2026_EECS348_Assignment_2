/* ===========================================================
 * email_priority.c
 *
 * Prioritizes a CEO's emails using a MaxHeap (array/list based
 * binary heap) implemented entirely from scratch -- no library
 * heap module is used.
 *
 * Priority rules:
 *   1. Sender category (higher category = read first):
 *        Boss            (5)
 *        Subordinate     (4)
 *        Peer            (3)
 *        ImportantPerson (2)
 *        OtherPerson     (1)
 *   2. Within the same category, the NEWEST date wins (read first).
 *
 * Commands read from stdin, one per line:
 *   EMAIL <sender category>,<subject line>,<date MM-DD-YYYY>
 *   NEXT
 *   READ
 *   COUNT
 * =========================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE      1024
#define MAX_SENDER    64
#define MAX_SUBJECT   512
#define MAX_DATE      16
#define INITIAL_CAPACITY 16

/* ----------------------------------------------------------- *
 * Email record stored in the heap.
 * ----------------------------------------------------------- */
typedef struct {
    char sender[MAX_SENDER];    /* sender category string, e.g. "Boss"  */
    char subject[MAX_SUBJECT];  /* subject line                         */
    char date[MAX_DATE];        /* original date string MM-DD-YYYY      */
    int  categoryRank;          /* precomputed numeric priority rank    */
    long dateValue;             /* precomputed YYYYMMDD numeric value   */
} Email;

/* ----------------------------------------------------------- *
 * MaxHeap: dynamic array based binary heap of Email records.
 * ----------------------------------------------------------- */
typedef struct {
    Email *data;      /* backing array (list)          */
    int    size;       /* number of emails currently in heap */
    int    capacity;   /* allocated capacity of data array   */
} MaxHeap;

/* ============================================================
 * Utility: trim trailing newline / carriage return from a line
 * ============================================================ */
static void trimNewline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

/* ============================================================
 * Map a sender category string to a numeric priority rank.
 * Higher number = higher priority (read sooner).
 * ============================================================ */
static int categoryToRank(const char *category) {
    if (strcmp(category, "Boss") == 0)            return 5;
    if (strcmp(category, "Subordinate") == 0)     return 4;
    if (strcmp(category, "Peer") == 0)            return 3;
    if (strcmp(category, "ImportantPerson") == 0) return 2;
    if (strcmp(category, "OtherPerson") == 0)     return 1;
    return 0; /* unknown category -- lowest priority */
}

/* ============================================================
 * Convert a date string "MM-DD-YYYY" into a comparable numeric
 * value of the form YYYYMMDD (bigger number = more recent date).
 * ============================================================ */
static long dateToValue(const char *date) {
    int month = 0, day = 0, year = 0;
    if (sscanf(date, "%d-%d-%d", &month, &day, &year) != 3) {
        return 0; /* malformed date -- treat as oldest possible */
    }
    return (long)year * 10000L + (long)month * 100L + (long)day;
}

/* ============================================================
 * MaxHeap creation / memory management
 * ============================================================ */
static void heapInit(MaxHeap *heap) {
    heap->capacity = INITIAL_CAPACITY;
    heap->size = 0;
    heap->data = (Email *)malloc(sizeof(Email) * heap->capacity);
    if (heap->data == NULL) {
        fprintf(stderr, "Error: could not allocate memory for heap.\n");
        exit(EXIT_FAILURE);
    }
}

static void heapFree(MaxHeap *heap) {
    free(heap->data);
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

/* Grow the backing array when the heap becomes full. */
static void heapGrow(MaxHeap *heap) {
    int newCapacity = heap->capacity * 2;
    Email *newData = (Email *)realloc(heap->data, sizeof(Email) * newCapacity);
    if (newData == NULL) {
        fprintf(stderr, "Error: could not grow heap memory.\n");
        exit(EXIT_FAILURE);
    }
    heap->data = newData;
    heap->capacity = newCapacity;
}

/* ============================================================
 * Compare two emails for max-heap ordering.
 * Returns 1 if email 'a' has strictly HIGHER priority than 'b',
 * otherwise returns 0.
 * ============================================================ */
static int hasHigherPriority(const Email *a, const Email *b) {
    if (a->categoryRank != b->categoryRank) {
        return a->categoryRank > b->categoryRank;
    }
    /* Same category: newer date (larger dateValue) wins. */
    return a->dateValue > b->dateValue;
}

static void swapEmails(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

/* ============================================================
 * Sift the element at index 'idx' UP toward the root until the
 * max-heap property is restored (used after inserting).
 * ============================================================ */
static void siftUp(MaxHeap *heap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (hasHigherPriority(&heap->data[idx], &heap->data[parent])) {
            swapEmails(&heap->data[idx], &heap->data[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

/* ============================================================
 * Sift the element at index 'idx' DOWN toward the leaves until
 * the max-heap property is restored (used after removing root).
 * ============================================================ */
static void siftDown(MaxHeap *heap, int idx) {
    while (1) {
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;
        int largest = idx;

        if (left < heap->size && hasHigherPriority(&heap->data[left], &heap->data[largest])) {
            largest = left;
        }
        if (right < heap->size && hasHigherPriority(&heap->data[right], &heap->data[largest])) {
            largest = right;
        }
        if (largest == idx) {
            break;
        }
        swapEmails(&heap->data[idx], &heap->data[largest]);
        idx = largest;
    }
}

/* ============================================================
 * Insert a new email into the heap.
 * ============================================================ */
static void heapInsert(MaxHeap *heap, const char *sender, const char *subject, const char *date) {
    if (heap->size == heap->capacity) {
        heapGrow(heap);
    }

    Email *slot = &heap->data[heap->size];
    strncpy(slot->sender, sender, MAX_SENDER - 1);
    slot->sender[MAX_SENDER - 1] = '\0';
    strncpy(slot->subject, subject, MAX_SUBJECT - 1);
    slot->subject[MAX_SUBJECT - 1] = '\0';
    strncpy(slot->date, date, MAX_DATE - 1);
    slot->date[MAX_DATE - 1] = '\0';
    slot->categoryRank = categoryToRank(sender);
    slot->dateValue = dateToValue(date);

    heap->size++;
    siftUp(heap, heap->size - 1);
}

/* ============================================================
 * Peek at the highest priority email without removing it.
 * Returns 1 on success (and fills 'out'), 0 if heap is empty.
 * ============================================================ */
static int heapPeek(MaxHeap *heap, Email *out) {
    if (heap->size == 0) {
        return 0;
    }
    *out = heap->data[0];
    return 1;
}

/* ============================================================
 * Remove the highest priority email from the heap.
 * Returns 1 on success, 0 if the heap was already empty.
 * ============================================================ */
static int heapExtractMax(MaxHeap *heap) {
    if (heap->size == 0) {
        return 0;
    }
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    if (heap->size > 0) {
        siftDown(heap, 0);
    }
    return 1;
}

/* ============================================================
 * Command handling
 * ============================================================ */

/* Parses "EMAIL <category>,<subject>,<date>" (the part AFTER
 * the literal "EMAIL " keyword has already been isolated in
 * 'rest') and inserts the resulting email into the heap.        */
static void handleEmailCommand(MaxHeap *heap, char *rest) {
    char *firstComma = strchr(rest, ',');
    char *lastComma  = strrchr(rest, ',');

    if (firstComma == NULL || lastComma == NULL || firstComma == lastComma) {
        fprintf(stderr, "Warning: malformed EMAIL command ignored: %s\n", rest);
        return;
    }

    /* Category = text before the first comma */
    size_t categoryLen = (size_t)(firstComma - rest);
    char category[MAX_SENDER];
    if (categoryLen >= MAX_SENDER) categoryLen = MAX_SENDER - 1;
    strncpy(category, rest, categoryLen);
    category[categoryLen] = '\0';

    /* Date = text after the last comma */
    char date[MAX_DATE];
    strncpy(date, lastComma + 1, MAX_DATE - 1);
    date[MAX_DATE - 1] = '\0';

    /* Subject = everything between the first and last comma */
    size_t subjectLen = (size_t)(lastComma - (firstComma + 1));
    char subject[MAX_SUBJECT];
    if (subjectLen >= MAX_SUBJECT) subjectLen = MAX_SUBJECT - 1;
    strncpy(subject, firstComma + 1, subjectLen);
    subject[subjectLen] = '\0';

    heapInsert(heap, category, subject, date);
}

static void handleNextCommand(MaxHeap *heap) {
    Email top;
    if (!heapPeek(heap, &top)) {
        printf("\nNo emails to read.\n");
        return;
    }
    printf("\nNext email:\n");
    printf("\tSender: %s\n", top.sender);
    printf("\tSubject: %s\n", top.subject);
    printf("\tDate: %s\n", top.date);
}

static void handleReadCommand(MaxHeap *heap) {
    /* Removes the highest priority email without displaying it.
     * If the queue is already empty, do nothing. */
    heapExtractMax(heap);
}

static void handleCountCommand(MaxHeap *heap) {
    if (heap->size == 1) {
        printf("There is 1 email to read.\n");
    } else {
        printf("There are %d emails to read.\n", heap->size);
    }
}

/* ============================================================
 * Main: reads commands line by line from stdin.
 * ============================================================ */
int main(void) {
    MaxHeap heap;
    heapInit(&heap);

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        trimNewline(line);

        /* Skip blank lines */
        if (line[0] == '\0') {
            continue;
        }

        if (strncmp(line, "EMAIL ", 6) == 0) {
            handleEmailCommand(&heap, line + 6);
        } else if (strcmp(line, "NEXT") == 0) {
            handleNextCommand(&heap);
        } else if (strcmp(line, "READ") == 0) {
            handleReadCommand(&heap);
        } else if (strcmp(line, "COUNT") == 0) {
            handleCountCommand(&heap);
        } else {
            fprintf(stderr, "Warning: unrecognized command ignored: %s\n", line);
        }
    }

    heapFree(&heap);
    return 0;
}
