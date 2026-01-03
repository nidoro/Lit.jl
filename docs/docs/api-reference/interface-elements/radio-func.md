---
sidebar_position: 20
---

# radio

Display a radio button group widget.

### Function Signature

```julia
function radio(
    label          ::String,
    options        ::Vector;
    id             ::Any = nothing,
    initial_value  ::Union{String, Nothing}=nothing
)::Union{String, Nothing}
```

 Argument           | Description
------------------  | -----------
 `label`            | A `String` to be displayed as the label for the radio button group. It can contain HTML.
 `options`          | A `Vector` of selectable values. Each element represents one radio option and will be displayed using its string representation.
 `id`              | An optional identifier for the checkbox. If provided, it is used to uniquely identify the widget so you can reference it in other functions, like `get_value()` and `set_value()`.
 `initial_value`    | The value that should be initially selected. If provided, it should match one of the values in `options`. If `nothing`, the default value set with `set_default_value()` will be selected if one was provided. Otherwise, first option in `options` will be selected.

### Return Value

The currently selected value from `options`, or `nothing` if no option is selected.
