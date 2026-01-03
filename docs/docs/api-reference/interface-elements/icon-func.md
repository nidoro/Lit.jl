---
sidebar_position: 70
---

# icon

Display an icon.

### Function Signature

```julia
function icon(
    icon   ::String;
    color  ::String = "inherit",
    size   ::String = "inherit",
    weight ::String = "inherit"
)::Nothing
```

 Argument    | Description
------------------ | -----------
 `icon`    | A `String` in the format `material/icon_name`. Example: `material/thumb_up`.<br/><br/>Check out https://fonts.google.com/icons?icon.set=Material+Icons to learn the icon names.
 `color`    | A `String` specifying the icon color using a CSS value. Default: `"inherit"`.
 `size`     | A `String` specifying the icon size using a CSS value. Default: `"inherit"`.
 `weight`   | A `String` specifying the icon weight or thickness, depending on the icon set. Default: `"inherit"`.

### Return Value

Nothing.
