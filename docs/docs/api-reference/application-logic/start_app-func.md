---
sidebar_position: 0
---

# start_app

Start the application server.

Call this function from the REPL to start the web app. Example:

```julia
> using Lit
> start_app("my-app.jl")
```

### Function Signature

```julia
function start_app(
    script_path ::String ="app.jl";
    host_name   ::String ="localhost",
    port        ::Int    =3443,
    docs        ::Bool   =false,
    dev_mode    ::Bool   =false
)::Nothing
```

 Argument        | Description
---------------- |-------------
 `script_path` | A `String` specifying the path to the application script to run.
 `host_name`   | A `String` specifying the hostname or IP address the server should bind to. Default is `"localhost"`.
 `port`        | An `Int` specifying the port number on which the server will listen. Default is `3443`.
 `docs`        | A `Bool`. If `true`, the `Lit.jl` documentation interface is enabled and served alongside the application at `/docs/build`.
 `dev_mode`    | A `Bool`. If `true`, development mode is enabled. This activates features such as more verbose error reporting and loading of locally built `liblit.so`.

### Return Value

Returns `nothing`.

Once called, this function blocks the current process and keeps the server
running until it is stopped with `Ctrl+C`.
