---
sidebar_position: 85
---

# Widget Value

You can get and set the value of uniquely identified widgets via the
[`get_value()`](#get_value) and [`set_value()`](#set_value) functions.

All widget creation functions accept a user-defined `id` as an argument.
A widget is uniquely identified if, at the moment of its creation, a user
defined `id` was provided. Example:

```julia
selectbox("Selectbox", initial_value="C", options=["A", "B", "C"], id="my_selectbox")
set_value("my_selectbox", "B")
value = get_value("my_selectbox") # value = "B"
````

`get_value()` can be called even before the creation of a widget, in which case
it will return either `missing` or, if the widget has a default value previously
set with [`set_default_value()`](#set_default_value), its default value. So, as you might have
guessed, `set_default_value()` can also be called before the creation of a
widget. Example:

```julia
set_default_value("my_selectbox", "B")
value = get_value("my_selectbox") # value = "B"
selectbox("Selectbox", options=["A", "B", "C"], id="my_selectbox")
````

The rationale behind this API behaviour is that, by setting a default value
before the creation of a widget, you can guarantee that `get_value()` will
always return the value of the widget, regardless if it has been already
created or not: if it has been created, it returns its actual value, and if it
hasn't been created yet, it returns the value it will be assigned when it is
created.

## `get_value()`

Retrieves the value of a uniquely identified widget.

### Function Signature

```julia
function get_value(id::String)::Any
```

 Argument  | Description
---------- |-------------
 `id` | User-defined widget id.

### Return Value

If a widget with the passed id already exists, it returns the value of the
widget.

If it doesn't exists yet but a default value has been assigned to it via
a previous call to `set_default_value()`, it returns the default value.

Otherwise, it returns `missing`.

## `set_value()`

Sets the value of a uniquely identified widget.

### Function Signature

```julia
function set_value(id::String, value::Any)::Nothing
```

 Argument  | Description
---------- |-------------
 `id` | User-defined widget id.
 `value` | The value that should be assigned to the widget. Its type must be compatible with the widget's kind.

## `get_default_value()`

Retrieves the default value of a uniquely identified widget. A widget has a
default value if, and only if, it has been assigned one via a
`set_default_value()` call.

### Function Signature

```julia
function get_default_value(id::String)::Any
```

 Argument  | Description
---------- |-------------
 `id` | User-defined widget id.

### Return Value

Returns the default value assigned to the widget associated with the `id` via
a previous call to `set_default_value()`.

## `set_default_value()`

Sets the default value of a uniquely identified widget.

### Function Signature

```julia
function set_default_value(id::String, value::Any)::Nothing
```

 Argument  | Description
---------- |-------------
 `id` | User-defined widget id.
 `value` | The default value that should be assigned to the widget. Its type must be compatible with the widget's kind.

