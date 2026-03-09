-- Dialogue with condition and effect for testing condition_met and invoke_effect.
-- Condition/effect receive the game context (lightuserdata with GameContext metatable).
-- Context can set/read: conditionShouldPass, effectCalled (for C tests to verify context is passed).
return {
  {
    id = "with_condition",
    text = "Option with condition.",
    options = {
      {
        text = "Always show",
        next = "with_condition"
        -- no condition => LUA_NOREF, condition_met returns true
      },
      {
        text = "Condition true",
        next = "with_condition",
        condition = function(context) return true end
      },
      {
        text = "Condition false",
        next = "with_condition",
        condition = function(context) return false end
      },
      {
        text = "Condition sets flag",
        next = "with_condition",
        condition = function(context)
          return context.conditionShouldPass
        end
      }
    }
  },
  {
    id = "with_effect",
    text = "Option with effect.",
    options = {
      {
        text = "No effect",
        next = "with_effect"
        -- no effect => LUA_NOREF
      },
      {
        text = "With effect",
        next = "with_effect",
        effect = function(context)
          context.effectCalled = true
        end
      }
    }
  }
}
