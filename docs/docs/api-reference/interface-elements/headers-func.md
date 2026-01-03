---
sidebar_position: 55
---

# headers

Family of functions `h1`, `h2`, `h3`, `h4`, `h5` and `h6` to display the 6 different levels of heading.

### Function Signature

```julia
# same signature for h1, h2, h3, h4, h5 and h6

function h1(
    text        ::String;
    icon        ::String = "",
    icon_color  ::String = "",
    css         ::Dict   = Dict()
)::Nothing
```

 Argument        | Description
------------------ | -----------
 `text`         | A `String` containing the heading text. It can contain HTML.
 `icon`    | A `String` in the format `material/icon_name`. Example: `material/thumb_up`.<br/><br/>Check out https://fonts.google.com/icons?icon.set=Material+Icons to learn the icon names.
 `icon_color`   | An optional `String` specifying the color of the icon using a CSS color value.
 `css`          | A `Dict` of additional CSS properties applied to the heading element.

### Return Value

Nothing.
