---
sidebar_position: 15
---

# @session_startup

Macro to define a code block that should only executed at a session's first run.
Usage:

```julia
@session_startup begin
    # session initialization logic
end
```

Internally, this macro is implemented by checking the result of
[`is_session_first_pass()`](/docs/build/docs/api-reference/application-logic/is_first_pass-func)
and running the `@session_startup` code block only if it returns `true`.

## Usage

You can define multiple `@session_startup` code blocks, but we recommend you to
keep all of your session initialization logic inside a single `@session_startup`
block after the page initialization logic.

Although sessions are not required to have `@session_startup` code blocks, some
initialization tasks should be only performed inside `@session_startup`
code blocks. See below what you are expected to do inside `@session_startup`
code blocks.

### 1. Initialization of session persistent data

Session persistent data is a user defined data that is bound to a session and
whose lifetime is the lifetime of the session, i.e., as long as the session is
active the data will persist. Session persistent data can be retrieved at any
moment using `get_page_data()`.

You can store data that you want to be available within a session via the
`set_session_data()` function, and retrieve it using the `get_session_data()`
function. See [Session persistent data](/docs/build/docs/api-reference/application-logic/session-persistent-data)
to learn more.

## See also

- [`@app_startup`](/docs/build/docs/api-reference/application-logic/app_startup-macro)
- [`@page_startup`](/docs/build/docs/api-reference/application-logic/page_startup-macro)

