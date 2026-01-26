---
sidebar_position: 50
---

# gen_serveable_path

Generates a file path where you can save a file to be served in your app.

## When to use this function

### 1. Generate a serveable path

Some widgets serve files to the client, such as
[`image`](/docs/build/docs/api-reference/interface-elements/image-func),
which serves an image in a given system path. Such widgets can only serve
resources that live inside your project's `.Magic/served-files/` directory and
subdirectories.

`gen_serveable_path()` is a convenience function to be used when you want
don't care for the name of the file you want to serve, nor where it lives, as
long as it is "serveable", i.e. it lives somewhere inside `.Magic/served-files/`.
`gen_serveable_path()` generates a path with a random file name with a given
extension inside `.Magic/served-files/generated`. You are then supposed to save your
file in the returned path and pass this path to the wiget you want.

For instance, consider an app that displays a plot given some user input. The
app's logic will look something like this:

```julia
plot_object = generate_fancy_plot(...)       # 1. Generate the plot
serveable_path = gen_serveable_path("png")
save_fancy_plot(plot_object, serveable_path) # 2. Save the plot
image(plot_path)                             # 3. Serve the plot
```

### 2. Avoid Browser Caching

Aside from conveniently giving you a serveable file path, `gen_serveable_path()`
also helps to avoid browser caching issues. For instance, in the above example,
suppose that instead of saving the plot in a random serveable path everytime the
plot is regenerated we always saved it in the same serveable path. Most browsers
will automatically cache the image associated with a path, so newly generated
images in the same path would never be requested by these browsers.
Serving the newly generated plot in the path returned by `gen_serveable_path()`
prevents that from happening.

### Function Signature

```julia
function gen_serveable_path(extension::String; lifetime::String="session")::String
```

 Argument  | Description
---------- |-------------
 `extension` | File extension `String` to be appended to the randomly generated path.
 `lifetime` | A `String` indicating the lifetime of the resource. Possible values: `"session"` (default) or `"app"`. If `"session"` is provided, the file will become unavailable after the session is ended. If `"app"` is provided, the file will become unavailable after the app is stopped.

### Return Value

A random file path inside `.Magic/served-files/generated/` with extension
`extension`.
