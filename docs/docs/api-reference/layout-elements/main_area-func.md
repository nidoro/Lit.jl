---
sidebar_position: 27
---

# main_area

Returns the `Container` corresponding to the page main area created by
`set_page_layout()`.

See `set_page_layout()` for more information.

### Function Signature

```julia
function main_area(inner_func::Function=()->())::ContainerInterface
```

 Argument                              | Description
------------------------------------ |-------------
 `inner_func`                      | An optional do-block `Function`, so you can define the page main area and its children like this: <pre>main_area() do<br/>  # main area content<br/>end</pre> which is basically the same as: <pre>@push main_area()<br/># main area content<br/>@pop</pre>In both cases, elements created inside the `do-end`/`push-pop` blocks will be placed inside the container returned by `main_area()`, with the difference that `push-pop` does not define a new scope, and thus variables created inside that block can be accessed after `@pop`.

### Return Value

The `ContainerInterface` of the page's main area.

### Usage

After calling `set_page_layout()` you can insert elements inside the main area
using `main_area()`. Example:

```julia
set_page_layout("centered")

main_area() do
    # main area content
end
```
