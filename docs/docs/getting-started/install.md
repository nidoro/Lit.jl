---
sidebar_position: 1
---

# Installation and Usage

## Installation

### Option 1: Global installation

Use the CLI command below to install `Lit.jl` globally.

```bash
$ julia -e 'using Pkg; Pkg.add(url="https://github.com/nidoro/Lit.jl")'
```

### Option 2: Local project installation

Use the CLI command below to install `Lit.jl` in the current project.

```bash
$ julia --project -e 'using Pkg; Pkg.add(url="https://github.com/nidoro/Lit.jl")'
```

## Implementing `Hello World`

`Lit.jl` web apps are julia scripts that run from top to bottom.

See below a simple `Hello World` script using `Lit.jl` saved with the name
`app.jl`:

```julia
# app.jl
using Lit
if button("Click me")
    text("Hello World!")
end
```

## Running a web app

The previous example web app can be started in the following ways:

### Option 1: From the REPL (recommended during development)

We use the `start_app()` function to start serving a web app from the REPL.

```julia
> using Lit
> start_app()
```

By default, `start_app()` will try to run a file named `app.jl` within the
current directory, but you can give your script a different name and call
`start_app("my_app.jl")`. You can also change some of the server settings,
such as its port, via `start_app()` arguments. See `start_app` to learn more.

The REPL is used just for starting the web app. Once it is started, the server
loop will begin and the REPL will be blocked until the server is stopped with
`Ctrl+C`.

> **✨ NOTE:** You don't necessarily have to stop and restart the server after
> making changes to your web app. The wep app script is reloaded (via `include`)
> on every page refresh/interaction. One exception is if you make changes to
> `@app_startup` or `@page_startup` code blocks, because these blocks are only
> executed once, at the app startup.

In summary, this is the basic web app development workflow:

1. Start the REPL and call `start_app` (once).
2. Open the browser (once).
3. Make changes to the web app script.
4. Refresh or interact with the page.
5. Back to 3.

### Option 2: From the terminal

`Lit.jl` can be executed as a command line tool using the `-m` julia flag.

```bash
$ julia --project -m Lit
```

All of the arguments accepted by `start_app()` are accepted as command line
argument. For instance, the command line below specifies the script that should
be run and the port that the server should listen to:

```bash
$ julia --project -m Lit "my_app.jl" --port 443
```

Run `julia -m Lit --help` to learn more.

Starting the web app from the terminal is not recommended if you are developing
the web app, because during development, you may need to restart the server to
make your changes take effect. But restarting the server from the terminal can
take more time than from restarting it from the REPL because both the julia
runtime and your package dependencies will have to be reloaded. Nevertheless,
this method works fine for running your app in production or running other
people's app that you don't intend to change.


