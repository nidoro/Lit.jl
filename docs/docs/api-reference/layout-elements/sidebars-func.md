---
sidebar_position: 35
---

# Sidebars

Pair of functions to retrieve the left and right sidebar `ContainerInterface`s:

- `left_sidebar()`
- `right_sidebar()`

The sidebars are disabled by default. In order to enable them, you must set
their initial state by calling the function `set_page_layout()`. Example:

```julia
set_page_layout("centered", left_sidebar_initial_state="open")
```

### Function Signature

```julia
function left_sidebar (inner_func::Function=()->())::ContainerInterface
function right_sidebar(inner_func::Function=()->())::ContainerInterface
```

 Argument                              | Description
------------------------------------ |-------------
 `inner_func`                      | An optional do-block `Function`, so you can add content to the sidebar like this: <pre>left_sidebar() do<br/>  # sidebar content<br/>end</pre> which is basically the same as: <pre>@push left_sidebar()<br/># sidebar content<br/>@pop</pre>In both cases, elements created inside the `do-end`/`push-pop` blocks will be placed inside the container returned by `left_sidebar()` or `right_sidebar()`, with the difference that `push-pop` does not define a new scope, and thus variables created inside that block can be accessed after `@pop`.

### Return Value

The `ContainerInterface` of the sidebar.

### Usage

After setting the sidebar initial state using `set_page_layout()` you can insert
inside the sidebars using `left_sidebar()` or `right_sidebar()`. Example:

```julia
set_page_layout(
    "centered",
    left_sidebar_initial_state="open",
    right_sidebar_initial_state="closed"
)

left_sidebar() do
    # left sidebar content
end

right_sidebar() do
    # right sidebar content
end
```
