---
sidebar_position: 45
---

# image

Display an image.

### Function Signature

```julia
function image(
    src_or_path ::String;
    fill_width  ::Bool                   = false,
    max_width   ::String                 = "100%",
    width       ::Union{Number, Nothing} = nothing,
    height      ::Union{Number, Nothing} = nothing,
    css         ::Dict                   = Dict("height" => "auto")
)::String
```

 Argument          | Description
------------------ | -----------
 `src_or_path`    | A `String` representing either a URL or a local file path to the image source.<br/><br/>Only images inside `.Magic/served-files` and subdirectories can be served. If it is a static image that does not change across sessions, a good practice is to place it inside `.Magic/served-files/static/images`. If it is a generated image, e.g. a plot that changes across app reruns, a good practice is to place it inside `.Magic/served-files/cache`.<br/><br/>For the common situation of regenerating and serving a new image on each rerun, there is a helper function `gen_serveable_path(ext)` that generates a file path with a random name and with the given extension `ext` inside `.Magic/served-files/cache`. This function returns the path that you should use to save your image and then pass to `image()` to place it in the app.
 `fill_width`     | A `Bool` indicating whether the image should expand to fill the available horizontal space. Default: `false`.
 `max_width`      | A `String` specifying the maximum width of the image using a CSS value (for example, `"100%"` or `"600px"`). Default: `"100%"`.
 `width`          | An optional numeric width to be used as the `width` attribute of the `img` tag.
 `height`         | An optional numeric height to be used as the `height` attribute of the `img` tag.
 `css`            | A `Dict` of additional CSS properties applied to the `img` tag. By default, the height is set to `"auto"`.

### Return Value

A `String` containing the rendered HTML for the image.
