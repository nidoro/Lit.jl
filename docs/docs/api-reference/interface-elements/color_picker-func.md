---
sidebar_position: 40
---

# color_picker

Display a color picker input widget.

### Function Signature

```julia
function color_picker(
    label           ::String;
    initial_value   ::Union{String, Nothing}=nothing,
    id              ::Any      =nothing,
    show_label      ::Bool     =true,
    fill_width      ::Bool     =false,
    onchange        ::Function =(args...; kwargs...)->(),
    css             ::Dict     =Dict()
)::String
```

 Argument        | Description
---------------- |-------------
 `label`       | A `String` used as the label for the color picker.
 `initial_value`    | Either a `String` specifying the initial hexadecimal color value of the input, or `nothing` (default). If `nothing` is provided, the initial value will be the default value previously set with `set_default_value()` if any; otherwise, the widget will be initialized with a grey color.
 `id`          | An optional identifier for the color picker. If provided, it is used to uniquely identify the widget so you can reference it in other functions, like `get_value()` and `set_value()`.
 `show_label`   | A `Bool` indicating whether the label should be displayed. Default: `true`.
 `fill_width`   | A `Bool` indicating whether the select box should expand to fill the available horizontal space. Default: `false`.
 `onchange`     | A callback `Function`. This function is called when the selected color changes, before the app script is rerun.
 `css`         | A `Dict` of additional CSS properties to apply to the color picker widget.

### Return Value

Returns the currently selected color value as a `String` in a hexadecimal format
such as `"#ff0000"`.
