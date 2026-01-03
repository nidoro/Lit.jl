---
sidebar_position: 45
---

# Session persistent data

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

## `set_session_data()`

Stores session persistent data.

### Function Signature

```julia
function set_session_data(session_data::Any)::Nothing
```

 Argument  | Description
---------- |-------------
 `session_data` | Data of `Any` type.

## `get_session_data()`

Retrieves the previously stored session persistent data.

### Function Signature

```julia
function get_session_data()::Any
```



