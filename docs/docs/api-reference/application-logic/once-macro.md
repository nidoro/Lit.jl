---
sidebar_position: 25
---

# @once

Macro to prevent structs from being redefined on script reruns.

Pass a struct definition to this macro to prevent it from being redefined
on script reruns. Example:

```julia
@once mutable struct Foo
    bar::String
    baz::Int
end
```

## Explanation

Struct redefinitions can cause problems, especially on session persistent data.
Consider the following example:

```julia
mutable struct Foo
    bar::Int
end

mutable struct SessionData
    foo::Foo
end

@session_startup begin
    session = SessionData(Foo(0))
    set_session_data(session)
end

session.foo = Foo(rand(1:10))
```

The above code will work on the first session pass but will fail in the next
one with the following message:

> LoadError:
>   MethodError: Cannot \`convert\` an object of type **Main.MagicApp.Foo**
>   to an object of type **Main.MagicApp.Foo**.

This means that `Foo` was redefined and its new definition cannot be converted
to the original one that is being used in the session persistent data.

To avoid this issue, we simply prepend `@once` to the definition of `Foo`.







