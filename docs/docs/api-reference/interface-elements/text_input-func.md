---
sidebar_position: 25
---

# text_input

Display a text input widget.

### Function Signature

```julia
function text_input(
    label          ::String;
    id             ::Any      = nothing,
    show_label     ::Bool     = true,
    fill_width     ::Bool     = false,
    initial_value  ::Union{String, Nothing}=nothing,
    placeholder    ::Union{String, Nothing}=nothing,
    css            ::Dict     = Dict()
)::Union{String, Nothing}
```

 Argument           | Description
------------------ | -----------
 `label`            | A `String` to be displayed as the label for the text input. It can contain HTML.
 `id`              | An optional identifier for the checkbox. If provided, it is used to uniquely identify the widget so you can reference it in other functions, like `get_value()` and `set_value()`.
 `show_label`       | A `Bool` indicating whether the label should be displayed. Default: `true`.
 `fill_width`       | A `Bool` indicating whether the text input should expand to fill the available horizontal space. Default: `false`.
 `initial_value`    | Either a `String` specifying the initial text value of the input, or `nothing` (default). If `nothing` is provided, the initial value will be the default value previously set with `set_default_value()` if any; otherwise, the widget will be initialized with value `nothing`.
 `placeholder`      | A `String` shown as placeholder text when the widget's value is `nothing`.
 `css`               | A `Dict` of additional CSS properties applied to the text input element.

### Return Value

The current value of the text input as a `String` or `nothing`.
