---
sidebar_position: 60
---

# Page static settings

Page static settings are persistent page settings that can only be defined
at the page's dry-run (`@page_startup`). The page static settings are used to
build the static HTML that is served when a user accesses one of the page's
URLs, which is why they cannot be changed after the page's dry-run.

Example:

```julia
@page_startup begin
    set_title("My Lit App")
    set_description("An awesome app built with Lit.jl")
end
```

See below functions to customize different page static settings.

## set_title

Sets the title of the current page (i.e. the HTML `<title>` tag).

### Function Signature

```julia
function set_title(title::String)::Nothing
```

Argument        | Description
---------------- |-------------
 `title`        | `String`. Title to be assigned to the current page.

## set_description

Sets the description of the current page (i.e. the HTML
`<meta property="og:description">` tag).

### Function Signature

```julia
function set_description(description::String)::Nothing
```

Argument        | Description
---------------- |-------------
 `description`        | `String`. Description to be assigned to the current page.

## add_font

Makes a font available in the current page, adding the necessary CSS
`@font-face` configuration in the `head` of the current page.

### Function Signature

```julia
function add_font(font_name::String, src_or_path::String)::Nothing
```

Argument        | Description
---------------- |-------------
 `font_name`        | `String`. The name that should be associated with the font.
 `src_or_path` | `String`. Either an external URL or a local serveable path inside the project's `.Lit/served-files/` directory. We recommend that you place all of your font files inside `.Lit/served-files/fonts/`.

## add_css_rule

Appends CSS rule(s) to the `head` of the current page.

Example:

```julia
add_css_rule("""
    label {
        font-weight: bold;
    }
    pre {
        border: 1px solid black;
    }
""")
```

### Function Signature

```julia
function add_css_rule(rule::String)::Nothing
```

Argument        | Description
---------------- |-------------
 `rule`        | `String`. A valid CSS rule. Example: <pre>h1, h2, h3, h4, h5, h6 \{<br/>  color: navy;<br/>\}</pre>


