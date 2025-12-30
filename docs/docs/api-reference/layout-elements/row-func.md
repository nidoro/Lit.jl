---
sidebar_position: 4
---

# row

Insert a container that stacks its children horizontally.

See also `column()`, which returns a container that stacks its children
vertically.

### Function Signature

```julia
function row(
    inner_func         ::Function   =()->();
    fill_width         ::Bool       =false,
    fill_height        ::Bool       =false,
    align_items        ::String     ="flex-start",
    justify_content    ::String     ="flex-start",
    gap                ::String     ="0.8rem",
    margin             ::String     ="0",
    css                ::Dict       =Dict()
)::ContainerInterface
```

 Argument        | Description
---------------- |-------------
 `inner_func`        | An optional do-block `Function`, so you can define a row and its children like this: <pre>row() do<br/>  # children here<br/>end</pre> which is basically the same as: <pre>@push row()<br/># children here<br/>@pop</pre>In both cases, elements created inside the `do-end`/`push-pop` blocks will be placed inside the row returned by `row()`, with the difference that `push-pop` does not define a new scope, and thus variables created inside that block can be accessed after `@pop`.
 `fill_width`        | A `Bool`. If `true`, the row expands to fill the available horizontal space.
 `fill_height`       | A `Bool`. If `true`, the row expands to fill the available vertical space.
 `align_items`       | A `String` controlling vertical alignment of children. Corresponds to the CSS `align-items` property. Example values: `flex-start`, `center`, `flex-end`.
 `justify_content`   | A `String` controlling horizontal alignment of children. Corresponds to the CSS `justify-content` property. Example values: `flex-start`, `center`, `space-between`.
 `margin`            | A `String` defining the CSS margin of the row. Corresponds to the CSS `margin` property.
 `gap`               | A `String` specifying the spacing between child elements. Corresponds to the CSS `gap` property.
 `css`               | A `Dict` of additional CSS properties to apply to the row.

### Return Value

A `ContainerInterface` representing the container.

### Usage

Child elements can be inserted
either by using the methods shown in the description of the `inner_func` argument
or by calling element creation functions directly from the returned
`ContainerInterface`. Example:

```julia
my_row = row()
my_row.button("Button inside my_row")
```
which is equivalent to:
```julia
my_row = row()
my_row() do
    button("Button inside my_row")
end
```

