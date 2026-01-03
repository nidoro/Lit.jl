---
sidebar_position: 50
---

# text

Display a text.

### Function Signature

```julia
function text(text::Any)::Nothing
```

 Argument    | Description
------------------ | -----------
 `text`     | The content to be displayed. If the value is a `String`, it is rendered as-is. Otherwise, its string representation is obtained using `repr()`.

### Return Value

Nothing.
