#include "dialogue_system.h"

#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

static void read_option(DialogueResponseOption *option, lua_State *luaState) {
    option->nextId = NULL;

    lua_getfield(luaState, -1, "text");
    const char *optionText = lua_tostring(luaState, -1);
    option->text = (char *)malloc(strlen(optionText) + 1);
    strcpy(option->text, optionText);
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "next");
    if (lua_isstring(luaState, -1)) {
        const char *nextId = lua_tostring(luaState, -1);
        option->nextId = (char *)malloc(strlen(nextId) + 1);
        strcpy(option->nextId, nextId);
    }
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "effect");
    if (!lua_isnil(luaState, -1)) {
        option->luaEffectRef = luaL_ref(luaState, LUA_REGISTRYINDEX);
    } else {
        option->luaEffectRef = LUA_NOREF;
        lua_pop(luaState, 1);
    }

    lua_getfield(luaState, -1, "condition");
    if (!lua_isnil(luaState, -1)) {
        option->luaConditionRef = luaL_ref(luaState, LUA_REGISTRYINDEX);
    } else {
        option->luaConditionRef = LUA_NOREF;
        lua_pop(luaState, 1);
    }
}

static void read_part(DialoguePart *part, lua_State *luaState) {
    lua_getfield(luaState, -1, "id");
    const char *partId = lua_tostring(luaState, -1);
    part->id = (char *)malloc(strlen(partId) + 1);
    strcpy(part->id, partId);
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "text");
    const char *partText = lua_tostring(luaState, -1);
    part->text = (char *)malloc(strlen(partText) + 1);
    strcpy(part->text, partText);
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "options");
    luaL_checktype(luaState, -1, LUA_TTABLE);
    part->optionsCount = (int)luaL_len(luaState, -1);
    part->options = malloc(sizeof(DialogueResponseOption) * part->optionsCount);

    // Lua tables start from 1
    for (int i = 1; i <= part->optionsCount; i++) {
        lua_geti(luaState, -1, i);

        read_option(&part->options[i - 1], luaState);

        lua_pop(luaState, 1);
    }

    lua_pop(luaState, 1);
}

DialoguesError read_lua_dialogue(DialogueSystem *system, const char *filename,
                                 LuaContextFn context_index, LuaContextFn context_newindex) {
    system->luaState = luaL_newstate();
    luaL_openlibs(system->luaState);

    luaL_newmetatable(system->luaState, "GameContext");
    lua_pushcfunction(system->luaState, context_index);
    lua_setfield(system->luaState, -2, "__index");
    lua_pushcfunction(system->luaState, context_newindex);
    lua_setfield(system->luaState, -2, "__newindex");
    lua_pop(system->luaState, 1);

    if (luaL_dofile(system->luaState, filename) != LUA_OK) {
        lua_close(system->luaState);
        return DIALOGUE_NOT_FOUND;
    }

    luaL_checktype(system->luaState, -1, LUA_TTABLE);
    system->partsCount = (int)luaL_len(system->luaState, -1);
    system->parts = malloc(sizeof(DialoguePart) * system->partsCount);

    // Lua tables start from 1
    for (int i = 1; i <= system->partsCount; i++) {
        lua_geti(system->luaState, -1, i);

        read_part(&system->parts[i - 1], system->luaState);

        lua_pop(system->luaState, 1);
    }

    return DIALOGUE_OK;
}

void free_dialogue(DialogueSystem *system) {
    lua_close(system->luaState);
    for (int i = 0; i < system->partsCount; i++) {
        DialoguePart *part = &system->parts[i];

        for (int j = 0; j < part->optionsCount; j++) {
            DialogueResponseOption *option = &part->options[j];
            free(option->text);
            if (option->nextId != NULL) {
                free(option->nextId);
            }
        }

        free(part->options);
        free(part->id);
        free(part->text);
    }

    free(system->parts);
}

bool condition_met(DialogueSystem *system, DialogueResponseOption *option, void *context) {
    if (option->luaConditionRef == LUA_NOREF) {
        return true;
    }

    lua_rawgeti(system->luaState, LUA_REGISTRYINDEX, option->luaConditionRef);
    lua_pushlightuserdata(system->luaState, context);
    luaL_getmetatable(system->luaState, "GameContext");
    lua_setmetatable(system->luaState, -2);

    if (lua_pcall(system->luaState, 1, 1, 0) != LUA_OK) {
        printf("Condition error");
    }

    bool condition_condition_result = (bool)lua_toboolean(system->luaState, -1);
    lua_pop(system->luaState, 1);

    return condition_condition_result;
}

void invoke_effect(DialogueSystem *system, DialogueResponseOption *option, void *context) {
    if (option->luaEffectRef == LUA_NOREF) {
        return;
    }

    lua_rawgeti(system->luaState, LUA_REGISTRYINDEX, option->luaEffectRef);
    lua_pushlightuserdata(system->luaState, context);
    luaL_getmetatable(system->luaState, "GameContext");
    lua_setmetatable(system->luaState, -2);

    if (lua_pcall(system->luaState, 1, 1, 0) != LUA_OK) {
        printf("Effect error");
    }

    lua_pop(system->luaState, 1);
}

void goto_part(DialogueSystem *system, const char *stageId) {
    for (int i = 0; i < system->partsCount; i++) {
        if (strcmp(stageId, system->parts[i].id) == 0) {
            system->currentPart = &system->parts[i];
        }
    }
}
