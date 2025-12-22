# ✨ Lit.jl

## A new way of creating Julia web apps!

> 🧪 **Experimental**:
> This package is in early stage of development. You are welcomed to try it out
> and give us your feedback!

## Demo Web Apps

## Quick start
### 0. Install:
```julia
using Pkg
Pkg.add(url="https://github.com/nidoro/Lit.jl")
```

### 1. Implement `app.jl`:
```julia
# app.jl
using Main.Lit
if button("Click me")
    text("Button Clicked!")
end
```

### 2. Start REPL and run `app.jl`
```julia
using Lit
@start
```

## Documentation

## Philosophy

`Lit.jl` is a web app framework for Julia that makes it easy for you to build
awesome interactable pages for your Julia creations. `Lit.jl` is what you need
to give a web interface to your Julia programs. With `Lit.jl` you can make
dashboards, data analysers, data transformers, optimizers, simulators, forms
and much more.

`Lit.jl` is inspired by the popular `streamlit` python package.
