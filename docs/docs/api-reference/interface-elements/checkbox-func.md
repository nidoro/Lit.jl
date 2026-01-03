---
sidebar_position: 10
---

# checkbox

Display a checkbox widget.

### Function Signature

```julia
function checkbox(
    label         ::String;
    id            ::Any       = nothing,
    initial_value ::Union{Bool, Nothing}=nothing,
    onchange      ::Function  = (args...; kwargs...)->(),
    args          ::Vector    = Vector()
)::Bool
```

 Argument          | Description
------------------ | -----------
 `label`           | A `String` to be displayed next to the checkbox. It can contain HTML.
 `id`              | An optional identifier for the checkbox. If provided, it is used to uniquely identify the widget so you can reference it in other functions, like `get_value()` and `set_value()`.
 `initial_value`   | The checkbox initial value. If `nothing`, the default value set with `set_default_value()` will be used, if any; otherwise, the initial value will be `false`.
 `onchange`        | A callback `Function`. This function is called when the checkbox value changes, before the app script is rerun.
 `args`            | A `Vector` of arguments that will be passed to the `onchange` callback function.

### Return Value

The current value of the checkbox (`true` if checked, `false` otherwise).
