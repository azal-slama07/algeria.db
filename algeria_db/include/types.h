#define _GNU_SOURCE
#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_NAME 256
#define MAX_DEF 1024
#define MAX_DATE 20
#define MAX_LINE 2048

//Date
typedef struct {
    int day;
    int month;
    int year;
} Date;

/Personality, event node
typedef struct TNode {
    char name[MAX_NAME];
    char definition[MAX_DEF];
    char dob[MAX_DATE];   /* date of birth  (or event date) */
    char dod[MAX_DATE];   /* date of death  (empty for events) */
    struct TNode *next;
    struct TNode *prev;     /* used for bidirectional list */
} TList;

/* ---------- Stack node ---------- */
typedef struct SNode {
    char name[MAX_NAME];
    char definition[MAX_DEF];
    char dob[MAX_DATE];
    char dod[MAX_DATE];
    struct SNode *next;
} TStack;

//Queue node
typedef struct QNode {
    char name[MAX_NAME];
    char definition[MAX_DEF];
    char dob[MAX_DATE];
    char dod[MAX_DATE];
    struct QNode *next;
} TQueueNode;

typedef struct {
    TQueueNode *head;
    TQueueNode *tail;
} TQueue;

//BST node
typedef struct TTreeNode {
    char name[MAX_NAME];
    char definition[MAX_DEF];
    char dob[MAX_DATE];
    char dod[MAX_DATE];
    struct TTreeNode *left;
    struct TTreeNode *right;
} TTree;

//Helper to parse a date string DD/MM/YYYY or YYYY
Date parseDate(const char *s) {
    Date d;
    char temp[MAX_DATE];
    d = {0, 0, 0};
    
    //checks if the pointer s is NULL;
    if (!s || s[0] == '\0') return d;
    
    if (strchr(s, '/')) {
        // DD/MM/YYYY format
        strcpy(temp, s);
        d.day = atoi(strtok(temp, "/"));
        d.month = atoi(strtok(NULL, "/"));
        d.year = atoi(strtok(NULL, "/"));
    } else {
        // YYYY only format
        d.year = atoi(s);
    }
    
    return d;
}

static inline int dateToYear(const char *s) {
    Date d = parseDate(s);
    return d.year;
}

static inline int personalityAge(const char *dob, const char *dod) {
    int y1 = dateToYear(dob);
    int y2 = dateToYear(dod);
    if (y1 == 0 || y2 == 0) return 0;
    return y2 - y1;
}

#endif /* TYPES_H */
