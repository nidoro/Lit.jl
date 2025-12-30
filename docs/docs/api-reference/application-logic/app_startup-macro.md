---
sidebar_position: 1
---

# @app_startup

Macro to define a code block that should only be executed at the startup of the
application (app dry-run). Usage:

```julia
@app_startup begin
    # app initialization logic
end
```

Internally, this macro is implemented by checking the result of
`is_app_first_pass()` and running the `@app_startup` code block only if it
returns `true`.

## Usage

You can define multiple `@app_startup` code blocks, but we recommend you to keep
all of your app initialization logic inside a single `@app_startup` block near
the top of your entry-point script (`app.jl` by default).

Although apps are not required to have `@app_startup` code blocks, some
initialization tasks should be only performed inside `@app_startup`
code blocks. See below what you are expected to do inside `@app_startup` code blocks.

### 1. Definition of the app pages

> **🛈 NOTE**: This is only needed if your app has multiple pages.

```julia
@app_startup begin
    add_page("/first-page")
    add_page("/second-page")
    add_page("/third-page")
end
```

See `add_page()` for more information.

### 2. Initialization of app persistent data

App persistent data is an user defined data whose lifetime is the lifetime of
the app, i.e., as long as the app is running the data will persist. App
persistent data can be retrieved at any moment using `get_app_data()` and is
shared accross sessions.

Although the app persistent data can be either mutable or immutable, a common
practice is to define a mutable struct to store all of your app's data and store
it with `set_app_data()` at the application startup. Example:

```julia
# Define the struct to hold the app persistent data
mutable struct AppData
    foo::String
    bar::Int
end

@app_startup begin
    # Initialize the app persistent data
    app = AppData("hello", 32)
    set_app_data(app)
end

# now `get_app_data()` can be called from anywhere to retrieve the app data
```

In the example above, since the persistent data is a mutable struct, every time
`get_app_data()` is called the same `AppData` object instance stored with
`set_app_data()` at the app startup is returned, and thus you can modify its
members without having to call `set_app_data()` ever again.

## See also

- `@page_startup`
- `@session_startup`

