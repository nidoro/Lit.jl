---
sidebar_position: 2
---

# @page_startup

Macro to define a code block that should only be executed at the startup
(dry-run) of the current page being run (i.e. the page associated with the URL
path returned by `get_url_path()`). Usage:

```julia
@page_startup begin
    # page initialization logic
end
```

Internally, this macro is implemented by checking the result of
`is_page_first_pass()` and running the `@page_startup` code block only if it
returns `true`.

## Usage

You can define multiple `@page_startup` code blocks, but we recommend you to
keep all of your page initialization logic inside a single `@page_startup` block
near the top of your page script file (or, if you have all of your pages in a
single file, near the top of where the page's logic begins).

Although pages are not required to have `@page_startup` code blocks, some
initialization tasks should be only performed inside `@page_startup`
code blocks. See below what you are expected to do inside `@page_startup` code
blocks.

### 1. Page static settings

Page static settings are persistent settings that are defined at the page's
dry-run and that cannot be changed later. These include the page title,
description, extra fonts and extra styles. The static settings related functions
below can only be called inside `@page_startup` blocks.

- `set_title()`
- `set_description()`
- `add_font()`
- `add_style()`

Example:

```julia
@page_startup begin
    set_title("My Lit App")
    set_description("An awesome app built with Lit.jl")
end
```

Internally, these static settings are used to build the static HTML that is
served when a user accesses the page's URL.

### 2. Initialization of page persistent data

Page persistent data is an user defined data that is bound to a page and
whose lifetime is the lifetime of the app, i.e., as long as the app is running
the data will persist. Page persistent data can be retrieved at any moment using
`get_page_data()` and is shared accross sessions.

Although the page persistent data can be either mutable or immutable, a common
practice is to define a mutable struct to store all of your page's data and
store it with `set_page_data()` at the page startup. Example:

```julia
# Define the struct to hold the page persistent data
mutable struct PageData
    foo::String
    bar::Int
end

@page_startup begin
    # Initialize the page persistent data
    page = PageData("hello", 32)
    set_page_data(page)
end

# now `get_page_data()` can be called from anywhere to retrieve the page data
```

In the example above, since the persistent data is a mutable struct, every time
`get_page_data()` is called the same `PageData` object instance stored with
`set_page_data()` at the page startup is returned, and thus you can modify its
members without having to call `set_page_data()` ever again.

## See also

- `@app_startup`
- `@session_startup`

