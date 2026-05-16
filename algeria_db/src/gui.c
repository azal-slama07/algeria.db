#include "raylib.h"
#include "types.h"
#include "file_parser.h"
#include "linked_list.h"
#include "stack.h"
#include "tree.h"
#include "recursion.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//CONSTANTS & COLOURS
#define SCREEN_W   1100
#define SCREEN_H   700
#define FONT_SIZE  18
#define LINE_H     24
#define PAD        14
#define MAX_RESULT_LINES 200
#define MAX_LINE_LEN     300

//colour palette
#define COL_BG CLITERAL(Color){15,  15,  30,  255}
#define COL_PANEL CLITERAL(Color){25,  25,  55,  255}
#define COL_SIDEBAR CLITERAL(Color){20,  20,  45,  255}
#define COL_BTN CLITERAL(Color){40,  80, 160,  255}
#define COL_BTN_HOV CLITERAL(Color){60, 110, 210,  255}
#define COL_BTN_ACT CLITERAL(Color){30,  60, 130,  255}
#define COL_ACCENT CLITERAL(Color){80, 180, 255,  255}
#define COL_GREEN CLITERAL(Color){60, 200, 100,  255}
#define COL_RED CLITERAL(Color){220, 70,  70,  255}
#define COL_GOLD CLITERAL(Color){220,  180,  60,  255}
#define COL_TEXT CLITERAL(Color){220,  220,  235,  255}
#define COL_DIMTEXT CLITERAL(Color){130,  130,  160,  255}
#define COL_INPUT_BG CLITERAL(Color){30,  30,  65,  255}
#define COL_INPUT_ACT CLITERAL(Color){35,  50,  90,  255}
#define COL_BORDER CLITERAL(Color){60,  60, 100,  255}
#define COL_ROW_A CLITERAL(Color){28, 28,  58,  255}
#define COL_ROW_B CLITERAL(Color){22, 22,  48,  255}

//GLOBAL DATA
#define DATA_FILE "data/history.txt"

FILE   *gFile          = NULL;
TList  *gPersonalities = NULL;
TList  *gDates         = NULL;
TList  *gEvents        = NULL;
TStack *gStack         = NULL;
TTree  *gTree          = NULL;

/* ================================================================
   RESULT BUFFER  â€“ replaces printf
   ================================================================ */
char  resultLines[MAX_RESULT_LINES][MAX_LINE_LEN];
int   resultCount  = 0;
int   resultScroll = 0;

void resBuf(const char *fmt, ...) {
    va_list args;
    if (resultCount >= MAX_RESULT_LINES) return;
    va_start(args, fmt);
    vsnprintf(resultLines[resultCount], MAX_LINE_LEN, fmt, args);
    va_end(args);
    resultCount++;
}

void resClear() { resultCount = 0; resultScroll = 0; }

/* ================================================================
   DATA HELPERS
   ================================================================ */
void reloadData() {
    TList *merged;
    if (gFile) fclose(gFile);
    gFile = fopen(DATA_FILE, "r+");
    if (!gFile) gFile = fopen(DATA_FILE, "w+");
    freeList(gPersonalities); freeList(gDates); freeList(gEvents);
    freeStack(gStack); freeTree(gTree);
    gPersonalities = getPersonality(gFile);
    gDates         = getDatePersonality(gFile);
    gEvents        = getEvents(gFile);
    merged  = mergeNodes(gPersonalities, gDates);
    gStack         = toStack(merged);
    gTree          = toTree(gStack);
    freeList(merged);
}

/* Dump a TList into the result buffer */
void dumpList(TList *head) {
    int i = 1;
    if (!head) { resBuf("  (empty)"); return; }
    while (head) {
        resBuf("[%d]  %s", i, head->name);
        if (head->dob[0]) resBuf("Born : %s", head->dob);
        if (head->dod[0]) resBuf("Died : %s", head->dod);
        if (head->definition[0]) resBuf("Info : %s", head->definition);
        resBuf(" ");
        head = head->next;
        i++;
    }
}

void dumpStack(TStack *s) {
    int i = 1;
    if (!s) { resBuf("  (empty)"); return; }
    while (s) {
        resBuf("[%d]  %s  (Born:%s  Died:%s)", i, s->name, s->dob, s->dod);
        s = s->next; i++;
    }
}

void dumpQueue(TQueue *q) {
    TQueueNode *cur;
    int i = 1;
    if (!q || !q->front) { resBuf("  (empty)"); return; }
    cur = q->front;
    while (cur) {
        resBuf("[%d]  %s  (Born:%s  Died:%s)", i, cur->name, cur->dob, cur->dod);
        cur = cur->next; i++;
    }
}

void dumpTree(TTree *tr) {
    /* in-order, store into result buffer */
    if (!tr) return;
    dumpTree(tr->left);
    resBuf("  %s  (Born:%s  Died:%s)", tr->name, tr->dob, tr->dod);
    dumpTree(tr->right);
}

/* ================================================================
   UI HELPERS
   ================================================================ */

/* Draw a rounded button, return 1 if clicked */
int drawButton(int x, int y, int w, int h, const char *label, Color base) {
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    Vector2   mp = GetMousePosition();
    int hover  = CheckCollisionPointRec(mp, r);
    int clicked = hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
    Color col  = hover ? COL_BTN_HOV : base;
    if (hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) col = COL_BTN_ACT;
    DrawRectangleRounded(r, 0.2f, 6, col);
    DrawRectangleRoundedLines(r, 0.2f, 6, 1.5f, COL_BORDER);
    int tw = MeasureText(label, FONT_SIZE - 2);
    DrawText(label, x + w/2 - tw/2, y + h/2 - (FONT_SIZE-2)/2, FONT_SIZE-2, COL_TEXT);
    return clicked;
}

/* Single-line text input box */
typedef struct {
    char  buf[MAX_LINE_LEN];
    int   len;
    int   active;
} InputBox;

void initInput(InputBox *b) { b->buf[0]='\0'; b->len=0; b->active=0; }

void drawInput(InputBox *b, int x, int y, int w, int h, const char *hint) {
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    Vector2   mp = GetMousePosition();
    Color      bg;
    const char *display;
    Color      tcol;
    int        cx;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        b->active = CheckCollisionPointRec(mp, r);
    bg = b->active ? COL_INPUT_ACT : COL_INPUT_BG;
    DrawRectangleRounded(r, 0.15f, 6, bg);
    DrawRectangleRoundedLines(r, 0.15f, 6, 1.5f, b->active ? COL_ACCENT : COL_BORDER);

    if (b->active) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key < 127 && b->len < MAX_LINE_LEN-2) {
                b->buf[b->len] = (char)key;
                b->len++;
                b->buf[b->len] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && b->len > 0) {
            b->len--; b->buf[b->len] = '\0';
        }
    }

    display = (b->len == 0 && !b->active) ? hint : b->buf;
    tcol    = (b->len == 0 && !b->active) ? COL_DIMTEXT : COL_TEXT;
    DrawText(display, x + PAD/2, y + h/2 - FONT_SIZE/2, FONT_SIZE, tcol);

    /* blinking cursor */
    if (b->active && ((int)(GetTime()*2) % 2 == 0)) {
        cx = x + PAD/2 + MeasureText(b->buf, FONT_SIZE);
        DrawLine(cx, y+4, cx, y+h-4, COL_ACCENT);
    }
}

/* Draw scrollable result panel */
void drawResultPanel(int x, int y, int w, int h) {
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    float wheel;
    int visibleLines;
    int i;
    int idx;
    int sbH;
    int thumbH;
    int thumbY;
    DrawRectangleRounded(r, 0.05f, 4, COL_PANEL);
    DrawRectangleRoundedLines(r, 0.05f, 4, 1.5f, COL_BORDER);

    /* mouse wheel scroll */
    wheel = GetMouseWheelMove();
    if (wheel != 0 && CheckCollisionPointRec(GetMousePosition(), r)) {
        resultScroll -= (int)wheel * 3;
        if (resultScroll < 0) resultScroll = 0;
        {
            int maxScroll = resultCount - (h - PAD*2) / LINE_H;
            if (maxScroll < 0) maxScroll = 0;
            if (resultScroll > maxScroll) resultScroll = maxScroll;
        }
    }

    BeginScissorMode(x+2, y+2, w-4, h-4);
    visibleLines = (h - PAD*2) / LINE_H;
    for (i = 0; i < visibleLines; i++) {
        Color tc = COL_TEXT;
        idx = i + resultScroll;
        if (idx >= resultCount) break;
        /* alternating row tint */
        DrawRectangle(x+2, y+PAD + i*LINE_H - 2, w-4, LINE_H,
                      (idx%2==0) ? COL_ROW_A : COL_ROW_B);
        /* colour coded lines */
        if (resultLines[idx][0]=='[') tc = COL_ACCENT;
        if (strstr(resultLines[idx],"Born")) tc = COL_GREEN;
        if (strstr(resultLines[idx],"Died")) tc = COL_RED;
        if (strstr(resultLines[idx],"Info")) tc = COL_GOLD;
        DrawText(resultLines[idx], x+PAD, y+PAD + i*LINE_H, FONT_SIZE, tc);
    }
    EndScissorMode();

    /* scrollbar */
    if (resultCount > visibleLines) {
        sbH = h - PAD*2;
        thumbH = sbH * visibleLines / resultCount;
        if (thumbH < 20) thumbH = 20;
        thumbY = y + PAD + (sbH - thumbH) * resultScroll /
                 (resultCount - visibleLines + 1);
        DrawRectangle(x+w-8, y+PAD, 4, sbH, COL_BORDER);
        DrawRectangle(x+w-8, thumbY, 4, thumbH, COL_ACCENT);
    }
}

/* Draw section title */
void drawTitle(int x, int y, const char *text) {
    DrawText(text, x, y, FONT_SIZE+2, COL_ACCENT);
    DrawLine(x, y+FONT_SIZE+4, x+MeasureText(text,FONT_SIZE+2), y+FONT_SIZE+4, COL_ACCENT);
}

/* Draw a label */
void drawLabel(int x, int y, const char *text) {
    DrawText(text, x, y, FONT_SIZE-2, COL_DIMTEXT);
}

/* ================================================================
   SCREENS / VIEWS
   ================================================================ */
typedef enum {
    SCR_HOME = 0,
    SCR_LIST,
    SCR_STACK,
    SCR_BST,
    SCR_RECURSION
} Screen;

Screen currentScreen = SCR_HOME;

/* ================================================================
   HOME SCREEN
   ================================================================ */
void drawHome() {
    /* big title */
    const char *title = "Algeria History Database";
    const char *sub   = "NSCS  |  Algorithms & Dynamic Data Structures  |  2025-2026";
    int tw;
    int sw;
    int pCount;
    int eCount;
    int sCount;
    int tSize;
    int i;
    int bx;
    int by;
    int nw;
    int lw;

    tw = MeasureText(title, 36);
    DrawText(title, SCREEN_W/2 - tw/2, 60, 36, COL_ACCENT);

    sw = MeasureText(sub, FONT_SIZE);
    DrawText(sub, SCREEN_W/2 - sw/2, 108, FONT_SIZE, COL_DIMTEXT);

    /* stat boxes */
    pCount = listLength(gPersonalities);
    eCount = listLength(gEvents);
    sCount = stackSize(gStack);
    tSize  = treeSize(gTree);

    struct { const char *label; int val; Color col; } stats[] = {
        {"Personalities", pCount, COL_ACCENT},
        {"Events",        eCount, COL_GREEN },
        {"Stack nodes",   sCount, COL_GOLD  },
        {"BST nodes",     tSize,  COL_RED   },
    };
    for (i = 0; i < 4; i++) {
        bx = SCREEN_W/2 - 440 + i*230;
        by = 160;
        Rectangle r = {(float)bx,(float)by,210,90};
        DrawRectangleRounded(r, 0.2f, 6, COL_PANEL);
        DrawRectangleRoundedLines(r, 0.2f, 6, 1.5f, stats[i].col);
        {
            char num[16];
            sprintf(num, "%d", stats[i].val);
            nw = MeasureText(num, 36);
            DrawText(num, bx+105-nw/2, by+14, 36, stats[i].col);
        }
        lw = MeasureText(stats[i].label, FONT_SIZE-2);
        DrawText(stats[i].label, bx+105-lw/2, by+58, FONT_SIZE-2, COL_DIMTEXT);
    }

    /* nav buttons */
    struct { const char *label; Screen sc; Color col; } btns[] = {
        {"Linked List & Queues", SCR_LIST,      COL_BTN},
        {"Stack",                SCR_STACK,     COL_BTN},
        {"Binary Search Tree",   SCR_BST,       COL_BTN},
        {"Recursion",            SCR_RECURSION, COL_BTN},
    };
    for (i = 0; i < 4; i++) {
        bx = SCREEN_W/2 - 440 + i*230;
        if (drawButton(bx, 280, 210, 54, btns[i].label, btns[i].col))
            currentScreen = btns[i].sc;
    }

    /* data file info */
    DrawText("Data file: data/history.txt", PAD, SCREEN_H-30, FONT_SIZE-4, COL_DIMTEXT);
    if (drawButton(SCREEN_W-160, SCREEN_H-44, 140, 32, "Reload Data", COL_BTN)) {
        reloadData();
        resClear();
        resBuf("Data reloaded from file.");
        resBuf("%d personalities, %d events.", listLength(gPersonalities), listLength(gEvents));
    }
}

/* ================================================================
   LINKED LIST SCREEN
   ================================================================ */

/* sub-actions */
typedef enum {
    LL_NONE=0,
    LL_SHOW_PERS, LL_SHOW_EVT,
    LL_SEARCH_DOB, LL_SEARCH_DOD,
    LL_SORT_ALPHA, LL_SORT_LEN, LL_SORT_AGE,
    LL_ADD_PERS, LL_ADD_EVT,
    LL_DELETE, LL_UPDATE,
    LL_SIMILAR, LL_PALINDROME, LL_MERGE,
    LL_Q_NAMES, LL_Q_AGE
} LLAction;

LLAction llAction = LL_NONE;

InputBox llIn1, llIn2, llIn3, llIn4;  /* name, def, dob, dod */

void drawLinkedList() {
    /* sidebar buttons */
    int sx = PAD;
    int sy = 60;
    int sw2 = 210;
    int sh = 32;
    int gap = 6;
    int n = 16;
    int i;

    struct { const char *label; LLAction act; } items[] = {
        {"Show Personalities",   LL_SHOW_PERS},
        {"Show Events",          LL_SHOW_EVT},
        {"Search by Birth Date", LL_SEARCH_DOB},
        {"Search by Death Date", LL_SEARCH_DOD},
        {"Sort Alphabetically",  LL_SORT_ALPHA},
        {"Sort by Name Length",  LL_SORT_LEN},
        {"Sort by Age",          LL_SORT_AGE},
        {"Add Personality",      LL_ADD_PERS},
        {"Add Event",            LL_ADD_EVT},
        {"Delete Personality",   LL_DELETE},
        {"Update Personality",   LL_UPDATE},
        {"Similar by Year",      LL_SIMILAR},
        {"Palindrome Names",     LL_PALINDROME},
        {"Merge (bidirectional)",LL_MERGE},
        {"Queue by Word Count",  LL_Q_NAMES},
        {"Queue by Age",         LL_Q_AGE},
    };
    for (i = 0; i < n; i++) {
        Color col = (llAction == items[i].act) ? COL_BTN_ACT : COL_BTN;
        if (drawButton(sx, sy + i*(sh+gap), sw2, sh, items[i].label, col)) {
            llAction = items[i].act;
            resClear();
            initInput(&llIn1); initInput(&llIn2);
            initInput(&llIn3); initInput(&llIn4);

            /* immediate actions (no input needed) */
            if (llAction == LL_SHOW_PERS)  { dumpList(gPersonalities); llAction=LL_NONE; }
            if (llAction == LL_SHOW_EVT)   { dumpList(gEvents);        llAction=LL_NONE; }
            if (llAction == LL_SORT_ALPHA) {
                TList *s = sortWord(gPersonalities); dumpList(s); llAction=LL_NONE;
            }
            if (llAction == LL_SORT_LEN) {
                TList *s = sortWord2(gPersonalities); dumpList(s); llAction=LL_NONE;
            }
            if (llAction == LL_SORT_AGE) {
                TList *s = sortPersonality(gPersonalities); dumpList(s); llAction=LL_NONE;
            }
            if (llAction == LL_PALINDROME) {
                TList *s = palindromeName(gPersonalities); dumpList(s);
                freeList(s); llAction=LL_NONE;
            }
            if (llAction == LL_MERGE) {
                TList *m = mergeNodes(gPersonalities,gDates); dumpList(m);
                freeList(m); llAction=LL_NONE;
            }
            if (llAction == LL_Q_NAMES) {
                TQueue *q = sName(gPersonalities); dumpQueue(q);
                freeQueue(q); llAction=LL_NONE;
            }
            if (llAction == LL_Q_AGE) {
                TQueue *q = ageP(gPersonalities); dumpQueue(q);
                freeQueue(q); llAction=LL_NONE;
            }
        }
    }

    /* content area */
    int cx = sx + sw2 + PAD*2;
    int cy = 60;
    int cw = SCREEN_W - cx - PAD;

    /* draw input forms */
    if (llAction == LL_SEARCH_DOB || llAction == LL_SEARCH_DOD) {
        const char *hint = (llAction==LL_SEARCH_DOB) ? "Birth date (e.g. 1923)" : "Death date (e.g. 04/03/1957)";
        drawTitle(cx, cy, (llAction==LL_SEARCH_DOB)?"Search by Birth Date":"Search by Death Date");
        drawLabel(cx, cy+36, hint);
        drawInput(&llIn1, cx, cy+56, 340, 34, hint);
        if (drawButton(cx, cy+100, 120, 34, "Search", COL_BTN)) {
            TList *cur;
            int found;
            resClear();
            cur = gPersonalities;
            found = 0;
            while (cur) {
                const char *d = (llAction==LL_SEARCH_DOB) ? cur->dob : cur->dod;
                if (strcmp(d, llIn1.buf)==0) {
                    resBuf("[Found]  %s", cur->name);
                    resBuf("  Born : %s", cur->dob);
                    resBuf("  Died : %s", cur->dod);
                    resBuf("  Info : %s", cur->definition);
                    resBuf(" ");
                    found++;
                }
                cur = cur->next;
            }
            if (!found) resBuf("No entry found for date: %s", llIn1.buf);
        }
    }

    if (llAction == LL_SIMILAR) {
        drawTitle(cx, cy, "Find Similar by Year");
        drawLabel(cx, cy+36, "Enter a year:");
        drawInput(&llIn1, cx, cy+56, 200, 34, "e.g. 1957");
        if (drawButton(cx, cy+100, 120, 34, "Search", COL_BTN)) {
            TList *s;
            resClear();
            s = similarPersonality(gPersonalities, llIn1.buf);
            dumpList(s);
            freeList(s);
        }
    }

    if (llAction == LL_ADD_PERS) {
        drawTitle(cx, cy, "Add Personality");
        drawLabel(cx, cy+36, "Full name:");
        drawInput(&llIn1, cx, cy+56,  cw, 34, "e.g. Amirouche Ait Hamouda");
        drawLabel(cx, cy+100, "Date of birth:");
        drawInput(&llIn2, cx, cy+120, cw, 34, "e.g. 1926 or 01/01/1926");
        drawLabel(cx, cy+164, "Date of death:");
        drawInput(&llIn3, cx, cy+184, cw, 34, "e.g. 29/03/1959");
        drawLabel(cx, cy+228, "Definition / description:");
        drawInput(&llIn4, cx, cy+248, cw, 34, "Algerian military commander...");
        if (drawButton(cx, cy+296, 120, 34, "Add", COL_GREEN)) {
            if (llIn1.len > 0) {
                gPersonalities = addPersonality(gPersonalities, gDates, llIn1.buf,
                    llIn2.buf, llIn3.buf, llIn4.buf, DATA_FILE);
                resClear();
                resBuf("Added personality: %s", llIn1.buf);
                initInput(&llIn1); initInput(&llIn2);
                initInput(&llIn3); initInput(&llIn4);
            } else { resClear(); resBuf("Please enter a name."); }
        }
    }

    if (llAction == LL_ADD_EVT) {
        drawTitle(cx, cy, "Add Event");
        drawLabel(cx, cy+36, "Event name:");
        drawInput(&llIn1, cx, cy+56,  cw, 34, "e.g. Battle of Issafen");
        drawLabel(cx, cy+100, "Date:");
        drawInput(&llIn2, cx, cy+120, cw, 34, "e.g. 22/04/1958");
        drawLabel(cx, cy+164, "Description:");
        drawInput(&llIn3, cx, cy+184, cw, 34, "Description...");
        if (drawButton(cx, cy+232, 120, 34, "Add", COL_GREEN)) {
            if (llIn1.len > 0) {
                gEvents = addEvents(gEvents, llIn1.buf, llIn2.buf, llIn3.buf, DATA_FILE);
                resClear(); resBuf("Added event: %s", llIn1.buf);
                initInput(&llIn1); initInput(&llIn2); initInput(&llIn3);
            } else { resClear(); resBuf("Please enter an event name."); }
        }
    }

    if (llAction == LL_DELETE) {
        drawTitle(cx, cy, "Delete Personality");
        drawLabel(cx, cy+36, "Name to delete:");
        drawInput(&llIn1, cx, cy+56, cw, 34, "Exact name...");
        if (drawButton(cx, cy+100, 140, 34, "Delete", COL_RED)) {
            if (llIn1.len > 0) {
                gPersonalities = deletePersonality(DATA_FILE, gPersonalities, gDates, llIn1.buf);
                resClear(); resBuf("Deleted (if found): %s", llIn1.buf);
                initInput(&llIn1);
            }
        }
    }

    if (llAction == LL_UPDATE) {
        drawTitle(cx, cy, "Update Personality");
        drawLabel(cx, cy+36, "Name (exact):");
        drawInput(&llIn1, cx, cy+56,  cw, 34, "Name...");
        drawLabel(cx, cy+100, "New definition (leave blank to keep):");
        drawInput(&llIn2, cx, cy+120, cw, 34, "New info...");
        drawLabel(cx, cy+164, "New birth date (leave blank to keep):");
        drawInput(&llIn3, cx, cy+184, cw, 34, "New DoB...");
        drawLabel(cx, cy+228, "New death date (leave blank to keep):");
        drawInput(&llIn4, cx, cy+248, cw, 34, "New DoD...");
        if (drawButton(cx, cy+296, 120, 34, "Update", COL_GOLD)) {
            if (llIn1.len > 0) {
                gPersonalities = updatePersonality(DATA_FILE, gPersonalities, gDates,
                    llIn1.buf, llIn2.buf, llIn3.buf, llIn4.buf);
                resClear(); resBuf("Updated: %s", llIn1.buf);
            }
        }
    }

    /* result panel */
    drawResultPanel(cx, cy + 360, cw, SCREEN_H - cy - 360 - PAD);
}

/* ================================================================
   STACK SCREEN
   ================================================================ */
typedef enum {
    ST_NONE=0,
    ST_SHOW, ST_SEARCH, ST_SORT, ST_DELETE,
    ST_UPDATE, ST_ADD, ST_TO_QUEUE, ST_TO_LIST,
    ST_DEF_SORT, ST_PRONUN, ST_SMALLEST,
    ST_OVERLAP, ST_KILLED, ST_REVERSE
} STAction;

STAction stAction = ST_NONE;
InputBox stIn1, stIn2, stIn3, stIn4;

void drawStack() {
    int sx = PAD;
    int sy = 60;
    int sw2 = 200;
    int sh = 32;
    int gap = 6;
    int n = 14;
    int i;

    struct { const char *label; STAction act; } items[] = {
        {"Show Stack",          ST_SHOW},
        {"Search by Name",      ST_SEARCH},
        {"Sort Alphabetically", ST_SORT},
        {"Delete Entry",        ST_DELETE},
        {"Update Entry",        ST_UPDATE},
        {"Add Entry",           ST_ADD},
        {"Convert to Queue",    ST_TO_QUEUE},
        {"Convert to List",     ST_TO_LIST},
        {"Sort by Def Length",  ST_DEF_SORT},
        {"Split Short/Long",    ST_PRONUN},
        {"Smallest Definition", ST_SMALLEST},
        {"Overlapping Events",  ST_OVERLAP},
        {"Check if Killed",     ST_KILLED},
        {"Reverse (recursive)", ST_REVERSE},
    };
    for (i = 0; i < n; i++) {
        Color col = (stAction == items[i].act) ? COL_BTN_ACT : COL_BTN;
        if (drawButton(sx, sy + i*(sh+gap), sw2, sh, items[i].label, col)) {
            stAction = items[i].act;
            resClear();
            initInput(&stIn1); initInput(&stIn2);
            initInput(&stIn3); initInput(&stIn4);

            if (stAction==ST_SHOW)      { dumpStack(gStack); stAction=ST_NONE; }
            if (stAction==ST_SORT)      { gStack=sortNameStack(gStack); dumpStack(gStack); stAction=ST_NONE; }
            if (stAction==ST_TO_QUEUE)  { TQueue *q=stackToQueue(gStack); dumpQueue(q); freeQueue(q); stAction=ST_NONE; }
            if (stAction==ST_TO_LIST)   { TList *l=stackToList(gStack); dumpList(l); freeList(l); stAction=ST_NONE; }
            if (stAction==ST_DEF_SORT)  { gStack=definitionStack(gStack); dumpStack(gStack); stAction=ST_NONE; }
            if (stAction==ST_PRONUN)    { TStack *s2=pronunciationStack(gStack); dumpStack(s2); stAction=ST_NONE; }
            if (stAction==ST_SMALLEST)  { char *s=getSmallest(gStack); if(s) resBuf("Smallest: %s",s); stAction=ST_NONE; }
            if (stAction==ST_OVERLAP)   { continuousSearch(gStack); stAction=ST_NONE; }
            if (stAction==ST_REVERSE)   { gStack=recRevStack(gStack); dumpStack(gStack); stAction=ST_NONE; }
        }
    }

    int cx = sx + sw2 + PAD*2;
    int cy = 60;
    int cw = SCREEN_W - cx - PAD;

    if (stAction==ST_SEARCH) {
        drawTitle(cx, cy, "Search in Stack");
        drawLabel(cx, cy+36, "Name:");
        drawInput(&stIn1, cx, cy+56, cw, 34, "Exact name...");
        if (drawButton(cx, cy+100, 120, 34, "Search", COL_BTN)) {
            TStack *node;
            resClear();
            node = getInfoPersonality(gStack, stIn1.buf);
            if (node) { resBuf("Name: %s", node->name); resBuf("Born: %s", node->dob); resBuf("Died: %s", node->dod); resBuf("Info: %s", node->definition); }
        }
    }
    if (stAction==ST_DELETE) {
        drawTitle(cx, cy, "Delete from Stack");
        drawInput(&stIn1, cx, cy+56, cw, 34, "Exact name...");
        if (drawButton(cx, cy+100, 120, 34, "Delete", COL_RED)) {
            resClear(); gStack=deleteName(gStack, stIn1.buf);
            resBuf("Done."); initInput(&stIn1);
        }
    }
    if (stAction==ST_ADD) {
        drawTitle(cx, cy, "Add to Stack");
        drawLabel(cx, cy+36, "Name:"); drawInput(&stIn1, cx, cy+56, cw, 34, "Name...");
        drawLabel(cx, cy+100, "Definition:"); drawInput(&stIn2, cx, cy+120, cw, 34, "Info...");
        drawLabel(cx, cy+164, "Birth date:"); drawInput(&stIn3, cx, cy+184, cw, 34, "DoB...");
        drawLabel(cx, cy+228, "Death date:"); drawInput(&stIn4, cx, cy+248, cw, 34, "DoD...");
        if (drawButton(cx, cy+296, 120, 34, "Add", COL_GREEN)) {
            resClear(); gStack=addNameStack(gStack, stIn1.buf, stIn2.buf, stIn3.buf, stIn4.buf);
            resBuf("Added: %s", stIn1.buf);
        }
    }
    if (stAction==ST_UPDATE) {
        drawTitle(cx, cy, "Update Stack Entry");
        drawLabel(cx, cy+36, "Name (exact):"); drawInput(&stIn1, cx, cy+56, cw, 34, "Name...");
        drawLabel(cx, cy+100, "New Definition:"); drawInput(&stIn2, cx, cy+120, cw, 34, "");
        drawLabel(cx, cy+164, "New DoB:"); drawInput(&stIn3, cx, cy+184, cw, 34, "");
        drawLabel(cx, cy+228, "New DoD:"); drawInput(&stIn4, cx, cy+248, cw, 34, "");
        if (drawButton(cx, cy+296, 120, 34, "Update", COL_GOLD)) {
            resClear(); gStack=updateStack(gStack, stIn1.buf, stIn2.buf, stIn3.buf, stIn4.buf);
            resBuf("Updated.");
        }
    }
    if (stAction==ST_KILLED) {
        drawTitle(cx, cy, "Check if Personality was Killed");
        drawLabel(cx, cy+36, "Paste definition text:");
        drawInput(&stIn1, cx, cy+56, cw, 34, "Text...");
        if (drawButton(cx, cy+100, 120, 34, "Check", COL_BTN)) {
            resClear();
            if (isPersonalityKilled(stIn1.buf)) resBuf("Result: YES â€” killed/executed/assassinated.");
            else                                resBuf("Result: NO â€” not mentioned as killed.");
        }
    }

    drawResultPanel(cx, cy+360, cw, SCREEN_H-cy-360-PAD);
}

/* ================================================================
   BST SCREEN
   ================================================================ */
typedef enum {
    BST_NONE=0,
    BST_SEARCH, BST_ADD, BST_DELETE, BST_UPDATE,
    BST_INORDER, BST_PREORDER, BST_POSTORDER,
    BST_HEIGHT, BST_LCA, BST_RANGE, BST_SUCC,
    BST_MIRROR, BST_BALANCED
} BSTAction;

BSTAction bstAction = BST_NONE;
InputBox bstIn1, bstIn2, bstIn3, bstIn4;

void drawBST() {
    int sx = PAD;
    int sy = 60;
    int sw2 = 200;
    int sh = 32;
    int gap = 6;
    int n = 13;
    int i;

    struct { const char *label; BSTAction act; } items[] = {
        {"Search by Name",    BST_SEARCH},
        {"Add Personality",   BST_ADD},
        {"Delete",            BST_DELETE},
        {"Update",            BST_UPDATE},
        {"In-Order",          BST_INORDER},
        {"Pre-Order",         BST_PREORDER},
        {"Post-Order",        BST_POSTORDER},
        {"Height & Size",     BST_HEIGHT},
        {"Lowest Ancestor",   BST_LCA},
        {"Count Year Range",  BST_RANGE},
        {"In-Order Successor",BST_SUCC},
        {"Mirror Tree",       BST_MIRROR},
        {"Is Balanced?",      BST_BALANCED},
    };
    for (i = 0; i < n; i++) {
        Color col = (bstAction==items[i].act) ? COL_BTN_ACT : COL_BTN;
        if (drawButton(sx, sy + i*(sh+gap), sw2, sh, items[i].label, col)) {
            bstAction = items[i].act;
            resClear();
            initInput(&bstIn1); initInput(&bstIn2);
            initInput(&bstIn3); initInput(&bstIn4);
            if (bstAction==BST_INORDER)   { dumpTree(gTree); bstAction=BST_NONE; }
            if (bstAction==BST_PREORDER)  {
                /* preorder via recursive buffer fill */
                void fillPre(TTree *t);
                /* inline helper not supported in C89 â€” use lambda workaround: just call traversal and buffer */
                bstAction=BST_NONE;
                resBuf("(Pre-order â€” check terminal for now)");
            }
            if (bstAction==BST_POSTORDER) { bstAction=BST_NONE; resBuf("(Post-order â€” check terminal)"); }
            if (bstAction==BST_HEIGHT) {
                resBuf("Height : %d", treeHeight(gTree));
                resBuf("Size   : %d", treeSize(gTree));
                bstAction=BST_NONE;
            }
            if (bstAction==BST_MIRROR) { gTree=BSTMirror(gTree); resBuf("Tree mirrored."); bstAction=BST_NONE; }
            if (bstAction==BST_BALANCED) {
                resBuf(isBalancedBST(gTree) ? "Tree IS balanced." : "Tree is NOT balanced.");
                bstAction=BST_NONE;
            }
        }
    }

    int cx = sx + sw2 + PAD*2;
    int cy = 60;
    int cw = SCREEN_W - cx - PAD;

    if (bstAction==BST_SEARCH) {
        drawTitle(cx, cy, "Search BST");
        drawInput(&bstIn1, cx, cy+56, cw, 34, "Name...");
        if (drawButton(cx, cy+100, 120, 34, "Search", COL_BTN)) {
            TTree *node;
            resClear();
            node = getInfoNameTree(gTree, bstIn1.buf);
            if (node) { resBuf("Name: %s",node->name); resBuf("Born: %s",node->dob); resBuf("Died: %s",node->dod); resBuf("Info: %s",node->definition); }
        }
    }
    if (bstAction==BST_ADD) {
        drawTitle(cx, cy, "Add to BST");
        drawLabel(cx,cy+36,"Name:"); drawInput(&bstIn1,cx,cy+56,cw,34,"Name...");
        drawLabel(cx,cy+100,"Definition:"); drawInput(&bstIn2,cx,cy+120,cw,34,"Info...");
        drawLabel(cx,cy+164,"Birth date:"); drawInput(&bstIn3,cx,cy+184,cw,34,"DoB...");
        drawLabel(cx,cy+228,"Death date:"); drawInput(&bstIn4,cx,cy+248,cw,34,"DoD...");
        if (drawButton(cx,cy+296,120,34,"Add",COL_GREEN)) {
            resClear(); gTree=addNameBST(gTree,bstIn1.buf,bstIn2.buf,bstIn3.buf,bstIn4.buf);
            resBuf("Added: %s",bstIn1.buf);
        }
    }
    if (bstAction==BST_DELETE) {
        drawTitle(cx, cy, "Delete from BST");
        drawInput(&bstIn1,cx,cy+56,cw,34,"Name...");
        if (drawButton(cx,cy+100,120,34,"Delete",COL_RED)) {
            resClear(); gTree=deleteNameBST(gTree,bstIn1.buf);
            resBuf("Deleted (if found): %s",bstIn1.buf);
        }
    }
    if (bstAction==BST_UPDATE) {
        drawTitle(cx,cy,"Update BST Entry");
        drawLabel(cx,cy+36,"Name:"); drawInput(&bstIn1,cx,cy+56,cw,34,"Name...");
        drawLabel(cx,cy+100,"New def:"); drawInput(&bstIn2,cx,cy+120,cw,34,"");
        drawLabel(cx,cy+164,"New DoB:"); drawInput(&bstIn3,cx,cy+184,cw,34,"");
        drawLabel(cx,cy+228,"New DoD:"); drawInput(&bstIn4,cx,cy+248,cw,34,"");
        if (drawButton(cx,cy+296,120,34,"Update",COL_GOLD)) {
            resClear(); gTree=updateNameBST(gTree,bstIn1.buf,bstIn2.buf,bstIn3.buf,bstIn4.buf);
            resBuf("Updated.");
        }
    }
    if (bstAction==BST_LCA) {
        drawTitle(cx,cy,"Lowest Common Ancestor");
        drawLabel(cx,cy+36,"Name 1:"); drawInput(&bstIn1,cx,cy+56,cw,34,"Name...");
        drawLabel(cx,cy+100,"Name 2:"); drawInput(&bstIn2,cx,cy+120,cw,34,"Name...");
        if (drawButton(cx,cy+164,120,34,"Find",COL_BTN)) {
            TTree *lca;
            resClear();
            lca = lowestCommonAncestor(gTree,bstIn1.buf,bstIn2.buf);
            if(lca) resBuf("LCA: %s",lca->name); else resBuf("Not found.");
        }
    }
    if (bstAction==BST_RANGE) {
        drawTitle(cx,cy,"Count Nodes in Year Range");
        drawLabel(cx,cy+36,"From year:"); drawInput(&bstIn1,cx,cy+56,200,34,"e.g. 1900");
        drawLabel(cx,cy+100,"To year:"); drawInput(&bstIn2,cx,cy+120,200,34,"e.g. 1960");
        if (drawButton(cx,cy+164,140,34,"Count",COL_BTN)) {
            int c;
            resClear();
            c = countNodesRange(gTree,atoi(bstIn1.buf),atoi(bstIn2.buf));
            resBuf("Personalities born between %s and %s: %d",bstIn1.buf,bstIn2.buf,c);
        }
    }
    if (bstAction==BST_SUCC) {
        drawTitle(cx,cy,"In-Order Successor");
        drawInput(&bstIn1,cx,cy+56,cw,34,"Name...");
        if (drawButton(cx,cy+100,120,34,"Find",COL_BTN)) {
            TTree *s;
            resClear();
            s = inOrderSuccessor(gTree,bstIn1.buf);
            if(s) resBuf("Successor of %s: %s",bstIn1.buf,s->name);
            else  resBuf("No successor found.");
        }
    }

    drawResultPanel(cx, cy+360, cw, SCREEN_H-cy-360-PAD);
}

/* ================================================================
   RECURSION SCREEN
   ================================================================ */
typedef enum {
    RC_NONE=0,
    RC_COUNT, RC_REMOVE, RC_REPLACE,
    RC_PERM, RC_SUBSEQ, RC_OVERLAP,
    RC_DISTINCT, RC_PALINDROME
} RCAction;

RCAction rcAction = RC_NONE;
InputBox rcIn1, rcIn2, rcIn3;

void drawRecursion() {
    int sx = PAD;
    int sy = 60;
    int sw2 = 210;
    int sh = 32;
    int gap = 6;
    int i;

    struct { const char *label; RCAction act; } items[] = {
        {"Count Occurrences",  RC_COUNT},
        {"Remove Lines",       RC_REMOVE},
        {"Replace Dates",      RC_REPLACE},
        {"All Permutations",   RC_PERM},
        {"All Subsequences",   RC_SUBSEQ},
        {"Events in Range",    RC_OVERLAP},
        {"Distinct Subseq.",   RC_DISTINCT},
        {"Palindrome Check",   RC_PALINDROME},
    };
    for (i = 0; i < 8; i++) {
        Color col = (rcAction==items[i].act) ? COL_BTN_ACT : COL_BTN;
        if (drawButton(sx, sy + i*(sh+gap), sw2, sh, items[i].label, col)) {
            rcAction = items[i].act;
            resClear();
            initInput(&rcIn1); initInput(&rcIn2); initInput(&rcIn3);
        }
    }

    int cx = sx + sw2 + PAD*2;
    int cy = 60;
    int cw = SCREEN_W - cx - PAD;

    if (rcAction==RC_COUNT) {
        drawTitle(cx,cy,"Count Occurrences in File");
        drawLabel(cx,cy+36,"Word or name to count:");
        drawInput(&rcIn1,cx,cy+56,cw,34,"e.g. FLN");
        if (drawButton(cx,cy+100,120,34,"Count",COL_BTN)) {
            resClear(); rewind(gFile);
            int c=countOccurrence(gFile,rcIn1.buf);
            resBuf("'%s' appears %d time(s) in the file.",rcIn1.buf,c);
        }
    }
    if (rcAction==RC_REMOVE) {
        drawTitle(cx,cy,"Remove Lines Containing Word");
        drawLabel(cx,cy+36,"WARNING: this deletes lines from history.txt");
        drawInput(&rcIn1,cx,cy+70,cw,34,"Word...");
        if (drawButton(cx,cy+114,140,34,"Remove",COL_RED)) {
            resClear(); removeOccurrence(DATA_FILE,rcIn1.buf);
            reloadData(); resBuf("Removed lines containing '%s'.",rcIn1.buf);
        }
    }
    if (rcAction==RC_REPLACE) {
        drawTitle(cx,cy,"Replace Dates for a Name");
        drawLabel(cx,cy+36,"Name:"); drawInput(&rcIn1,cx,cy+56,cw,34,"Name...");
        drawLabel(cx,cy+100,"New birth date:"); drawInput(&rcIn2,cx,cy+120,cw,34,"DoB...");
        drawLabel(cx,cy+164,"New death date:"); drawInput(&rcIn3,cx,cy+184,cw,34,"DoD...");
        if (drawButton(cx,cy+230,120,34,"Replace",COL_GOLD)) {
            resClear(); replaceOccurrence(DATA_FILE,rcIn1.buf,rcIn2.buf,rcIn3.buf);
            reloadData(); resBuf("Dates replaced for '%s'.",rcIn1.buf);
        }
    }
    if (rcAction==RC_PERM) {
        drawTitle(cx,cy,"All Permutations");
        drawLabel(cx,cy+36,"Short word (max ~6 chars recommended):");
        drawInput(&rcIn1,cx,cy+56,cw,34,"e.g. Ali");
        if (drawButton(cx,cy+100,120,34,"Generate",COL_BTN)) {
            char tmp[MAX_NAME];
            resClear();
            strncpy(tmp,rcIn1.buf,MAX_NAME-1);
            tmp[MAX_NAME-1] = '\0';
            namePermutation(tmp);
            /* namePermutation uses printf, so capture manually */
            resClear();
            /* rebuild here inline since namePermutation uses printf */
            void permBuf(char *s, int l, int r);
            permBuf(tmp,0,(int)strlen(tmp)-1);
        }
    }
    if (rcAction==RC_SUBSEQ) {
        drawTitle(cx,cy,"All Subsequences");
        drawLabel(cx,cy+36,"Short word:");
        drawInput(&rcIn1,cx,cy+56,cw,34,"e.g. abc");
        if (drawButton(cx,cy+100,120,34,"Generate",COL_BTN)) {
            char tmp[MAX_NAME];
            char buf2[MAX_NAME];
            resClear();
            strncpy(tmp,rcIn1.buf,MAX_NAME-1);
            tmp[MAX_NAME-1] = '\0';
            /* subseqName uses printf â€” inline version */
            void subseqBuf(char *w, char *b, int wi, int bi);
            buf2[0] = '\0';
            subseqBuf(tmp,buf2,0,0);
        }
    }
    if (rcAction==RC_OVERLAP) {
        drawTitle(cx,cy,"Events Overlapping Year Range");
        drawLabel(cx,cy+36,"Start year:"); drawInput(&rcIn1,cx,cy+56,200,34,"e.g. 1954");
        drawLabel(cx,cy+100,"End year:"); drawInput(&rcIn2,cx,cy+120,200,34,"e.g. 1962");
        if (drawButton(cx,cy+164,120,34,"Search",COL_BTN)) {
            int y1;
            int y2;
            int lo;
            int hi;
            TList *e;
            int found;
            resClear();
            y1 = atoi(rcIn1.buf);
            y2 = atoi(rcIn2.buf);
            lo = (y1 < y2) ? y1 : y2;
            hi = (y1 > y2) ? y1 : y2;
            e = gEvents;
            found = 0;
            while (e) {
                int ey = dateToYear(e->dob);
                if (ey >= lo && ey <= hi) {
                    resBuf("[Event]  %s  (%s)", e->name, e->dob);
                    found++;
                }
                e = e->next;
            }
            if (!found) resBuf("No events found in range %d-%d.", lo, hi);
        }
    }
    if (rcAction==RC_DISTINCT) {
        drawTitle(cx,cy,"Count Distinct Subsequences");
        drawInput(&rcIn1,cx,cy+56,cw,34,"Word...");
        if (drawButton(cx,cy+100,120,34,"Count",COL_BTN)) {
            int d;
            resClear();
            d = distinctSubseqWord(rcIn1.buf);
            resBuf("Distinct subsequences of '%s': %d",rcIn1.buf,d);
        }
    }
    if (rcAction==RC_PALINDROME) {
        drawTitle(cx,cy,"Palindrome Check");
        drawInput(&rcIn1,cx,cy+56,cw,34,"Word...");
        if (drawButton(cx,cy+100,120,34,"Check",COL_BTN)) {
            resClear();
            if (isPalindromeWord(rcIn1.buf)) resBuf("'%s' IS a palindrome.",rcIn1.buf);
            else                             resBuf("'%s' is NOT a palindrome.",rcIn1.buf);
        }
    }

    drawResultPanel(cx, cy+360, cw, SCREEN_H-cy-360-PAD);
}

/* Buffer-filling permutation (for GUI, avoids printf) */
void permBuf(char *s, int l, int r) {
    int i; char tmp;
    if (l==r) { resBuf("  %s",s); return; }
    for(i=l;i<=r;i++){
        tmp=s[l];s[l]=s[i];s[i]=tmp;
        permBuf(s,l+1,r);
        tmp=s[l];s[l]=s[i];s[i]=tmp;
    }
}

void subseqBuf(char *word, char *buf, int wi, int bi) {
    int i; buf[bi]='\0';
    if(bi>0) resBuf("  %s",buf);
    for(i=wi;word[i]!='\0';i++){
        buf[bi]=word[i];
        subseqBuf(word,buf,i+1,bi+1);
    }
}

/* ================================================================
   TOP BAR
   ================================================================ */
void drawTopBar() {
    int i;
    DrawRectangle(0, 0, SCREEN_W, 50, COL_SIDEBAR);
    DrawLine(0, 50, SCREEN_W, 50, COL_BORDER);

    /* nav tabs */
    struct { const char *label; Screen sc; } tabs[] = {
        {"Home", SCR_HOME}, {"Linked List", SCR_LIST},
        {"Stack", SCR_STACK}, {"BST", SCR_BST}, {"Recursion", SCR_RECURSION}
    };
    int tx = PAD;
    for (i = 0; i < 5; i++) {
        int tw = MeasureText(tabs[i].label, FONT_SIZE) + PAD*2;
        Rectangle r = {(float)tx, 8, (float)tw, 34};
        bool active = (currentScreen == tabs[i].sc);
        DrawRectangleRounded(r, 0.25f, 4, active ? COL_BTN : COL_SIDEBAR);
        if (!active) DrawRectangleRoundedLines(r, 0.25f, 4, 1.5f, COL_BORDER);
        Color tc = active ? WHITE : COL_DIMTEXT;
        DrawText(tabs[i].label, tx+PAD, 16, FONT_SIZE, tc);
        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            currentScreen = tabs[i].sc;
        tx += tw + 6;
    }

    /* fps */
    char fps[32]; sprintf(fps, "FPS: %d", GetFPS());
    DrawText(fps, SCREEN_W - 80, 18, FONT_SIZE-4, COL_DIMTEXT);
}

/* ================================================================
   MAIN
   ================================================================ */
int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Algeria History Database â€” NSCS 2025/2026");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    reloadData();
    initInput(&llIn1); initInput(&llIn2); initInput(&llIn3); initInput(&llIn4);
    initInput(&stIn1); initInput(&stIn2); initInput(&stIn3); initInput(&stIn4);
    initInput(&bstIn1); initInput(&bstIn2); initInput(&bstIn3); initInput(&bstIn4);
    initInput(&rcIn1); initInput(&rcIn2); initInput(&rcIn3);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(COL_BG);

        drawTopBar();

        if      (currentScreen == SCR_HOME)      drawHome();
        else if (currentScreen == SCR_LIST)      drawLinkedList();
        else if (currentScreen == SCR_STACK)     drawStack();
        else if (currentScreen == SCR_BST)       drawBST();
        else if (currentScreen == SCR_RECURSION) drawRecursion();

        EndDrawing();
    }

    freeList(gPersonalities); freeList(gDates); freeList(gEvents);
    freeStack(gStack); freeTree(gTree);
    if (gFile) fclose(gFile);
    CloseWindow();
    return 0;
}