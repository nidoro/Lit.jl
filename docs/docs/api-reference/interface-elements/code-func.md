---
sidebar_position: 65
---

# code

Display a code block.

### Function Signature

```julia
function code(
    initial_value      ::String   = "";
    initial_value_file ::Union{String, Nothing} = nothing,
    fill_width         ::Bool     = true,
    max_width          ::String   = "100%",
    max_height         ::String   = "initial",
    padding            ::String   = "0",
    strip_whitespace   ::Bool     = true,
    css                ::Dict     = Dict("overflow-y" => "auto")
)::String
```

 Argument               | Description
---------------------- | -----------
 `initial_value`        | A `String` containing the initial code content to display.
 `initial_value_file`   | An optional path to a file whose contents will be loaded as the initial code value. If provided, it takes precedence over `initial_value`.
 `fill_width`           | A `Bool` indicating whether the code block should expand to fill the available horizontal space. Default: `true`.
 `max_width`            | A `String` specifying the maximum width of the code block using a CSS value (for example, `"100%"` or `"800px"`).
 `max_height`           | A `String` specifying the maximum height of the code block using a CSS value. If the content exceeds this height, it becomes scrollable.
 `padding`              | A `String` specifying the padding applied inside the code block using a CSS value.
 `strip_whitespace`     | A `Bool` indicating whether leading and trailing whitespace should be removed from the initial code content. Default: `true`.
 `css`                  | A `Dict` of additional CSS properties applied to the code block.

### Return Value

A `String` containing the current code content.
