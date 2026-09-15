#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 256
#define MAX_SUBJECT_LEN 128
#define MAX_CATEGORY_LEN 32

typedef struct {
    char sender[MAX_CATEGORY_LEN];
    char subject[MAX_SUBJECT_LEN];
    char date[11]; // MM-DD-YYYY\0
    int category_rank;
    int year;
    int month;
    int day;
} Email;

typedef struct {
    Email *arr;
    int capacity;
    int size;
} MaxHeap;

// Returns priority score for sender category (higher number = higher priority)
int get_category_rank(const char *category) {
    if (strcmp(category, "Boss") == 0) return 5;
    if (strcmp(category, "Subordinate") == 0) return 4;
    if (strcmp(category, "Peer") == 0) return 3;
    if (strcmp(category, "ImportantPerson") == 0) return 2;
    if (strcmp(category, "OtherPerson") == 0) return 1;
    return 0;
}

// Parses date MM-DD-YYYY into integer components
void parse_date(const char *date, int *month, int *day, int *year) {
    sscanf(date, "%d-%d-%d", month, day, year);
}

// Compares two emails. Returns positive if a > b, negative if a < b, 0 if equal.
int compare_emails(const Email *a, const Email *b) {
    // Primary criteria: Sender category priority
    if (a->category_rank != b->category_rank) {
        return a->category_rank - b->category_rank;
    }
    // Secondary criteria: Date (Newer date has higher priority)
    if (a->year != b->year) return a->year - b->year;
    if (a->month != b->month) return a->month - b->month;
    return a->day - b->day;
}

// Initializes heap
MaxHeap* create_heap(int initial_capacity) {
    MaxHeap *heap = (MaxHeap*)malloc(sizeof(MaxHeap));
    heap->capacity = initial_capacity;
    heap->size = 0;
    heap->arr = (Email*)malloc(sizeof(Email) * heap->capacity);
    return heap;
}

void swap(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

// Maintains MaxHeap property upward
void heapify_up(MaxHeap *heap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (compare_emails(&heap->arr[idx], &heap->arr[parent]) > 0) {
            swap(&heap->arr[idx], &heap->arr[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

// Maintains MaxHeap property downward
void heapify_down(MaxHeap *heap, int idx) {
    int largest = idx;
    while (1) {
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;

        if (left < heap->size && compare_emails(&heap->arr[left], &heap->arr[largest]) > 0) {
            largest = left;
        }
        if (right < heap->size && compare_emails(&heap->arr[right], &heap->arr[largest]) > 0) {
            largest = right;
        }

        if (largest != idx) {
            swap(&heap->arr[idx], &heap->arr[largest]);
            idx = largest;
        } else {
            break;
        }
    }
}

// Dynamic array insertion into heap
void insert_email(MaxHeap *heap, Email email) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->arr = (Email*)realloc(heap->arr, sizeof(Email) * heap->capacity);
    }
    heap->arr[heap->size] = email;
    heap->size++;
    heapify_up(heap, heap->size - 1);
}

// Deletes and removes top priority email
void delete_max(MaxHeap *heap) {
    if (heap->size == 0) return;
    heap->arr[0] = heap->arr[heap->size - 1];
    heap->size--;
    if (heap->size > 0) {
        heapify_down(heap, 0);
    }
}

// Frees heap allocation
void free_heap(MaxHeap *heap) {
    free(heap->arr);
    free(heap);
}

int main() {
    MaxHeap *heap = create_heap(10);
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), stdin)) {
        // Strip trailing newline character
        line[strcspn(line, "\r\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strncmp(line, "EMAIL ", 6) == 0) {
            char *data = line + 6;
            char category[MAX_CATEGORY_LEN];
            char subject[MAX_SUBJECT_LEN];
            char date[11];

            // Parse formatted strings delimited by commas
            char *token1 = strtok(data, ",");
            char *token2 = strtok(NULL, ",");
            char *token3 = strtok(NULL, ",");

            if (token1 && token2 && token3) {
                strncpy(category, token1, sizeof(category));
                strncpy(subject, token2, sizeof(subject));
                strncpy(date, token3, sizeof(date));

                Email email;
                strncpy(email.sender, category, sizeof(email.sender));
                strncpy(email.subject, subject, sizeof(email.subject));
                strncpy(email.date, date, sizeof(email.date));
                email.category_rank = get_category_rank(category);
                parse_date(date, &email.month, &email.day, &email.year);

                insert_email(heap, email);
            }
        } else if (strcmp(line, "COUNT") == 0) {
            printf("There are %d emails to read.\n", heap->size);
        } else if (strcmp(line, "NEXT") == 0) {
            if (heap->size > 0) {
                Email top = heap->arr[0];
                printf("\nNext email:\n");
                printf("\tSender: %s\n", top.sender);
                printf("\tSubject: %s\n", top.subject);
                printf("\tDate: %s\n\n", top.date);
            }
        } else if (strcmp(line, "READ") == 0) {
            if (heap->size > 0) {
                delete_max(heap);
            }
        }
    }

    free_heap(heap);
    return 0;
}