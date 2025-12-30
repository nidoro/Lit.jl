---
sidebar_position: 5
---

# columns

Insert multiple column containers side by side.

This function is a convenience wrapper for creating a horizontal layout with
multiple columns at once. It returns a collection of containers that can be
used to insert child elements independently into each column.

### Function Signature

```julia
function columns(
    amount_or_widths ::Union{Int, Vector};
    kwargs...
)::Containers
```

 Argument              | Description
--------------------- |-------------
 `amount_or_widths` | Either an `Int`, specifying the number of equally sized columns to create, or a `Vector{Number}` specifying the width of each column relative to eachother.<br/><br/>For `Vector{Number}`, the width of a column is calculated based on the available space on the parent container and the proportion of its width relative to the sum of all relative widths. For instance, `columns([70,30])` will return two columns: the first one with width `70/100 = 0.7`, taking 70% of the available space, and the second with width `30/100 = 0.3`, taking 30% of the available space.
 `kwargs`              | Keyword arguments forwarded to each individual column container. These correspond to the keyword arguments accepted by `column()`, such as alignment, spacing, borders, CSS, and attributes.

### Return Value

A `Containers` object representing the created columns. Each element of the
returned collection is a `ContainerInterface` corresponding to one column.

### Usage

You can define `do-end` blocks for each column like this:

```julia
cols = columns(3)

cols(1) do
    button("Left")
end

cols(2) do
    button("Center")
end

cols(3) do
    button("Right")
end
```

Alternativelly, you can call element creation functions directly from the
returned columns like this:

```julia
cols = columns(3)
cols[1].button("Left")
cols[2].button("Center")
cols[3].button("Right")
```
