---
sidebar_position: 15
---

# checkboxes

Display a checkbox group widget.

### Function Signature

```julia
function checkboxes(
    label           ::String,
    options         ::Vector;
    id              ::Any       =nothing,
    initial_value   ::Union{Vector, Nothing}=nothing,
    onchange        ::Function  =(args...; kwargs...)->(),
    args            ::Vector    =Vector()
)::Vector
```

 Argument          | Description
------------------ | -----------
 `label`           | A `String` to be displayed next to the checkbox. It can contain HTML.
 `options`         | A `Vector` specifying the selectable options. One checkbox for each option will be created.
 `id`              | An optional identifier for the checkbox. If provided, it is used to uniquely identify the widget so you can reference it in other functions, like `get_value()` and `set_value()`.
 `initial_value`   | A either a `Vector` indicating which options in `options` are initially checked, or `nothing`. If `nothing` is provided, the default value set with `set_default_value()` will be used, if any; otherwise, the initial value will be an empty `Vector` `[]`.
 `onchange`        | A callback `Function`. This function is called when the checkbox value changes, before the app script is rerun.
 `args`            | A `Vector` of arguments that will be passed to the `onchange` callback function.

### Return Value

A `Vector` indicating which options in `options` are checked.
