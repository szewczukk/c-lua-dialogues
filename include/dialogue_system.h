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

typedef struct Dialogue {
    int partsCount;
    DialoguePart *parts;
    lua_State *luaState;

    DialoguePart *currentPart;
} Dialogue;

typedef enum DialoguesError {
    DIALOGUE_OK,
    DIALOGUE_NOT_FOUND,
} DialoguesError;

DialoguesError read_lua_dialogue(Dialogue *dialogue, const char *filename,
                                 LuaContextFn context_index, LuaContextFn context_newindex);
void free_dialogue(Dialogue *dialogue);
bool condition_met(Dialogue *dialogue, DialogueResponse *response, void *context);
void invoke_effect(Dialogue *dialogue, DialogueResponse *response, void *context);
void goto_part(Dialogue *dialogue, const char *stageId);
