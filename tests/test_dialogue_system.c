#include "dialogue_system.h"
#include <stdbool.h>
#include <string.h>
#include <unity.h>

static Dialogue dialogue;
static bool system_loaded;

/* Test context for verifying condition/effect receive the context pointer. */
typedef struct TestContext {
    bool effectCalled;
    bool conditionShouldPass;
} TestContext;

/* Stub context callbacks so read_lua_dialogue can load without game logic. */
static int stub_context_index(lua_State *lua_state) {
    lua_pushnil(lua_state);
    return 1;
}

static int stub_context_newindex(lua_State *lua_state) {
    (void)lua_state;
    return 0;
}

/* Test context callbacks: Lua can set context.effectCalled and context.conditionShouldPass. */
static int test_context_index(lua_State *lua_state) {
    TestContext *ctx = (TestContext *)lua_touserdata(lua_state, 1);
    const char *key = lua_tostring(lua_state, 2);
    if (!ctx || !key) {
        return 0;
    }
    if (strcmp(key, "effectCalled") == 0) {
        lua_pushboolean(lua_state, (int)ctx->effectCalled);
        return 1;
    }
    if (strcmp(key, "conditionShouldPass") == 0) {
        lua_pushboolean(lua_state, (int)ctx->conditionShouldPass);
        return 1;
    }
    lua_pushnil(lua_state);
    return 1;
}

static int test_context_newindex(lua_State *luaState) {
    TestContext *ctx = (TestContext *)lua_touserdata(luaState, 1);
    const char *key = lua_tostring(luaState, 2);
    if (!ctx || !key) {
        return 0;
    }
    if (strcmp(key, "effectCalled") == 0) {
        ctx->effectCalled = (bool)lua_toboolean(luaState, 3);
        return 0;
    }
    if (strcmp(key, "conditionShouldPass") == 0) {
        ctx->conditionShouldPass = (bool)lua_toboolean(luaState, 3);
        return 0;
    }
    return 0;
}

void setUp(void) {
    memset(&dialogue, 0, sizeof(dialogue));
    system_loaded = false;
}

void tearDown(void) {
    if (system_loaded) {
        free_dialogue(&dialogue);
        system_loaded = false;
    }
}

/* read_lua_dialogue: success */
void test_read_lua_dialogue_success(void) {
    DialoguesError err = read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua",
                                           stub_context_index, stub_context_newindex);
    system_loaded = true;

    TEST_ASSERT_EQUAL(DIALOGUE_OK, err);

    DialoguePart *start = find_part(&dialogue, "start");
    TEST_ASSERT_NOT_NULL(start);
    TEST_ASSERT_EQUAL_STRING("start", start->id);
    TEST_ASSERT_EQUAL_STRING("Hello. Choose an option.", start->text);
    TEST_ASSERT_EQUAL(2, start->optionsCount);
    TEST_ASSERT_EQUAL_STRING("Go to second", start->options[0].text);
    TEST_ASSERT_EQUAL_STRING("second", start->options[0].nextId);
    TEST_ASSERT_EQUAL_STRING("Stay", start->options[1].text);
    TEST_ASSERT_EQUAL_STRING("start", start->options[1].nextId);

    DialoguePart *second = find_part(&dialogue, "second");
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("second", second->id);
    TEST_ASSERT_EQUAL(1, second->optionsCount);
}

/* read_lua_dialogue: missing file */
void test_read_lua_dialogue_missing_file(void) {
    DialoguesError err = read_lua_dialogue(&dialogue, "tests/fixtures/nonexistent.lua",
                                           stub_context_index, stub_context_newindex);
    TEST_ASSERT_EQUAL(DIALOGUE_NOT_FOUND, err);
    /* system was not loaded (lua_close already called on failure) */
}

/* free_dialogue: no crash */
void test_free_dialogue_no_crash(void) {
    DialoguesError err = read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua",
                                           stub_context_index, stub_context_newindex);
    system_loaded = true;
    TEST_ASSERT_EQUAL(DIALOGUE_OK, err);

    free_dialogue(&dialogue);
    system_loaded = false;
}

/* find_part: found */
void test_find_part_found(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;

    DialoguePart *part = find_part(&dialogue, "second");
    TEST_ASSERT_NOT_NULL(part);
    TEST_ASSERT_EQUAL_STRING("second", part->id);
}

/* find_part: not found returns NULL */
void test_find_part_not_found(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;

    TEST_ASSERT_NULL(find_part(&dialogue, "nonexistent"));
    TEST_ASSERT_NOT_NULL(find_part(&dialogue, "start"));
}

/* condition_met: no condition (LUA_NOREF) returns true */
void test_condition_met_no_condition(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "start");
    TEST_ASSERT_NOT_NULL(part);
    bool result = condition_met(&dialogue, &part->options[0], NULL);
    TEST_ASSERT_TRUE(result);
}

/* condition_met: with condition true/false */
void test_condition_met_with_condition(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/with_condition_effect.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "with_condition");
    TEST_ASSERT_NOT_NULL(part);
    /* options[0] = "Always show" (no condition), [1] = "Condition true", [2] = "Condition false" */
    TEST_ASSERT_TRUE(condition_met(&dialogue, &part->options[0], NULL));
    TEST_ASSERT_TRUE(condition_met(&dialogue, &part->options[1], NULL));
    TEST_ASSERT_FALSE(condition_met(&dialogue, &part->options[2], NULL));
}

/* invoke_effect: no effect (LUA_NOREF) does not crash */
void test_invoke_effect_no_effect(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/simple.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "start");
    TEST_ASSERT_NOT_NULL(part);
    invoke_effect(&dialogue, &part->options[0], NULL);
}

/* invoke_effect: with effect does not crash */
void test_invoke_effect_with_effect(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/with_condition_effect.lua", stub_context_index,
                      stub_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "with_effect");
    TEST_ASSERT_NOT_NULL(part);
    invoke_effect(&dialogue, &part->options[1], NULL);
}

/* condition_met: condition receives context and returns context.conditionShouldPass */
void test_condition_met_receives_context(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/with_condition_effect.lua", test_context_index,
                      test_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "with_condition");
    TEST_ASSERT_NOT_NULL(part);

    /* Option "Condition sets flag" is index 3 (options[3]); returns context.conditionShouldPass */
    TestContext ctx_pass = {.effectCalled = false, .conditionShouldPass = true};
    bool result_pass = condition_met(&dialogue, &part->options[3], &ctx_pass);
    TEST_ASSERT_TRUE(result_pass);

    TestContext ctx_fail = {.effectCalled = false, .conditionShouldPass = false};
    bool result_fail = condition_met(&dialogue, &part->options[3], &ctx_fail);
    TEST_ASSERT_FALSE(result_fail);
}

/* invoke_effect: effect receives context and can set flags */
void test_invoke_effect_receives_context(void) {
    read_lua_dialogue(&dialogue, "tests/fixtures/with_condition_effect.lua", test_context_index,
                      test_context_newindex);
    system_loaded = true;
    DialoguePart *part = find_part(&dialogue, "with_effect");
    TEST_ASSERT_NOT_NULL(part);
    TestContext ctx = {.effectCalled = false};

    invoke_effect(&dialogue, &part->options[1], &ctx);

    TEST_ASSERT_TRUE(ctx.effectCalled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_lua_dialogue_success);
    RUN_TEST(test_read_lua_dialogue_missing_file);
    RUN_TEST(test_free_dialogue_no_crash);
    RUN_TEST(test_find_part_found);
    RUN_TEST(test_find_part_not_found);
    RUN_TEST(test_condition_met_no_condition);
    RUN_TEST(test_condition_met_with_condition);
    RUN_TEST(test_invoke_effect_no_effect);
    RUN_TEST(test_invoke_effect_with_effect);
    RUN_TEST(test_condition_met_receives_context);
    RUN_TEST(test_invoke_effect_receives_context);
    return UNITY_END();
}
