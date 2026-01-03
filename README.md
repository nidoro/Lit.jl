# ✨ Lit.jl

## A new way of creating Julia web apps!

Made for scientists and researchers, `Lit.jl` is a web app framework for Julia
that makes it easy for you to build awesome interactable web pages in no time!

## 🔥 Features

- **Simple and Julian**: We appreciate the simplicity of julia. We don't want
you to have to learn new obscure macros.
- **Fast, ergonomic development**: Our API enables fast development iteration
cycles.
- **No front-end experience required**: Lit is designed for people with no
web development experience.
- **Data-centric web apps**: We aim to support the development of any data
science web app.

## ⚙️ Supported Platforms

> **NOTE**: Compatible with julia version 1.10 or greater

- ✅ Linux x86_64 &mdash; also works on WSL
- ⏳ Windows (will come eventually)

## 🎓 Documentation

- [Getting Started](https://lit.coisasdodavi.net/docs/build/docs/getting-started/install)
- [Demo Web Apps](https://lit.coisasdodavi.net/)
- [API Reference](https://lit.coisasdodavi.net/docs/build/docs/category/api-reference)

## —͟͟͞͞★ Quick start

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

The default address is http://localhost:3443

## 💡 Philosophy

Inspired by the popular [Streamlit](https://streamlit.io/) Python package, Lit
is the data-centric web app framework that we wanted in julia. Since we didn't
find any package with the features we were looking for, we created our own.

The core idea is simple: a Lit web app is a regular julia script that runs
from top to bottom every time an interaction happens. This idea is simple enough
for any julia programmer to understand and powerful enough to enable them to
get an web app up and running in no time.

But our past experience developing web apps has taught us something about web
app frameworks. In the begining of the development you want the framework to
be very opinionated, so you don't have to worry about details when you just want
to have something reasonably nice-looking and that performs well enough. But
later in the development, when the app is more mature, you *do* want as much
control as possible, both over the look of the app in the front-end and over
technical details in the back-end.

For that reason, we don't require our users to know HTML or CSS to make a web
app, but if they do want to tweek something using HTML or CSS, we believe that
they should be able to. The same goes for back-end configuration details and
performance. Of course, not every detail can or should be exposed to the user,
but we try our best to not get in their way when they need more control.





