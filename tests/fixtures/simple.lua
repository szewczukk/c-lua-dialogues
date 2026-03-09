-- Minimal valid dialogue for tests.
-- Script must return a table of parts; each part has id, text, options (array of {text, next?, effect?, condition?}).
return {
  {
    id = "start",
    text = "Hello. Choose an option.",
    options = {
      { text = "Go to second", next = "second" },
      { text = "Stay", next = "start" }
    }
  },
  {
    id = "second",
    text = "You are at the second part.",
    options = {
      { text = "Back to start", next = "start" }
    }
  }
}
