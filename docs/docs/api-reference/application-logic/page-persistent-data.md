---
sidebar_position: 40
---

# Page persistent data

Page persistent data is an user defined data that is bound to a page and
whose lifetime is the lifetime of the app, i.e., as long as the app is running
the data will persist. Page persistent data can be retrieved at any moment using
`get_page_data()` and is shared accross sessions.

Although the page persistent data can be either mutable or immutable, a common
practice is to define a mutable struct to store all of your page's data and
store it with `set_page_data()` at the page startup. Example:

```julia
# Define the struct to hold the page persistent data
mutable struct PageData
    foo::String
    bar::Int
end

@page_startup begin
    # Initialize the page persistent data
    page = PageData("hello", 32)
    set_page_data(page)
end

# now `get_page_data()` can be called from anywhere to retrieve the page data
```

In the example above, since the persistent data is a mutable struct, every time
`get_page_data()` is called the same `PageData` object instance stored with
`set_page_data()` at the page startup is returned, and thus you can modify its
members without having to call `set_page_data()` ever again.

## `set_page_data()`

Stores page persistent data.

### Function Signature

```julia
function set_page_data(page_data::Any)::Nothing
```

 Argument  | Description
---------- |-------------
 `page_data` | Data of `Any` type.

## `get_page_data()`

Retrieves the previously stored page persistent data.

### Function Signature

```julia
function get_page_data()::Any
```



