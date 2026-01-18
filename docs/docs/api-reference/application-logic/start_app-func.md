---
sidebar_position: 0
---

# start_app

Start the application server.

Call this function from the REPL to start the web app. Example:

```julia
> using Magic
> start_app("my-app.jl")
```

### Function Signature

```julia
function start_app(
    script_path ::String ="app.jl";
    host_name   ::String ="localhost",
    port        ::Int    =3443,
    docs_path   ::Union{String, Nothing}=nothing,
    dev_mode    ::Bool   =false
)::Nothing
```

 Argument        | Description
---------------- |-------------
 `script_path` | A `String` specifying the path to the application script to run.
 `host_name`   | A `String` specifying the hostname or IP address the server should bind to. Default is `"localhost"`.
 `port`        | An `Int` specifying the port number on which the server will listen. Default is `3443`.
 `docs_path`   | A `String` specifying a path to Magic's docs where it has been built, or `nothing` (default). If a `String` is passed, the docs will be served under `/docs`.
 `dev_mode`    | A `Bool`. If `true`, development mode is enabled. This activates features such as more verbose error reporting and loading of locally built `libmagic.so`.

### Return Value

Returns `nothing`.

Once called, this function blocks the current process and keeps the server
running until it is stopped with `Ctrl+C`.
