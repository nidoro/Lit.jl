---
sidebar_position: 55
---

# is_on_page

Checks if the current page is the page associated with a given path.

## Function Signature

```julia
function is_on_page(path::String)::Bool
```

Argument        | Description
---------------- |-------------
 `path`        | `String`. A path associated with a page of the app.

## Return Value

`true` if the current page is the page associated with `path`.

The current page is the page associated with the URL path returned by
`get_url_path()`. `is_on_page()` will return `true` if `path` is any of the
paths associated with the current page, regardless of which URL path the user
used to reach the page.

## Example

The code below defines two pages, the first one being associated with two paths
(`/` and `/page-1`):

```julia
@app_startup begin
    add_page(["/", "/page-1"])
    add_page("/page-2")
end

if     is_on_page("/")        # if the current URL path is either `/` or
    include("page-1.jl")      # `/page-1`, this will return true.
elseif is_on_page("/page-2")
    include("page-2.jl")
end
```

If the user accessed the page via `/page-1`, `is_on_page("/")` will return
`true`, because the current page is in fact associated with both `/` and
`/page-1`.
