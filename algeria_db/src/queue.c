#include "../include/algeria_db.h"

TQueue* createQueue(void) {
    TQueue *q = (TQueue*)calloc(1, sizeof(TQueue));
    if (!q) { perror("calloc"); exit(1); }
    return q;
}

void enqueue(TQueue *q, const char *name, const char *def,
             const char *dob, const char *dod, int type) {
    if (!q) return;
    QNode *n = (QNode*)calloc(1, sizeof(QNode));
    if (!n) { perror("calloc"); exit(1); }
    strncpy(n->name,       name ? name : "", MAX_NAME - 1);
    strncpy(n->definition, def  ? def  : "", MAX_DEF  - 1);
    strncpy(n->dob,        dob  ? dob  : "", MAX_DATE - 1);
    strncpy(n->dod,        dod  ? dod  : "", MAX_DATE - 1);
    n->type = type;
    n->next = NULL;
    if (!q->rear) {
        q->front = q->rear = n;
    } else {
        q->rear->next = n;
        q->rear = n;
    }
    q->size++;
}

QNode* dequeue(TQueue *q) {
    if (!q || !q->front) return NULL;
    QNode *n = q->front;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    n->next = NULL;
    q->size--;
    return n;
}

void printQueue(TQueue *q) {
    if (!q || !q->front) { printf("  (empty queue)\n"); return; }
    QNode *cur = q->front;
    int idx = 1;
    while (cur) {
        printf("  [%d] %s | DOB: %s | DOD: %s\n      %s\n",
               idx++, cur->name, cur->dob, cur->dod, cur->definition);
        cur = cur->next;
    }
}

void freeQueue(TQueue *q) {
    if (!q) return;
    QNode *cur = q->front;
    while (cur) {
        QNode *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    free(q);
}
