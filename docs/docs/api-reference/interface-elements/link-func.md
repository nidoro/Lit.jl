---
sidebar_position: 35
---

# link

Display a link styled as a button.

### Function Signature

```julia
function link(
    label      ::String,
    url        ::String;
    style      ::String = "secondary",
    fill_width ::Bool   = false,
    new_tab    ::Bool   = false,
    css        ::Dict   = Dict()
)::Nothing
```

 Argument        | Description
------------------ | -----------
 `label`        | A `String` to be displayed as the link text. It can contain HTML.
 `url`          | A `String` specifying the destination URL of the link.
 `style`        | A `String` defining the visual style of the link. Accepted values: `"primary"`, `"secondary"`, or `"naked"`. Default: `"secondary"`.
 `fill_width`   | A `Bool` indicating whether the link should expand to fill the available horizontal space. Default: `false`.
 `new_tab`      | A `Bool` indicating whether the link should open in a new browser tab. Default: `false`.
 `css`          | A `Dict` of CSS properties applied inline to the element.

### Return Value

Nothing.

