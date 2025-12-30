---
sidebar_position: 3
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
`is_session_first_pass()` and running the `@session_startup` code block only if
it returns `true`.

## Usage

You can define multiple `@session_startup` code blocks, but we recommend you to
keep all of your session initialization logic inside a single `@session_startup`
block after the page initialization logic.

Although sessions are not required to have `@session_startup` code blocks, some
initialization tasks should be only performed inside `@session_startup`
code blocks. See below what you are expected to do inside `@session_startup`
code blocks.

### 1. Initialization of session persistent data

Session persistent data is an user defined data that is bound to a session and
whose lifetime is the lifetime of the session, i.e., as long as the session
stays active the data will persist. Session persistent data can be retrieved at
any moment using `get_session_data()` and is only visible to the current
session.

Although the session persistent data can be either mutable or immutable, a
common practice is to define a mutable struct to store all of your sessions's
data and store it with `set_session_data()` at the session startup. Example:

```julia
# Define the struct to hold the session persistent data
mutable struct SessionData
    foo::String
    bar::Int
end

@session_startup begin
    # Initialize the session persistent data
    session = SessionData("hello", 32)
    set_session_data(session)
end

# now `get_session_data()` can be called from anywhere
# to retrieve the current session data
```

In the example above, since the persistent data is a mutable struct, every time
`get_session_data()` is called the same `SessionData` object instance stored with
`set_session_data()` at the session startup is returned, and thus you can modify its
members without having to call `set_session_data()` ever again.

## See also

- `@app_startup`
- `@page_startup`

