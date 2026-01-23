<p align="center">
  <img src="served-files/images/magic-logo-and-text.svg" alt="Logo" height="100"><br/>
  <span>A new way of creating Julia web apps!</span>
</p>

---

Made for scientists and researchers, `Magic.jl` is a web app framework for Julia
that makes it easy for you to build awesome interactive web pages in no time!

## 🔥 Features

- **Simple and Julian**: We appreciate the simplicity of julia. We don't want
you to have to learn new obscure macros.
- **Fast, ergonomic development**: Our API enables fast development iteration
cycles.
- **No front-end experience required**: Magic is designed for people with no
web development experience.
- **Data-centric web apps**: We aim to support the development of any data
science web app.

## ⚙️ Supported Platforms

> **NOTE**: Compatible with julia version 1.10 or greater

- ✅ Linux x86_64
- ✅ Windows 64-bit

## 🎓 Documentation

- [Getting Started](https://magic.coisasdodavi.net/docs/build/docs/getting-started/install)
- [Demo Web Apps](https://magic.coisasdodavi.net/)
- [API Reference](https://magic.coisasdodavi.net/docs/build/docs/category/api-reference)

## —͟͟͞͞★ Quick start

### 1. Install:

```bash
pkg> add Magic
```

### 2. Implement `app.jl`:

```julia
# app.jl
using Magic
if button("Click me")
    text("Button Clicked!")
end
```

### 3. Start the app

From REPL (recommended for faster app restart during development):

```julia
> using Magic
> start_app()
```

Or from the terminal (requires Julia 1.12):

```bash
$ julia -m Magic
```

### 4. Open the app in your browser

The default address is http://localhost:3443

## 💡 Philosophy

Inspired by the popular [Streamlit](https://streamlit.io/) Python package, Magic
is the data-centric web app framework that we wanted in julia. Since we didn't
find any package with the features we were looking for, we created our own.

The core idea is simple: a Magic web app is a regular julia script that runs
from top to bottom every time an interaction happens. This idea is simple enough
for any julia programmer to understand and powerful enough to enable them to
get an web app up and running in no time.

But our past experience developing web apps has taught us something about web
app frameworks. In the begining of the development you want the framework to
be very opinionated, so you don't have to worry about details when you just want
to have something reasonably nice-looking and that performs just well enough.
But later in the development, when the app is more mature, you *do* want as much
control as possible, both over the look of the app in the front-end and over
technical details in the back-end.

For that reason, we don't require our users to know HTML or CSS to make a web
app, but if they do want to tweek something using HTML or CSS, we believe that
they should be able to. The same goes for back-end configuration details and
performance. Of course, not every detail can or should be exposed to the user,
but we try our best to not get in their way when they need more control.

## Pre-1.0 Status

As long as we are in version `0.x`, there are a few things that you should know:

- **Collaboration**: You can collaborate for with Magic's development by using it,
testing it and giving your feedback on
[issues](https://github.com/nidoro/Magic.jl/issues). But at this point in time,
when the design of the package is still taking shape and form, we think it is
best that its development stays centralized on us. In the future we will open
for code collaboration.
- **Security and Resilience**: While we believe that Magic has no major
vulnerability since the first public release, tests are still being implemented
to ensure that. Meanwhile, our advice is that you don't host your Magic web app
along with sensitive data and don't use it for anything safety-critical. If you
want to be extra safe, you can run it inside a sandbox like
[NSJail](https://github.com/google/nsjail).




