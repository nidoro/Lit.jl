---
sidebar_position: 7
---

# container

Insert a generic container.

`container()` imposes minimal layout behavior by default. It can be styled
and customized using CSS properties and HTML attributes, and can be used as a
building block for more specialized layouts. `column()` and `row()` are
specializations of `container()`.

### Function Signature

```julia
function container(
    inner_func  ::Function =()->();
    css         ::Dict     =Dict(),
    attributes  ::Dict     =Dict()
)::ContainerInterface
```

 Argument        | Description
---------------- |-------------
 `inner_func`   | An optional do-block `Function`, so you can define a container and its children like this: <pre>container() do<br/>  # children here<br/>end</pre> which is basically the same as: <pre>@push container()<br/># children here<br/>@pop</pre>In both cases, elements created inside the `do-end`/`push-pop` blocks will be placed inside the container returned by `container()`, with the difference that `push-pop` does not define a new scope, and thus variables created inside that block can be accessed after `@pop`.
 `css`          | A `Dict` of CSS properties to apply to the container.
 `attributes`   | A `Dict` of additional HTML attributes to attach to the container.

### Return Value

A `ContainerInterface` representing the container.

### Usage

Child elements can be inserted
either by using the methods shown in the description of the `inner_func` argument
or by calling element creation functions directly from the returned
`ContainerInterface`. Example:

```julia
my_container = container()
my_container.button("Button inside my_container")
```

which is equivalent to:

```julia
my_container = container()
my_container() do
    button("Button inside my_container")
end
```
