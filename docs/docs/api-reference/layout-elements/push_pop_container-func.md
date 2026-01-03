---
sidebar_position: 30
---

# Push/Pop Container

## The `@push` and `@pop` macros

The `@push` and `@pop` macros are mechanisms through which you begin and end
blocks of container content. Every element created inside a `@push`/`@pop` block
will be placed inside the pushed container. Example:

```julia
button("Before column()")

@push column()
    # children of the column
    button("Inside column()")
@pop

button("After column()")
```

Containers can be nested. Example:

```julia
@push column()
    # children of the column
    button("Inside column()")

    @push row()
        button("Inside row()")
    @pop

    # more children of the column
    button("Also Inside column()")
@pop
```

> ⚠️ **IMPORTANT**: Make sure to pop any pushed container.

## `do-end` blocks

Another way to begin and end blocks of container content is by using `do-end`
blocks with a container. Example:

```julia
column() do
    button("Inside column()")
end
```

`do-end` blocks also work by pushing and poping the container to the container
stack, so you can also have nested `do-end` blocks. Example:

```julia
column() do
    button("Inside column()")
    row() do
        button("Inside row()")
    end
end
```

The most important difference between `do-end` blocks and `@push`/`@pop` blocks
is that a `do-end` block defines a new scope, so julia variables created inside
it will not be visible after the block ends.

## The `push_container()` function

Internally, the `@push` macro works by calling the functions `push_container()`
with the passed container.

### Function Signature

```julia
function push_container(container::ContainerInterface)::ContainerInterface
```

Argument  | Description
---------- |-------------
 `container` | A `ContainerInterface` to push to the top of the container stack.

### Return Value

The same container that was passed to it.

## The `pop_container()` function

Internally, the `@pop` macro works by calling the functions `pop_container()`
with the passed container.

### Function Signature

```julia
function pop_container()::ContainerInterface
```

### Return Value

The container on the top of the container stack.







