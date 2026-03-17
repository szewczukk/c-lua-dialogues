#pragma once

#include <lua.h>

typedef int (*LuaContextFn)(lua_State *luaState);

typedef struct DialogueResponse {
    char *text;
    char *nextId;
    int luaEffectRef;
    int luaConditionRef;
} DialogueResponse;

typedef struct DialoguePart {
    char *id;
    char *text;
    int optionsCount;
    DialogueResponse *options;
} DialoguePart;

typedef struct DialoguePartBucket {
    struct DialoguePartBucket *next;
    DialoguePart dialoguePart;
} DialoguePartBucket;

#define MAX_PART_POOLS_SIZE 64

typedef struct Dialogue {
    DialoguePartBucket *partBuckets[MAX_PART_POOLS_SIZE];
    lua_State *luaState;
} Dialogue;

typedef enum DialoguesError {
    DIALOGUE_OK,
    DIALOGUE_NOT_FOUND,
    DIALOGUE_PART_NOT_FOUND
} DialoguesError;

DialoguesError read_lua_dialogue(Dialogue *dialogue, const char *filename,
                                 LuaContextFn context_index, LuaContextFn context_newindex);
void free_dialogue(Dialogue *dialogue);
bool condition_met(Dialogue *dialogue, DialogueResponse *response, void *context);
void invoke_effect(Dialogue *dialogue, DialogueResponse *response, void *context);
DialoguePart *find_part(Dialogue *dialogue, const char *stageId);
void free_bucket(DialoguePartBucket *bucket);
