#include "dialogue_system.h"

#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#define HASH_MULTIPLIER 31

unsigned int hash(const char *key) {
    unsigned int hashedIndex = 0;
    while (*key) {
        hashedIndex = (hashedIndex * HASH_MULTIPLIER) + *key++;
    }
    return hashedIndex % MAX_PART_POOLS_SIZE;
}

static void read_option(DialogueResponse *response, lua_State *luaState) {
    response->nextId = NULL;

    lua_getfield(luaState, -1, "text");
    const char *optionText = lua_tostring(luaState, -1);
    response->text = (char *)malloc(strlen(optionText) + 1);
    strcpy(response->text, optionText);
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "next");
    if (lua_isstring(luaState, -1)) {
        const char *nextId = lua_tostring(luaState, -1);
        response->nextId = (char *)malloc(strlen(nextId) + 1);
        strcpy(response->nextId, nextId);
    }
    lua_pop(luaState, 1);

    lua_getfield(luaState, -1, "effect");
    if (!lua_isnil(luaState, -1)) {
        response->luaEffectRef = luaL_ref(luaState, LUA_REGISTRYINDEX);
    } else {
        response->luaEffectRef = LUA_NOREF;
        lua_pop(luaState, 1);
    }

    lua_getfield(luaState, -1, "condition");
    if (!lua_isnil(luaState, -1)) {
        response->luaConditionRef = luaL_ref(luaState, LUA_REGISTRYINDEX);
    } else {
        response->luaConditionRef = LUA_NOREF;
        lua_pop(luaState, 1);
    }
}

static void read_part(Dialogue *dialogue, lua_State *luaState) {
    lua_getfield(luaState, -1, "id");
    const char *partId = lua_tostring(luaState, -1);

    DialoguePartBucket *newBucket;

    unsigned int hashedKey = hash(partId);
    if (dialogue->partBuckets[hashedKey]) {
        DialoguePartBucket *bucket = dialogue->partBuckets[hashedKey];
        while (bucket->next) {
            bucket = bucket->next;
        }
        bucket->next = malloc(sizeof(DialoguePartBucket));
        newBucket = bucket->next;
    } else {
        dialogue->partBuckets[hashedKey] = malloc(sizeof(DialoguePartBucket));
        newBucket = dialogue->partBuckets[hashedKey];
    }
    memset(newBucket, 0, sizeof(DialoguePartBucket));

    DialoguePart *part = &newBucket->dialoguePart;
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
    part->options = malloc(sizeof(DialogueResponse) * part->optionsCount);

    // Lua tables start from 1
    for (int i = 1; i <= part->optionsCount; i++) {
        lua_geti(luaState, -1, i);

        read_option(&part->options[i - 1], luaState);

        lua_pop(luaState, 1);
    }

    lua_pop(luaState, 1);
}

DialoguesError read_lua_dialogue(Dialogue *dialogue, const char *filename,
                                 LuaContextFn context_index, LuaContextFn context_newindex) {
    dialogue->luaState = luaL_newstate();
    luaL_openlibs(dialogue->luaState);

    luaL_newmetatable(dialogue->luaState, "GameContext");
    lua_pushcfunction(dialogue->luaState, context_index);
    lua_setfield(dialogue->luaState, -2, "__index");
    lua_pushcfunction(dialogue->luaState, context_newindex);
    lua_setfield(dialogue->luaState, -2, "__newindex");
    lua_pop(dialogue->luaState, 1);

    if (luaL_dofile(dialogue->luaState, filename) != LUA_OK) {
        lua_close(dialogue->luaState);
        return DIALOGUE_NOT_FOUND;
    }

    luaL_checktype(dialogue->luaState, -1, LUA_TTABLE);
    int partsCount = (int)luaL_len(dialogue->luaState, -1);

    // Lua tables start from 1
    for (int i = 1; i <= partsCount; i++) {
        lua_geti(dialogue->luaState, -1, i);

        read_part(dialogue, dialogue->luaState);

        lua_pop(dialogue->luaState, 1);
    }

    return DIALOGUE_OK;
}

void free_dialogue(Dialogue *dialogue) {
    lua_close(dialogue->luaState);
    for (int i = 0; i < MAX_PART_POOLS_SIZE; i++) {
        free_bucket(dialogue->partBuckets[i]);
    }
}

bool condition_met(Dialogue *dialogue, DialogueResponse *response, void *context) {
    if (response->luaConditionRef == LUA_NOREF) {
        return true;
    }

    lua_rawgeti(dialogue->luaState, LUA_REGISTRYINDEX, response->luaConditionRef);
    lua_pushlightuserdata(dialogue->luaState, context);
    luaL_getmetatable(dialogue->luaState, "GameContext");
    lua_setmetatable(dialogue->luaState, -2);

    if (lua_pcall(dialogue->luaState, 1, 1, 0) != LUA_OK) {
        printf("Condition error: %s", lua_tostring(dialogue->luaState, -1));
        lua_pop(dialogue->luaState, 1);
        return false;
    }

    bool result = (bool)lua_toboolean(dialogue->luaState, -1);
    lua_pop(dialogue->luaState, 1);
    return result;
}

void invoke_effect(Dialogue *dialogue, DialogueResponse *response, void *context) {
    if (response->luaEffectRef == LUA_NOREF) {
        return;
    }

    lua_rawgeti(dialogue->luaState, LUA_REGISTRYINDEX, response->luaEffectRef);
    lua_pushlightuserdata(dialogue->luaState, context);
    luaL_getmetatable(dialogue->luaState, "GameContext");
    lua_setmetatable(dialogue->luaState, -2);

    if (lua_pcall(dialogue->luaState, 1, 0, 0) != LUA_OK) {
        printf("Effect error: %s", lua_tostring(dialogue->luaState, -1));
        lua_pop(dialogue->luaState, 1);
    }
}

DialoguePart *find_part(Dialogue *dialogue, const char *stageId) {
    if (!stageId) {
        return NULL;
    }

    unsigned int bucketId = hash(stageId);
    DialoguePartBucket *bucket = dialogue->partBuckets[bucketId];
    while (bucket) {
        if (strcmp(bucket->dialoguePart.id, stageId) == 0) {
            return &bucket->dialoguePart;
        }
        bucket = bucket->next;
    }

    return NULL;
}

void free_bucket(DialoguePartBucket *bucket) {
    if (!bucket) {
        return;
    }
    if (bucket->next) {
        free_bucket(bucket->next);
    }

    DialoguePart *part = &bucket->dialoguePart;
    free(part->id);
    free(part->text);

    for (int i = 0; i < part->optionsCount; i++) {
        free(part->options[i].text);
        free(part->options[i].nextId);
    }
    free(part->options);

    free(bucket);
}
