# ✨ Lit.jl

## A new way of creating Julia web apps!

> 🧪 **Experimental**:
> This package is in early stage of development. You are welcomed to try it out
> and give us your feedback!

`Lit.jl` is a web app framework for Julia that makes it easy for you to build
awesome interactable pages for your Julia creations.

## Features

- **Simple and Julian**: Write beautiful, easy-to-read code.
- **Fast development**: You can very quickly see the effects of your source code
changes, which allows you to fastly detect errors and fix them.
- **No front-end experience required**: You just define the general layout of
the elements in the web app, and `Lit.jl` takes care of displaying it in the
user browser.
- **Immediate mode UI**: No separation between the code for displaying widgets
and the code for getting their values.

## Supported Platforms

- [x] Linux x86_64
- [ ] Windows (will come eventually)

## Documentation

- Getting Started
- Demo Web Apps
- API Reference

## Quick start

### 1. Install:

```bash
$ julia -e 'using Pkg; Pkg.add(url="https://github.com/nidoro/Lit.jl")'
```

### 2. Implement `app.jl`:

```julia
# app.jl
using Lit
if button("Click me")
    text("Button Clicked!")
end
```

### 3. Start the app

From REPL (recommended for faster app restart during development):

```julia
> using Lit
> start_app()
```

Or from the terminal:

```bash
$ julia -m Lit
```

### 4. Open the app in your browser

The default host is http://localhost:3443

## Philosophy

`Lit.jl` was inspired by the popular `streamlit` Python package.
Like `streamlit`, it gives you the necessary
structure to rapidly build a functional nice-looking web app and enough
flexibility to customize it so it looks exactly how you want.



