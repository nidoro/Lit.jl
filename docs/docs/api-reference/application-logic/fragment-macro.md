---
sidebar_position: 30
---

# Fragments

A fragment is a function that can be rerun independently of the full app. It
can be created using the the `fragment()` function or the `@fragment` macro.

Use fragments to avoid rerunning the entire app script on every widget
interaction.

### The `fragment()` function

Turns a function into a fragment.

#### Function Signature

```julia
function fragment(func::Function; id::String=String(nameof(func)))
```

 Argument  | Description
---------- |-------------
 `func`   | The `Function` that will be isolated from the rest of the app.
 `id`   | A `String` to uniquely identify the fragment.

### The `@fragment` macro

Turns a code block into a fragment. Usage:

```julia
@fragment begin
    # code block
end
```

Internally, `@fragment` creates a function with the passed code block as its
body, and then calls the function `fragment()` to register the function as
a fragment.

## Example

[Live example here](https://magic.coisasdodavi.net/fragment-example).

```julia
using Magic

@once mutable struct SessionData
    app_reruns::Int
    fragment_reruns::Int
end

@session_startup begin
    session = SessionData(0, 0)
    set_session_data(session)
end

session = get_session_data()
button("Outside Button")
session.app_reruns += 1
text("App reruns: $(session.app_reruns)")

@fragment begin
    session = get_session_data()
    button("Inside Button")
    session.fragment_reruns += 1
    text("Fragment reruns: $(session.fragment_reruns)")
end
```

In the above example, everytime the `Outside Button` is clicked, both counters
`session.app_reruns` and `session.fragment_reruns` are incremented. But when
the `Inside Button` is clicked, only the `session.fragment_reruns` counter is
incremented.







