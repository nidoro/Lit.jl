---
sidebar_position: 60
---

# metric

Display a metric value with an optional delta indicator.

### Function Signature

```julia
function metric(
    label             ::String,
    value             ::String,
    delta             ::String = "",
    higher_is_better  ::Bool   = true
)::Nothing
```

 Argument               | Description
---------------------- | -----------
 `label`               | A `String` used as the label for the metric.
 `value`               | A `String` representing the main value of the metric.
 `delta`               | An optional `String` representing the change or difference associated with the metric (for example, `"+5%"` or `"-2"`).
 `higher_is_better`    | A `Bool` indicating whether an increase in the metric value should be considered positive. Default: `true`.

### Return Value

Nothing.
