---
sidebar_position: 80
---

# html

Render a raw HTML element.

### Function Signature

```julia
function html(
    tag         ::String,
    inner_html  ::String;
    attributes  ::Dict = Dict(),
    css         ::Dict = Dict()
)::Nothing
```

 Argument          | Description
------------------ | -----------
 `tag`             | A `String` specifying the HTML tag name to render (for example, `"div"`, `"span"`, or `"p"`).
 `inner_html`      | A `String` containing the raw HTML content to be placed inside the element.
 `attributes`      | A `Dict` of HTML attributes to apply to the element. Keys are attribute names and values are their corresponding values.
 `css`             | A `Dict` of CSS properties applied inline to the element.

### Return Value

Nothing.
