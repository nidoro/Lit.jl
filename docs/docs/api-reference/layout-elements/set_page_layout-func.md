---
sidebar_position: 1
---

# set_page_layout

Configure the overall page layout.

This function defines the global layout of the page, including the main content
area and optional left and right sidebars. The containers corresponding to each
of these regions can be accessed via the API functions `main_area()`,
`left_sidebar()` and `right_sidebar()`, and they can be used like any other
function that returns a `ContainerInterface`. See usage example below.

### Function Signature

```julia
function set_page_layout(
    inner_func                    ::Function               =()->();
    style                         ::String                 ="basic",
    max_width                     ::String                 ="600px",

    left_sidebar_initial_state    ::Union{Nothing, String} =nothing,
    left_sidebar_initial_width    ::String                 ="300px",
    left_sidebar_position         ::String                 ="slide-out",
    left_sidebar_toggle_labels    ::Tuple{Union{String, Nothing}, Union{String, Nothing}} =(nothing, nothing),

    right_sidebar_initial_state   ::Union{Nothing, String} =nothing,
    right_sidebar_initial_width   ::String                 ="300px",
    right_sidebar_position        ::String                 ="slide-out",
    right_sidebar_toggle_labels   ::Tuple{Union{String, Nothing}, Union{String, Nothing}} =(nothing, nothing),
)::Containers
```

 Argument                              | Description
------------------------------------ |-------------
 `inner_func`                      | An optional do-block `Function`, so you can define the page main area and its children like this: <pre>set_page_layout("centered") do<br/>  # main area content<br/>end</pre> which is basically the same as: <pre>@push set_page_layout("centered")<br/># main area content<br/>@pop</pre>In both cases, elements created inside the `do-end`/`push-pop` blocks will be placed inside the container returned by `main_area()`, with the difference that `push-pop` does not define a new scope, and thus variables created inside that block can be accessed after `@pop`.
 `style`                           | A `String` for selecting the overall layout style. The default `"basic"` style imposes minimal layout behaviour on the `main_area()` container.
 `max_width`                      | A `String` specifying the maximum width of the container returned by `main_area()`.
 `left_sidebar_initial_state`   | Either `nothing` or a `String` specifying the initial state of the left sidebar (`"open"` or `"closed"`). If `nothing`, the sidebar is disabled.
 `left_sidebar_initial_width`   | A `String` specifying the initial width of the left sidebar.
 `left_sidebar_position`         | A `String` specifying how the left sidebar is positioned relative to the main content (`"slide-out"` or `"overlay"`).
 `left_sidebar_toggle_labels`   | A `Tuple` of two optional `String` values defining the labels for opening and closing the left sidebar toggle button.
 `right_sidebar_initial_state`   | Either `nothing` or a `String` specifying the initial state of the right sidebar (`"open"` or `"closed"`). If `nothing`, the sidebar is disabled.
 `right_sidebar_initial_width`   | A `String` specifying the initial width of the right sidebar.
 `right_sidebar_position`         | A `String` specifying how the right sidebar is positioned relative to the main content (`"slide-out"` or `"overlay"`).
 `right_sidebar_toggle_labels`   | A `Tuple` of two optional `String` values defining the labels for opening and closing the right sidebar toggle button.

### Return Value

A `Containers` object representing the layout regions created by the page
layout.

### Usage

The `Container`s created by `set_page_layout()` can be accessed via the API
functions:

- `main_area()`
- `left_sidebar()`
- `right_sidebar()`

After calling `set_page_layout()` you can insert elements inside these regions
using the functions above. Example:

```julia
set_page_layout("centered", left_sidebar_initial_state="open")

main_area() do # Optional call to main_area()
    # main area content
end

left_sidebar() do
    # left sidebar content
end
```

> **TIP**: In general, you don't have to explicitly call `main_area()` to place
> elements into the main area. After calling `set_page_layout()`, any element
> created in the top-level of your app is placed inside `main_area()`.
