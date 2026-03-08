#ifndef DIALOGUE_SYSTEM_H
#define DIALOGUE_SYSTEM_H

#include <lua.h>

typedef int (*LuaContextFn)(lua_State *luaState);

typedef struct DialogueResponseOption {
    char *text;
    char *nextId;
    int luaEffectRef;
    int luaConditionRef;
} DialogueResponseOption;

typedef struct DialoguePart {
    char *id;
    char *text;
    int optionsCount;
    DialogueResponseOption *options;
} DialoguePart;

typedef struct DialogueSystem {
    int partsCount;
    DialoguePart *parts;
    lua_State *luaState;

    DialoguePart *currentPart;
} DialogueSystem;

typedef enum DialoguesError {
    DIALOGUE_OK,
    DIALOGUE_NOT_FOUND,
} DialoguesError;

DialoguesError read_lua_dialogue(DialogueSystem *system, const char *filename,
                                 LuaContextFn context_index, LuaContextFn context_newindex);
void free_dialogue(DialogueSystem *system);
bool condition_met(DialogueSystem *system, DialogueResponseOption *option, void *context);
void invoke_effect(DialogueSystem *system, DialogueResponseOption *option, void *context);
void goto_part(DialogueSystem *system, const char *stageId);

#endif /* DIALOGUE_SYSTEM_H */
