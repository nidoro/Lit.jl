---
sidebar_position: 5
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
[`is_app_first_pass()`](/docs/build/docs/api-reference/application-logic/is_first_pass-func)
and running the `@app_startup` code block only if it returns `true`.

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

### 2. Initialization of app persistent data

App persistent data is an user defined data whose lifetime is the lifetime of
the app, i.e., as long as the app is running the data will persist. App
persistent data can be retrieved at any moment using `get_app_data()` and is
shared accross sessions.

You can store data that you want to be globally available through all of your
app's pages and sessions via the `set_app_data()` function, and retrieve it
using the `get_app_data()` function.
See [App persistent data](/docs/build/docs/api-reference/application-logic/app-persistent-data)
to learn more.

## See also

- [`@page_startup`](/docs/build/docs/api-reference/application-logic/page_startup-macro)
- [`@session_startup`](/docs/build/docs/api-reference/application-logic/session_startup-macro)

