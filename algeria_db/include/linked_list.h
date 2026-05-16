#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "types.h"

//Construction 
TList* getPersonality(FILE *f);
TList* getDatePersonality(FILE *f);
TList* getEvents(FILE *f);

/* ---- Search ---- */
void   getInfoByDates(TList *s, const char *DoB);
void   getInfoByDates2(TList *s, const char *DoD);

/* ---- Sort ---- */
TList* sortWord(TList *lst);          /* alphabetical by name */
TList* sortWord2(TList *lst);         /* ascending by name length */
TList* sortPersonality(TList *lst);   /* ascending by age */

/* ---- CRUD ---- */
TList* deletePersonality(const char *filename, TList *s, TList *a, const char *name);
TList* updatePersonality(const char *filename, TList *s, TList *a,
                         const char *name, const char *definition,
                         const char *DoB, const char *DoD);
TList* addPersonality(TList *s, TList *a,
                      const char *name, const char *DoB, const char *DoD,
                      const char *def, const char *filename);
TList* addEvents(TList *b, const char *nameEvent, const char *date,
                 const char *def, const char *filename);

/* ---- Special queries ---- */
TList* similarPersonality(TList *s, const char *word);
TList* countPersonality(TList *s, Date *prt);
TList* palindromeName(TList *s);

/* ---- Merge ---- */
TList* mergeNodes(TList *s, TList *a);    /* bidirectional */
TList* merge2Nodes(TList *s, TList *a);   /* circular      */

/* ---- Queue conversions ---- */
TQueue* sName(TList *s);
TQueue* ageP(TList *a);
TQueue* toQueue(TList *merged);

/* ---- Queue helpers ---- */
TQueue*    newQueue(void);
void       enqueue(TQueue *q, TList *node);
TQueueNode* dequeue(TQueue *q);
bool       isQueueEmpty(TQueue *q);
void       printQueue(TQueue *q);
void       freeQueue(TQueue *q);

/* ---- List helpers ---- */
TList* newNode(const char *name, const char *dob, const char *dod, const char *def);
void   appendNode(TList **head, TList *node);
void   printList(TList *head);
void   freeList(TList *head);
int    listLength(TList *head);

#endif
