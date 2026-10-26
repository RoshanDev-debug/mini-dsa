#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 900
#define SIDEBAR_WIDTH 245
#define MAX_VALUES 24
#define MAX_FRAMES 160
#define MODULE_COUNT 7
#define PI 3.14159265359f

typedef enum { ARRAY_MODULE, LIST_MODULE, STACK_MODULE, QUEUE_MODULE, TREE_MODULE, SORT_MODULE, GRAPH_MODULE } Module;
typedef enum { ACTION_INSERT, ACTION_REMOVE, ACTION_FIND, ACTION_UPDATE } Action;

typedef struct {
    char title[96];
    char detail[180];
    int values[MAX_VALUES];
    int count;
    int activeIndex;
    int compareA;
    int compareB;
    int activeValue;
    int markedValues[MAX_VALUES];
    int markedCount;
    int graphVisited[8];
    int graphVisitedCount;
    int graphActive;
    int graphStart;
} Frame;

typedef struct {
    Module module;
    Action action;
    Frame frames[MAX_FRAMES];
    int frameCount;
    int currentFrame;
    bool playing;
    float speed;
    double nextTick;
    int values[MODULE_COUNT][MAX_VALUES];
    int counts[MODULE_COUNT];
    int operationValue;
    int operationIndex;
    int operationMode;
    bool dark;
} App;

static const Color BG = {245, 247, 251, 255};
static const Color SURFACE = {255, 255, 255, 255};
static const Color SOFT = {248, 250, 255, 255};
static const Color NAVY = {25, 38, 74, 255};
static const Color TEXT = {23, 35, 61, 255};
static const Color MUTED = {131, 144, 168, 255};
static const Color LIGHT_MUTED = {165, 175, 192, 255};
static const Color LINE = {231, 235, 243, 255};
static const Color PURPLE = {103, 86, 232, 255};
static const Color PURPLE_SOFT = {237, 234, 255, 255};
static const Color CORAL = {255, 131, 109, 255};
static const Color CORAL_SOFT = {255, 240, 237, 255};
static const Color MINT = {69, 198, 162, 255};
static const Color MINT_SOFT = {230, 248, 243, 255};
static const Color GOLD = {245, 184, 75, 255};

static const char *module_names[MODULE_COUNT] = {"Arrays", "Linked lists", "Stacks", "Queues", "Binary trees", "Sorting", "Graph traversal"};
static const char *module_short[MODULE_COUNT] = {"ARRAY", "LIST", "STACK", "QUEUE", "TREE", "SORT", "GRAPH"};
static const char *module_titles[MODULE_COUNT] = {"Arrays, made visual.", "Linked lists, connected.", "Stacks, one layer at a time.", "Queues, moving forward.", "Trees, branching with purpose.", "Sorting, frame by frame.", "Graphs, explored with intent."};
static const char *module_copies[MODULE_COUNT] = {
    "An array stores elements next to each other in memory, which makes indexed access delightfully fast.",
    "Each node stores a value and a pointer to the next node. The chain can grow without moving every neighbor.",
    "A stack only exposes its top. The last item pushed is the first one that can leave.",
    "A queue keeps the order of arrival intact: the first item in line gets served first.",
    "A binary search tree keeps smaller values left and larger values right, making each comparison useful.",
    "Sorting algorithms make small local choices that gradually transform an unsorted list into a predictable order.",
    "Traversal algorithms remember what they have seen so they can explore a network without getting lost in a loop."
};

static void copy_text(char *destination, int size, const char *source) {
    snprintf(destination, (size_t)size, "%s", source);
}

static void clear_frame(Frame *frame) {
    memset(frame, 0, sizeof(Frame));
    frame->activeIndex = -1;
    frame->compareA = -1;
    frame->compareB = -1;
    frame->activeValue = -1;
    frame->graphActive = -1;
    frame->graphStart = 0;
}

static void snapshot_values(App *app, Frame *frame) {
    int module = (int)app->module;
    frame->count = app->counts[module];
    for (int i = 0; i < frame->count; i++) frame->values[i] = app->values[module][i];
}

static Frame *new_frame(App *app, const char *title, const char *detail) {
    if (app->frameCount >= MAX_FRAMES) return &app->frames[app->frameCount - 1];
    Frame *frame = &app->frames[app->frameCount++];
    clear_frame(frame);
    snapshot_values(app, frame);
    copy_text(frame->title, sizeof(frame->title), title);
    copy_text(frame->detail, sizeof(frame->detail), detail);
    return frame;
}

static void reset_frames(App *app) {
    app->frameCount = 0;
    app->currentFrame = 0;
    app->playing = false;
    int module = (int)app->module;
    const char *title = "Ready to explore.";
    const char *detail = "Choose an operation below to create an animation.";
    if (app->module == GRAPH_MODULE) { title = "A network waiting to be explored."; detail = "Choose BFS or DFS to reveal the visit order."; }
    if (app->module == TREE_MODULE) { title = "A binary search tree waiting for a path."; detail = "Insert or search for a value to trace a branch."; }
    if (app->module == SORT_MODULE) { title = "An unsorted collection."; detail = "Pick an algorithm and start the comparison dance."; }
    Frame *frame = new_frame(app, title, detail);
    if (app->module == GRAPH_MODULE) { frame->graphStart = 0; }
    (void)module;
}

static void set_module_defaults(App *app) {
    memset(app->values, 0, sizeof(app->values));
    memset(app->counts, 0, sizeof(app->counts));
    int array[] = {10, 20, 30, 40, 50};
    int list[] = {12, 24, 38, 51};
    int stack[] = {1, 2, 3};
    int queue[] = {12, 24, 38, 51};
    int tree[] = {50, 28, 72, 16, 38, 61, 84};
    int sort[] = {62, 27, 81, 43, 19, 74, 36, 55};
    int *defaults[] = {array, list, stack, queue, tree, sort, NULL};
    int sizes[] = {5, 4, 3, 4, 7, 8, 0};
    for (int module = 0; module < MODULE_COUNT; module++) {
        app->counts[module] = sizes[module];
        for (int i = 0; i < sizes[module]; i++) app->values[module][i] = defaults[module][i];
    }
    app->operationValue = 25;
    app->operationIndex = 2;
    app->operationMode = 0;
    app->speed = 1.0f;
}

static void init_app(App *app) {
    memset(app, 0, sizeof(App));
    app->module = ARRAY_MODULE;
    app->action = ACTION_INSERT;
    set_module_defaults(app);
    reset_frames(app);
}

static void copy_current_values(App *app, int *destination, int *count) {
    int module = (int)app->module;
    *count = app->counts[module];
    for (int i = 0; i < *count; i++) destination[i] = app->values[module][i];
}

static void apply_values(App *app, const int *values, int count) {
    int module = (int)app->module;
    app->counts[module] = count;
    for (int i = 0; i < count; i++) app->values[module][i] = values[i];
}

static void mark_frame(Frame *frame, int index) {
    if (index >= 0 && index < MAX_VALUES) frame->markedValues[frame->markedCount++] = index;
}

static void array_operation(App *app) {
    int data[MAX_VALUES], count = 0;
    copy_current_values(app, data, &count);
    int index = app->operationIndex;
    if (index < 0) index = 0;
    if (index > count) index = count;
    int value = app->operationValue;
    if (app->action == ACTION_INSERT) {
        Frame *one = new_frame(app, TextFormat("Find index %d.", index), "This is where the new value belongs.");
        one->activeIndex = index < count ? index : count - 1;
        if (index < count) {
            Frame *two = new_frame(app, "Make room for the new value.", "Everything after the index shifts right.");
            two->activeIndex = index;
        }
        for (int i = count; i > index; i--) data[i] = data[i - 1];
        data[index] = value; count++;
        apply_values(app, data, count);
        Frame *three = new_frame(app, TextFormat("Insert %d at index %d.", value, index), "The gap is filled.");
        three->activeIndex = index;
        new_frame(app, "Operation complete.", TextFormat("The array now has %d elements.", count));
    } else if (app->action == ACTION_REMOVE) {
        if (count == 0) return;
        if (index >= count) index = count - 1;
        Frame *one = new_frame(app, TextFormat("Locate index %d.", index), TextFormat("Value %d is ready to leave.", data[index])); one->activeIndex = index;
        Frame *two = new_frame(app, "Remove the selected cell.", "The following elements will slide left."); two->compareA = index;
        int removed = data[index]; for (int i = index; i < count - 1; i++) data[i] = data[i + 1]; count--; apply_values(app, data, count);
        new_frame(app, TextFormat("Remove %d.", removed), "The remaining values close the gap.");
        new_frame(app, "Operation complete.", TextFormat("The array now has %d elements.", count));
    } else if (app->action == ACTION_FIND) {
        int found = -1;
        for (int i = 0; i < count; i++) { Frame *frame = new_frame(app, TextFormat("Compare with index %d.", i), TextFormat("Is %d equal to %d?", data[i], value)); frame->compareA = i; if (data[i] == value) { found = i; break; } }
        if (found >= 0) { Frame *frame = new_frame(app, TextFormat("Found %d at index %d.", value, found), "The search can stop here."); frame->activeIndex = found; }
        else new_frame(app, TextFormat("%d is not in the array.", value), "A linear search checked every cell.");
    } else {
        if (count == 0) return;
        if (index >= count) index = count - 1;
        Frame *one = new_frame(app, TextFormat("Select index %d.", index), TextFormat("Replace %d with %d.", data[index], value)); one->activeIndex = index;
        data[index] = value; apply_values(app, data, count);
        Frame *two = new_frame(app, TextFormat("Update index %d.", index), "The cell keeps its position but gets a new value."); two->activeIndex = index;
        new_frame(app, "Operation complete.", "The array shape stayed the same.");
    }
}
static void list_operation(App *app) {
    int data[MAX_VALUES], count = 0; copy_current_values(app, data, &count); int value = app->operationValue;
    if (app->operationMode == 0) {
        Frame *one = new_frame(app, "Walk to the tail.", "The last node still points to null."); if (count) one->activeIndex = count - 1;
        data[count++] = value; apply_values(app, data, count); Frame *two = new_frame(app, TextFormat("Append %d.", value), "Only the old tail's next pointer changes."); two->activeIndex = count - 1; new_frame(app, "Operation complete.", "The chain has one new node.");
    } else if (app->operationMode == 1) {
        Frame *one = new_frame(app, "Start at the head.", "A new node will become the first node."); if (count) one->activeIndex = 0;
        for (int i = count; i > 0; i--) data[i] = data[i - 1]; data[0] = value; count++; apply_values(app, data, count); Frame *two = new_frame(app, TextFormat("Prepend %d.", value), "The new node points to the old head."); two->activeIndex = 0; new_frame(app, "Operation complete.", "Head now points to the new node.");
    } else {
        int found = -1; for (int i = 0; i < count; i++) if (data[i] == value) { found = i; break; }
        Frame *one = new_frame(app, found >= 0 ? TextFormat("Find %d.", value) : TextFormat("%d was not found.", value), found >= 0 ? "Follow pointers until the value matches." : "Nothing to remove."); if (found >= 0) one->activeIndex = found;
        if (found >= 0) { for (int i = found; i < count - 1; i++) data[i] = data[i + 1]; count--; apply_values(app, data, count); new_frame(app, TextFormat("Delete %d.", value), "The previous node skips over it."); }
        new_frame(app, "Operation complete.", "The chain reconnects around the removed node.");
    }
}

static void stack_operation(App *app) {
    int data[MAX_VALUES], count = 0; copy_current_values(app, data, &count);
    if (app->operationMode == 0) { new_frame(app, "The top is open.", "A stack only changes at one end."); data[count++] = app->operationValue; apply_values(app, data, count); Frame *two = new_frame(app, TextFormat("Push %d.", app->operationValue), "The new item becomes the top."); two->activeIndex = count - 1; new_frame(app, "Operation complete.", "Last in, first out."); }
    else if (count > 0) { Frame *one = new_frame(app, TextFormat("Peek at %d.", data[count - 1]), "The top item is the only item visible."); one->activeIndex = count - 1; int removed = data[--count]; apply_values(app, data, count); new_frame(app, TextFormat("Pop %d.", removed), "The next item is now exposed."); new_frame(app, "Operation complete.", "The newest item left first."); }
    else new_frame(app, "The stack is empty.", "There is nothing to pop.");
}

static void queue_operation(App *app) {
    int data[MAX_VALUES], count = 0; copy_current_values(app, data, &count);
    if (app->operationMode == 0) { new_frame(app, "The rear is ready.", "New arrivals join at the back of the line."); data[count++] = app->operationValue; apply_values(app, data, count); Frame *two = new_frame(app, TextFormat("Enqueue %d.", app->operationValue), "The rear pointer advances."); two->activeIndex = count - 1; new_frame(app, "Operation complete.", "Everyone keeps their place."); }
    else if (count > 0) { Frame *one = new_frame(app, TextFormat("Serve %d first.", data[0]), "The front of the line is always next."); one->activeIndex = 0; int removed = data[0]; for (int i = 0; i < count - 1; i++) data[i] = data[i + 1]; count--; apply_values(app, data, count); new_frame(app, TextFormat("Dequeue %d.", removed), "The rest of the line moves forward."); new_frame(app, "Operation complete.", "First in, first out."); }
    else new_frame(app, "The queue is empty.", "There is nobody waiting.");
}

static void tree_operation(App *app) {
    int data[MAX_VALUES], count = 0; copy_current_values(app, data, &count); int value = app->operationValue;
    int position = 0; int found = -1;
    while (position < count) {
        Frame *frame = new_frame(app, TextFormat("Compare %d with %d.", value, data[position]), value < data[position] ? "Smaller values branch left." : "Larger values branch right.");
        frame->activeValue = data[position];
        if (value == data[position]) { found = position; break; }
        position = value < data[position] ? position * 2 + 1 : position * 2 + 2;
        if (position >= count) break;
    }
    if (app->operationMode == 0 && found < 0 && count < MAX_VALUES) {
        data[count++] = value; apply_values(app, data, count); Frame *frame = new_frame(app, TextFormat("Insert %d.", value), "An empty child slot becomes a new leaf."); frame->activeValue = value; new_frame(app, "Operation complete.", "The ordering rule still holds.");
    } else if (found >= 0) new_frame(app, TextFormat("Found %d.", value), "The branch ends here.");
    else new_frame(app, TextFormat("%d is not in the tree.", value), "The search reached a null child.");
}

static void sort_operation(App *app) {
    int data[MAX_VALUES], count = 0; copy_current_values(app, data, &count); bool ascending = app->operationMode != 2;
    for (int end = count - 1; end > 0; end--) {
        for (int j = 0; j < end; j++) {
            Frame *frame = new_frame(app, TextFormat("Compare %d and %d.", data[j], data[j + 1]), "Compare, then swap if they are out of order."); frame->compareA = j; frame->compareB = j + 1;
            bool out_of_order = ascending ? data[j] > data[j + 1] : data[j] < data[j + 1];
            if (out_of_order) { int temporary = data[j]; data[j] = data[j + 1]; data[j + 1] = temporary; apply_values(app, data, count); }
        }
        if (app->frameCount >= MAX_FRAMES - 4) break;
    }
    apply_values(app, data, count); Frame *done = new_frame(app, "Collection sorted.", ascending ? "Bubble sort finished in ascending order." : "Bubble sort finished in descending order."); for (int i = 0; i < count; i++) mark_frame(done, i);
}

static const int graph_edges[7][2] = {{0,1},{0,2},{1,3},{1,4},{2,5},{4,6},{5,6}};
static const int graph_adjacency[7][3] = {{1,2,-1},{0,3,4},{0,5,-1},{1,-1,-1},{1,6,-1},{2,6,-1},{4,5,-1}};
static const int graph_degree[7] = {2,3,2,1,2,2,2};

static void graph_operation(App *app) {
    int frontier[16], front = 0, back = 0, visited[8], visited_count = 0; bool seen[8] = {false};
    frontier[back++] = app->operationIndex;
    while (front < back && visited_count < 8) {
        int current = app->operationMode == 0 ? frontier[front++] : frontier[--back];
        if (seen[current]) continue;
        int previous = visited_count > 0 ? visited[visited_count - 1] : -1;
        seen[current] = true; visited[visited_count++] = current;
        Frame *frame = new_frame(app, TextFormat("Visit node %c.", 'A' + current), app->operationMode == 0 ? "BFS uses a queue." : "DFS uses a stack."); frame->graphActive = current; frame->graphStart = app->operationIndex; frame->graphVisitedCount = visited_count - 1; for (int i = 0; i < visited_count - 1; i++) frame->graphVisited[i] = visited[i];
        (void)previous;
        for (int i = graph_degree[current] - 1; i >= 0; i--) { int neighbor = graph_adjacency[current][i]; if (!seen[neighbor]) frontier[back++] = neighbor; }
    }
    Frame *done = new_frame(app, app->operationMode == 0 ? "BFS traversal complete." : "DFS traversal complete.", "Visit order is the algorithm's fingerprint."); done->graphStart = app->operationIndex; done->graphVisitedCount = visited_count; for (int i = 0; i < visited_count; i++) done->graphVisited[i] = visited[i];
}

static void run_operation(App *app) {
    app->frameCount = 0; app->currentFrame = 0; app->playing = false;
    if (app->module == ARRAY_MODULE) array_operation(app);
    else if (app->module == LIST_MODULE) list_operation(app);
    else if (app->module == STACK_MODULE) stack_operation(app);
    else if (app->module == QUEUE_MODULE) queue_operation(app);
    else if (app->module == TREE_MODULE) tree_operation(app);
    else if (app->module == SORT_MODULE) sort_operation(app);
    else if (app->module == GRAPH_MODULE) graph_operation(app);
    if (app->frameCount == 0) reset_frames(app);
}

static void go_to_module(App *app, Module module) {
    app->module = module; app->operationMode = 0; app->action = ACTION_INSERT; app->operationIndex = 2; app->operationValue = module == STACK_MODULE ? 4 : 25; reset_frames(app);
}

static void advance_frame(App *app, int direction) {
    app->playing = false;
    app->currentFrame += direction;
    if (app->currentFrame < 0) app->currentFrame = 0;
    if (app->currentFrame >= app->frameCount) app->currentFrame = app->frameCount - 1;
}
static void draw_rounded(Rectangle rectangle, Color color) { DrawRectangleRounded(rectangle, 0.18f, 8, color); }
static bool clicked(Rectangle rectangle) { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rectangle); }
static void draw_wrapped(const char *text, int x, int y, int width, int font_size, Color color) {
    char line[128] = {0}; int line_length = 0, line_y = y;
    for (size_t i = 0; i <= strlen(text); i++) {
        char character = text[i];
        bool break_line = character == '\0' || line_length >= width / (font_size / 2 + 1);
        if (break_line) { line[line_length] = '\0'; DrawText(line, x, line_y, font_size, color); line_y += font_size + 5; line_length = 0; if (character == '\0') break; }
        if (character != ' ' || line_length > 0) line[line_length++] = character;
    }
}
static void draw_label(const char *text, int x, int y, int size, Color color) { DrawText(text, x, y, size, color); }
static void draw_button(Rectangle rectangle, const char *text, bool active, Color accent) {
    Color fill = active ? accent : SOFT; Color text_color = active ? WHITE : MUTED;
    draw_rounded(rectangle, fill); int text_width = MeasureText(text, 11); DrawText(text, (int)(rectangle.x + (rectangle.width - text_width) / 2), (int)(rectangle.y + 10), 11, text_color);
}
static void draw_section_title(const char *overline, const char *title, int x, int y) { DrawText(overline, x, y, 10, LIGHT_MUTED); DrawText(title, x, y + 20, 17, TEXT); }
static void draw_header(App *app) {
    DrawRectangle(SIDEBAR_WIDTH, 0, SCREEN_WIDTH - SIDEBAR_WIDTH, 76, SURFACE);
    DrawLine(SIDEBAR_WIDTH, 75, SCREEN_WIDTH, 75, LINE);
    DrawText("Visualizer", SIDEBAR_WIDTH + 46, 30, 12, MUTED); DrawText("/", SIDEBAR_WIDTH + 115, 30, 12, LIGHT_MUTED); DrawText(module_names[app->module], SIDEBAR_WIDTH + 137, 30, 12, TEXT);
    draw_button((Rectangle){SCREEN_WIDTH - 208, 22, 31, 31}, app->dark ? "☾" : "☼", false, PURPLE);
    DrawText("Quick guide", SCREEN_WIDTH - 157, 31, 11, MUTED); DrawCircle(SCREEN_WIDTH - 49, 37, 15, CORAL); DrawText("AR", SCREEN_WIDTH - 58, 32, 9, WHITE);
}
static void draw_sidebar(App *app) {
    DrawRectangle(0, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, NAVY);
    DrawRectangle(25, 31, 7, 15, CORAL); DrawRectangle(35, 22, 7, 24, (Color){143,131,255,255}); DrawRectangle(45, 27, 7, 19, MINT);
    DrawText("AlgoFlow", 66, 23, 20, WHITE); DrawText("DSA PLAYGROUND", 67, 48, 9, (Color){156,169,199,255});
    DrawText("EXPLORE MODULES", 28, 100, 10, (Color){135,150,183,255});
    for (int i = 0; i < MODULE_COUNT; i++) {
        Rectangle row = {18, 124 + i * 45, SIDEBAR_WIDTH - 36, 37}; bool active = app->module == (Module)i;
        if (active) draw_rounded(row, (Color){66, 65, 139, 255});
        DrawText(module_short[i], 35, (int)row.y + 12, 9, active ? WHITE : (Color){166,178,207,255}); DrawText(module_names[i], 93, (int)row.y + 10, 12, active ? WHITE : (Color){166,178,207,255});
        if (active) DrawCircle((int)row.x + 12, (int)row.y + row.height / 2, 3, MINT);
    }
    draw_rounded((Rectangle){23, SCREEN_HEIGHT - 112, SIDEBAR_WIDTH - 46, 69}, (Color){38, 52, 91, 255}); DrawText("✦", 36, SCREEN_HEIGHT - 93, 16, GOLD); DrawText("LEARNING MODE", 64, SCREEN_HEIGHT - 95, 9, WHITE); DrawText("Slow down and inspect", 64, SCREEN_HEIGHT - 78, 9, (Color){137,151,181,255}); DrawText("every move.", 64, SCREEN_HEIGHT - 64, 9, (Color){137,151,181,255}); DrawText("v1.0   ·   BUILT FOR CURIOUS MINDS", 24, SCREEN_HEIGHT - 24, 8, (Color){102,118,153,255});
}
static void draw_array_stage(const Frame *frame, Rectangle stage) {
    int gap = 9, cell_width = 66, total = frame->count * cell_width + (frame->count - 1) * gap, start_x = (int)(stage.x + (stage.width - total) / 2), y = (int)(stage.y + 88);
    for (int i = 0; i < frame->count; i++) { Rectangle cell = {(float)(start_x + i * (cell_width + gap)), (float)y, (float)cell_width, 66}; bool active = frame->activeIndex == i, compared = frame->compareA == i || frame->compareB == i; Color fill = active ? PURPLE_SOFT : compared ? CORAL_SOFT : SURFACE; Color border = active ? PURPLE : compared ? CORAL : LINE; draw_rounded(cell, fill); DrawRectangleLinesEx(cell, 1, border); char buffer[16]; snprintf(buffer, sizeof(buffer), "%d", frame->values[i]); DrawText(buffer, (int)cell.x + 23, (int)cell.y + 22, 17, active ? PURPLE : TEXT); snprintf(buffer, sizeof(buffer), "%d", i); DrawText(buffer, (int)cell.x + 28, (int)cell.y + 73, 9, LIGHT_MUTED); }
}
static void draw_list_stage(const Frame *frame, Rectangle stage) {
    int cell_width = 86, gap = 33, total = frame->count * cell_width + (frame->count - 1) * gap, start_x = (int)(stage.x + (stage.width - total) / 2), y = (int)(stage.y + 105);
    for (int i = 0; i < frame->count; i++) { int x = start_x + i * (cell_width + gap); bool active = frame->activeIndex == i; draw_rounded((Rectangle){x, y, cell_width, 54}, active ? PURPLE_SOFT : SURFACE); DrawRectangleLinesEx((Rectangle){x, y, cell_width, 54}, 1, active ? PURPLE : LINE); char value[20]; snprintf(value, sizeof(value), "%d", frame->values[i]); DrawText(value, x + 19, y + 18, 15, active ? PURPLE : TEXT); DrawText(i == frame->count - 1 ? "×" : "→", x + 63, y + 18, 14, active ? PURPLE : MUTED); if (i < frame->count - 1) DrawText("→", x + cell_width + 9, y + 17, 18, PURPLE); }
    DrawText("head", start_x, y - 28, 10, MUTED); if (frame->count) DrawText("tail", start_x + (frame->count - 1) * (cell_width + gap) + 55, y - 28, 10, MUTED);
}
static void draw_stack_stage(const Frame *frame, Rectangle stage) {
    int x = (int)stage.x + 225, bottom = (int)stage.y + 210; DrawLine(x, bottom, x + 120, bottom, NAVY); DrawLine(x, bottom, x, (int)stage.y + 65, LINE); DrawLine(x + 120, bottom, x + 120, (int)stage.y + 65, LINE); DrawText("TOP", x + 139, (int)stage.y + 67, 9, MUTED);
    for (int i = 0; i < frame->count; i++) { int y = bottom - 25 - i * 36; bool active = frame->activeIndex == i || i == frame->count - 1; draw_rounded((Rectangle){x + 12, y, 96, 30}, active ? CORAL_SOFT : PURPLE_SOFT); char value[20]; snprintf(value, sizeof(value), "%d", frame->values[i]); DrawText(value, x + 49, y + 9, 11, active ? CORAL : PURPLE); }
}
static void draw_queue_stage(const Frame *frame, Rectangle stage) {
    int cell_width = 62, gap = 7, total = frame->count * cell_width + (frame->count - 1) * gap, start_x = (int)(stage.x + (stage.width - total) / 2), y = (int)stage.y + 105;
    DrawText("FRONT", start_x - 2, y - 32, 9, MUTED); DrawText("REAR", start_x + total - 28, y - 32, 9, MUTED); DrawLine(start_x, y - 17, start_x + total, y - 17, LINE); DrawLine(start_x, y + 64, start_x + total, y + 64, LINE);
    for (int i = 0; i < frame->count; i++) { int x = start_x + i * (cell_width + gap); bool active = frame->activeIndex == i || i == 0; draw_rounded((Rectangle){x, y, cell_width, 47}, active ? (i == 0 ? CORAL_SOFT : MINT_SOFT) : SURFACE); DrawRectangleLinesEx((Rectangle){x, y, cell_width, 47}, 1, i == 0 ? CORAL : i == frame->count - 1 ? MINT : LINE); char value[20]; snprintf(value, sizeof(value), "%d", frame->values[i]); DrawText(value, x + 23, y + 16, 12, i == 0 ? CORAL : TEXT); }
}
static void draw_tree_node(const Frame *frame, int index, int count, float x, float y, float spread) {
    if (index >= count) return;
    if (2 * index + 1 < count) { DrawLineEx((Vector2){x, y + 20}, (Vector2){x - spread, y + 70}, 2, LINE); draw_tree_node(frame, 2 * index + 1, count, x - spread, y + 70, spread * 0.52f); }
    if (2 * index + 2 < count) { DrawLineEx((Vector2){x, y + 20}, (Vector2){x + spread, y + 70}, 2, LINE); draw_tree_node(frame, 2 * index + 2, count, x + spread, y + 70, spread * 0.52f); }
    bool active = frame->activeValue == frame->values[index]; draw_rounded((Rectangle){x - 20, y, 40, 40}, active ? PURPLE_SOFT : SURFACE); DrawCircleLines((int)x, (int)(y + 20), 20, active ? PURPLE : LINE); char value[16]; snprintf(value, sizeof(value), "%d", frame->values[index]); DrawText(value, (int)x - MeasureText(value, 11) / 2, (int)y + 15, 11, active ? PURPLE : TEXT);
}
static void draw_tree_stage(const Frame *frame, Rectangle stage) { draw_tree_node(frame, 0, frame->count, stage.x + stage.width / 2, stage.y + 34, 160); }
static void draw_sort_stage(const Frame *frame, Rectangle stage) {
    int count = frame->count, gap = 11, bar_width = 41, total = count * bar_width + (count - 1) * gap, start_x = (int)(stage.x + (stage.width - total) / 2), bottom = (int)stage.y + 220, max_value = 1;
    for (int i = 0; i < count; i++) if (frame->values[i] > max_value) max_value = frame->values[i];
    for (int i = 0; i < count; i++) { int height = 26 + frame->values[i] * 145 / max_value, x = start_x + i * (bar_width + gap), y = bottom - height; bool compared = frame->compareA == i || frame->compareB == i; draw_rounded((Rectangle){x, y, bar_width, height}, compared ? CORAL : PURPLE); char value[16]; snprintf(value, sizeof(value), "%d", frame->values[i]); DrawText(value, x + (bar_width - MeasureText(value, 10)) / 2, y - 18, 10, MUTED); snprintf(value, sizeof(value), "%d", i); DrawText(value, x + 16, bottom + 9, 9, LIGHT_MUTED); }
}
static const Vector2 graph_points[7] = {{0.50f,0.18f},{0.24f,0.45f},{0.76f,0.45f},{0.12f,0.79f},{0.40f,0.79f},{0.64f,0.79f},{0.90f,0.79f}};
static bool graph_was_visited(const Frame *frame, int node) { for (int i = 0; i < frame->graphVisitedCount; i++) if (frame->graphVisited[i] == node) return true; return false; }
static void draw_graph_stage(const Frame *frame, Rectangle stage) {
    for (int i = 0; i < 7; i++) { int from = graph_edges[i][0], to = graph_edges[i][1]; Vector2 a = {stage.x + graph_points[from].x * stage.width, stage.y + graph_points[from].y * stage.height}; Vector2 b = {stage.x + graph_points[to].x * stage.width, stage.y + graph_points[to].y * stage.height}; DrawLineEx(a, b, 2, LINE); }
    for (int i = 0; i < 7; i++) { Vector2 point = {stage.x + graph_points[i].x * stage.width, stage.y + graph_points[i].y * stage.height}; bool visited = graph_was_visited(frame, i), active = frame->graphActive == i, start = frame->graphStart == i; Color fill = active ? PURPLE_SOFT : visited ? MINT_SOFT : start ? CORAL_SOFT : SURFACE; Color ring = active ? PURPLE : visited ? MINT : start ? CORAL : LINE; DrawCircleV(point, 22, fill); DrawCircleLines((int)point.x, (int)point.y, 22, ring); char label[2] = {(char)('A' + i), '\0'}; DrawText(label, (int)point.x - 5, (int)point.y - 7, 14, active ? PURPLE : TEXT); }
    DrawText("start", (int)stage.x + 8, (int)stage.y + stage.height - 5, 9, CORAL); DrawText("current", (int)stage.x + 60, (int)stage.y + stage.height - 5, 9, PURPLE); DrawText("visited", (int)stage.x + 125, (int)stage.y + stage.height - 5, 9, MINT);
}
static void draw_stage(App *app, Rectangle stage) {
    Frame *frame = &app->frames[app->currentFrame];
    draw_rounded(stage, SOFT);
    for (int x = (int)stage.x + 15; x < stage.x + stage.width; x += 16) for (int y = (int)stage.y + 15; y < stage.y + stage.height; y += 16) DrawCircle(x, y, 1, (Color){225,230,239,170});
    if (app->module == ARRAY_MODULE) draw_array_stage(frame, stage); else if (app->module == LIST_MODULE) draw_list_stage(frame, stage); else if (app->module == STACK_MODULE) draw_stack_stage(frame, stage); else if (app->module == QUEUE_MODULE) draw_queue_stage(frame, stage); else if (app->module == TREE_MODULE) draw_tree_stage(frame, stage); else if (app->module == SORT_MODULE) draw_sort_stage(frame, stage); else draw_graph_stage(frame, stage);
    DrawText(frame->title, (int)stage.x + 20, (int)stage.y + stage.height - 45, 12, TEXT); DrawText(frame->detail, (int)stage.x + 20, (int)stage.y + stage.height - 25, 10, MUTED);
}
static void draw_timeline(App *app, Rectangle bounds) {
    Frame *frame = &app->frames[app->currentFrame]; (void)frame; float progress = app->frameCount <= 1 ? 0 : (float)app->currentFrame / (float)(app->frameCount - 1);
    DrawText("STEP", (int)bounds.x, (int)bounds.y - 3, 9, LIGHT_MUTED); DrawText(TextFormat("%02d", app->currentFrame + 1), (int)bounds.x + 39, (int)bounds.y - 4, 12, PURPLE); DrawText("/", (int)bounds.x + 65, (int)bounds.y - 3, 10, LIGHT_MUTED); DrawText(TextFormat("%02d", app->frameCount), (int)bounds.x + 76, (int)bounds.y - 4, 12, LIGHT_MUTED);
    DrawRectangle((int)bounds.x + 113, (int)bounds.y, (int)bounds.width - 160, 5, (Color){233,237,245,255}); DrawRectangle((int)bounds.x + 113, (int)bounds.y, (int)((bounds.width - 160) * progress), 5, PURPLE); DrawCircle((int)(bounds.x + 113 + (bounds.width - 160) * progress), (int)bounds.y + 2, 6, PURPLE); DrawText(TextFormat("%d%%", (int)(progress * 100)), (int)bounds.x + bounds.width - 35, (int)bounds.y - 4, 10, MUTED);
}
static void draw_playback(App *app, Rectangle bounds) {
    draw_button((Rectangle){bounds.x, bounds.y, 33, 32}, "‹", false, PURPLE); draw_button((Rectangle){bounds.x + 42, bounds.y, 132, 32}, app->playing ? "Ⅱ  Pause" : "▶  Play animation", true, PURPLE); draw_button((Rectangle){bounds.x + 184, bounds.y, 33, 32}, "›", false, PURPLE);
    DrawText("SPEED", (int)bounds.x + bounds.width - 172, (int)bounds.y + 8, 9, LIGHT_MUTED); DrawRectangle((int)bounds.x + bounds.width - 105, (int)bounds.y + 14, 74, 4, (Color){225,230,239,255}); DrawRectangle((int)bounds.x + bounds.width - 105, (int)bounds.y + 14, (int)(74 * ((app->speed - 0.5f) / 1.5f)), 4, PURPLE); DrawCircle((int)(bounds.x + bounds.width - 105 + 74 * ((app->speed - 0.5f) / 1.5f)), (int)bounds.y + 16, 6, PURPLE); DrawText(TextFormat("%.2gx", app->speed), (int)bounds.x + bounds.width - 21, (int)bounds.y + 8, 10, TEXT);
}
static void draw_complexity(App *app, Rectangle panel) {
    int x = (int)panel.x + 22, y = (int)panel.y + 23; draw_section_title("COMPLEXITY", "Performance map", x, y); DrawText(app->module == ARRAY_MODULE ? "O(n)" : app->module == TREE_MODULE ? "O(log n)" : app->module == GRAPH_MODULE ? "O(V + E)" : "O(1)", (int)panel.x + panel.width - 75, y + 3, 10, MINT);
    const char *labels[] = {"Access / peek", "Search", "Insert / remove"}; const char *values[] = {app->module == TREE_MODULE ? "O(log n)" : "O(1)", app->module == SORT_MODULE ? "O(n²)" : app->module == GRAPH_MODULE ? "O(V + E)" : "O(n)", app->module == ARRAY_MODULE ? "O(n)" : app->module == TREE_MODULE ? "O(log n)" : "O(1)"};
    for (int i = 0; i < 3; i++) { int yy = y + 61 + i * 47; DrawText(labels[i], x, yy, 10, MUTED); DrawText(values[i], (int)panel.x + panel.width - 62, yy, 10, TEXT); DrawRectangle(x, yy + 18, (int)panel.width - 44, 5, (Color){237,240,246,255}); DrawRectangle(x, yy + 18, (int)((panel.width - 44) * (i == 1 ? .55f : .87f)), 5, i == 1 ? CORAL : i == 2 ? MINT : PURPLE); }
}
static void draw_concept(App *app, Rectangle panel) {
    int x = (int)panel.x + 22, y = (int)panel.y + 23; draw_section_title("CONCEPT", app->module == ARRAY_MODULE ? "Contiguous memory" : app->module == LIST_MODULE ? "Nodes + pointers" : app->module == STACK_MODULE ? "LIFO" : app->module == QUEUE_MODULE ? "FIFO" : app->module == TREE_MODULE ? "Divide the search space" : app->module == SORT_MODULE ? "Local decisions, global order" : "A graph has many paths", x, y); draw_wrapped(module_copies[app->module], x, y + 67, (int)panel.width - 44, 10, MUTED); DrawLine(x, (int)panel.y + panel.height - 41, (int)panel.x + panel.width - 22, (int)panel.y + panel.height - 41, LINE); DrawText("What to notice", x, (int)panel.y + panel.height - 27, 10, PURPLE); DrawText("→", (int)panel.x + panel.width - 38, (int)panel.y + panel.height - 29, 15, PURPLE);
}
static void draw_lesson(App *app, Rectangle panel) {
    draw_rounded(panel, (Color){255,250,242,255}); DrawRectangle((int)panel.x + 16, (int)panel.y + 18, 28, 28, (Color){255,240,204,255}); DrawText("✦", (int)panel.x + 25, (int)panel.y + 23, 13, (Color){217,148,37,255}); DrawText("LEARNING CHECKPOINT", (int)panel.x + 56, (int)panel.y + 17, 9, (Color){166,108,24,255});
    const char *tips[] = {"Try inserting into the middle and watch every item shift.", "Prepend a node and notice that only one pointer changes.", "Push twice, then pop once: newest goes first.", "Dequeue twice: the front pointer keeps moving.", "Insert a smaller value and follow the left branch.", "Compare the choreography of two different algorithms.", "Run BFS and DFS from the same start node."}; draw_wrapped(tips[app->module], (int)panel.x + 56, (int)panel.y + 36, (int)panel.width - 75, 9, (Color){179,140,88,255});
}
static void draw_operation_panel(App *app, Rectangle panel) {
    int x = (int)panel.x + 24, y = (int)panel.y + 23; draw_section_title("TRY IT YOURSELF", "Build an operation", x, y); DrawText("Enter", (int)panel.x + panel.width - 63, y + 4, 9, LIGHT_MUTED); DrawText("↵", (int)panel.x + panel.width - 32, y + 2, 13, MUTED);
    int button_y = (int)panel.y + 71, button_x = x;
    if (app->module == ARRAY_MODULE) { const char *labels[] = {"Insert", "Remove", "Find", "Update"}; for (int i = 0; i < 4; i++) draw_button((Rectangle){button_x + i * 94, button_y, 82, 31}, labels[i], app->action == (Action)i, app->action == (Action)i ? PURPLE : PURPLE); DrawText(TextFormat("Value  %d", app->operationValue), x, button_y + 49, 10, MUTED); DrawText(TextFormat("Index  %d", app->operationIndex), x + 103, button_y + 49, 10, MUTED); }
    else if (app->module == LIST_MODULE) { const char *labels[] = {"Append", "Prepend", "Delete"}; for (int i = 0; i < 3; i++) draw_button((Rectangle){button_x + i * 100, button_y, 88, 31}, labels[i], app->operationMode == i, PURPLE); DrawText(TextFormat("Value  %d", app->operationValue), x, button_y + 49, 10, MUTED); }
    else if (app->module == STACK_MODULE) { const char *labels[] = {"Push onto top", "Pop from top"}; for (int i = 0; i < 2; i++) draw_button((Rectangle){button_x + i * 139, button_y, 125, 31}, labels[i], app->operationMode == i, PURPLE); DrawText(TextFormat("Value  %d", app->operationValue), x, button_y + 49, 10, MUTED); }
    else if (app->module == QUEUE_MODULE) { const char *labels[] = {"Enqueue", "Dequeue"}; for (int i = 0; i < 2; i++) draw_button((Rectangle){button_x + i * 112, button_y, 98, 31}, labels[i], app->operationMode == i, PURPLE); DrawText(TextFormat("Value  %d", app->operationValue), x, button_y + 49, 10, MUTED); }
    else if (app->module == TREE_MODULE) { const char *labels[] = {"Insert value", "Search value"}; for (int i = 0; i < 2; i++) draw_button((Rectangle){button_x + i * 122, button_y, 108, 31}, labels[i], app->operationMode == i, PURPLE); DrawText(TextFormat("Value  %d", app->operationValue), x, button_y + 49, 10, MUTED); }
    else if (app->module == SORT_MODULE) { draw_button((Rectangle){button_x, button_y, 115, 31}, "Bubble sort", true, PURPLE); draw_button((Rectangle){button_x + 127, button_y, 115, 31}, app->operationMode == 2 ? "Descending" : "Ascending", app->operationMode == 2, app->operationMode == 2 ? CORAL : MINT); DrawText("Press D to switch order", x, button_y + 49, 10, MUTED); }
    else { draw_button((Rectangle){button_x, button_y, 152, 31}, "Breadth-first", app->operationMode == 0, PURPLE); draw_button((Rectangle){button_x + 164, button_y, 126, 31}, "Depth-first", app->operationMode == 1, PURPLE); DrawText(TextFormat("Start node  %c", 'A' + app->operationIndex), x, button_y + 49, 10, MUTED); }
    Rectangle run = {panel.x + panel.width - 150, panel.y + 101, 126, 37}; draw_button(run, "Run operation  →", true, CORAL);
    DrawText("Try it, then step through every frame with the transport controls.", x, (int)panel.y + panel.height - 19, 9, MUTED);
}
static void draw_visualizer_panel(App *app, Rectangle panel) {
    int x = (int)panel.x, y = (int)panel.y; draw_rounded(panel, SURFACE); draw_section_title(module_short[app->module], module_titles[app->module], x + 25, y + 23); DrawCircle(x + panel.width - 75, y + 34, 4, MINT); DrawText(app->playing ? "Playing" : app->currentFrame == app->frameCount - 1 && app->frameCount > 1 ? "Complete" : "Ready", x + panel.width - 61, y + 28, 10, MINT);
    Rectangle stage = {panel.x + 25, panel.y + 84, panel.width - 50, 285}; draw_stage(app, stage); draw_timeline(app, (Rectangle){panel.x + 25, panel.y + 388, panel.width - 50, 22}); draw_playback(app, (Rectangle){panel.x + 25, panel.y + 416, panel.width - 50, 32});
}
static void draw_main(App *app) {
    int screen_width = GetScreenWidth(), screen_height = GetScreenHeight();
    DrawRectangle(SIDEBAR_WIDTH, 76, screen_width - SIDEBAR_WIDTH, screen_height - 76, BG); draw_header(app); draw_sidebar(app);
    int x = SIDEBAR_WIDTH + 46, y = 111, right_width = 285, gap = 22, available = screen_width - x - 46, left_width = available - right_width - gap;
    DrawText("DATA STRUCTURES · INTERACTIVE", x, y, 10, PURPLE); DrawCircle(x - 10, y + 5, 3, CORAL); DrawText(module_titles[app->module], x, y + 22, 30, TEXT); DrawText("See how every operation changes the structure, one deliberate step at a time.", x, y + 64, 12, MUTED); draw_button((Rectangle){x + left_width - 121, y + 27, 121, 31}, "↺  Reset module", false, PURPLE);
    int panels_y = 202; Rectangle visual = {x, panels_y, (float)left_width, 465}; Rectangle operation = {x, panels_y + 484, (float)left_width, 151}; Rectangle concept = {x + left_width + gap, panels_y, (float)right_width, 194}; Rectangle complexity = {x + left_width + gap, panels_y + 216, (float)right_width, 239}; Rectangle lesson = {x + left_width + gap, panels_y + 475, (float)right_width, 112};
    draw_visualizer_panel(app, visual); draw_rounded(operation, SURFACE); draw_operation_panel(app, operation); draw_rounded(concept, (Color){250,249,255,255}); draw_concept(app, concept); draw_rounded(complexity, SURFACE); draw_complexity(app, complexity); draw_lesson(app, lesson);
}
static void handle_mouse(App *app) {
    int screen_width = GetScreenWidth(), available = screen_width - (SIDEBAR_WIDTH + 46) - 46, right_width = 285, gap = 22, left_width = available - right_width - gap; int x = SIDEBAR_WIDTH + 46, panels_y = 202;
    for (int i = 0; i < MODULE_COUNT; i++) { if (clicked((Rectangle){18, 124 + i * 45, SIDEBAR_WIDTH - 36, 37})) { go_to_module(app, (Module)i); return; } }
    if (clicked((Rectangle){screen_width - 208, 22, 31, 31})) { app->dark = !app->dark; return; }
    if (clicked((Rectangle){x + left_width - 121, 138, 121, 31})) { set_module_defaults(app); reset_frames(app); return; }
    Rectangle visual = {x, panels_y, (float)left_width, 465};
    if (clicked((Rectangle){visual.x + 25, visual.y + 416, 33, 32})) { advance_frame(app, -1); return; }
    if (clicked((Rectangle){visual.x + 67, visual.y + 416, 132, 32})) { app->playing = !app->playing; app->nextTick = GetTime(); return; }
    if (clicked((Rectangle){visual.x + 209, visual.y + 416, 33, 32})) { advance_frame(app, 1); return; }
    Rectangle timeline = {visual.x + 138, visual.y + 388, visual.width - 185, 22}; if (clicked(timeline)) { float progress = (GetMousePosition().x - timeline.x) / timeline.width; app->currentFrame = (int)(progress * (app->frameCount - 1)); if (app->currentFrame < 0) app->currentFrame = 0; if (app->currentFrame >= app->frameCount) app->currentFrame = app->frameCount - 1; return; }
    Rectangle operation = {x, panels_y + 484, (float)left_width, 151}; int button_y = (int)operation.y + 71, button_x = (int)operation.x + 24;
    if (app->module == ARRAY_MODULE) for (int i = 0; i < 4; i++) if (clicked((Rectangle){button_x + i * 94, button_y, 82, 31})) app->action = (Action)i;
    if (app->module == LIST_MODULE) for (int i = 0; i < 3; i++) if (clicked((Rectangle){button_x + i * 100, button_y, 88, 31})) app->operationMode = i;
    if (app->module == STACK_MODULE) for (int i = 0; i < 2; i++) if (clicked((Rectangle){button_x + i * 139, button_y, 125, 31})) app->operationMode = i;
    if (app->module == QUEUE_MODULE) for (int i = 0; i < 2; i++) if (clicked((Rectangle){button_x + i * 112, button_y, 98, 31})) app->operationMode = i;
    if (app->module == TREE_MODULE) for (int i = 0; i < 2; i++) if (clicked((Rectangle){button_x + i * 122, button_y, 108, 31})) app->operationMode = i;
    if (app->module == SORT_MODULE && clicked((Rectangle){button_x + 127, button_y, 115, 31})) app->operationMode = app->operationMode == 2 ? 0 : 2;
    if (app->module == GRAPH_MODULE && clicked((Rectangle){button_x + 164, button_y, 126, 31})) app->operationMode = 1;
    if (clicked((Rectangle){operation.x + operation.width - 150, operation.y + 101, 126, 37})) { run_operation(app); return; }
}
static void handle_keyboard(App *app) {
    if (IsKeyPressed(KEY_ONE)) go_to_module(app, ARRAY_MODULE); if (IsKeyPressed(KEY_TWO)) go_to_module(app, LIST_MODULE); if (IsKeyPressed(KEY_THREE)) go_to_module(app, STACK_MODULE); if (IsKeyPressed(KEY_FOUR)) go_to_module(app, QUEUE_MODULE); if (IsKeyPressed(KEY_FIVE)) go_to_module(app, TREE_MODULE); if (IsKeyPressed(KEY_SIX)) go_to_module(app, SORT_MODULE); if (IsKeyPressed(KEY_SEVEN)) go_to_module(app, GRAPH_MODULE);
    if (IsKeyPressed(KEY_SPACE)) { app->playing = !app->playing; app->nextTick = GetTime(); }
    if (IsKeyPressed(KEY_RIGHT)) advance_frame(app, 1); if (IsKeyPressed(KEY_LEFT)) advance_frame(app, -1); if (IsKeyPressed(KEY_R)) { set_module_defaults(app); reset_frames(app); } if (IsKeyPressed(KEY_ENTER)) run_operation(app);
    if (IsKeyPressed(KEY_UP)) app->operationValue++; if (IsKeyPressed(KEY_DOWN)) app->operationValue--; if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) app->speed += .25f; if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) app->speed -= .25f;
    if (app->speed < .5f) app->speed = .5f; if (app->speed > 2.0f) app->speed = 2.0f;
    if (IsKeyPressed(KEY_I)) app->operationMode = 0; if (IsKeyPressed(KEY_D)) app->operationMode = app->module == SORT_MODULE ? 2 : 1; if (IsKeyPressed(KEY_F)) { app->action = ACTION_FIND; app->operationMode = 1; } if (IsKeyPressed(KEY_U)) app->action = ACTION_UPDATE;

}
static void update_animation(App *app) {
    if (!app->playing || app->frameCount <= 1) return;
    if (GetTime() >= app->nextTick) { app->currentFrame++; app->nextTick = GetTime() + 0.92 / app->speed; if (app->currentFrame >= app->frameCount) { app->currentFrame = app->frameCount - 1; app->playing = false; } }
}
int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT); InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "AlgoFlow — Interactive DSA Visualizer"); SetTargetFPS(60);
    App app; init_app(&app);
    while (!WindowShouldClose()) { handle_keyboard(&app); handle_mouse(&app); update_animation(&app); BeginDrawing(); ClearBackground(BG); draw_main(&app); EndDrawing(); }
    CloseWindow(); return 0;
}


