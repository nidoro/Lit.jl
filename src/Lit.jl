module Lit

# Interface Elements
#--------------------
export html, text, h1, h2, h3, h4, h5, h6, link, space, metric, button, image,
dataframe, selectbox, radio, checkbox, checkboxes, text_input,
code, color_picker, get_value, set_value, get_changes

# Layout Elements
#-------------------
export set_page_layout, main_area, left_sidebar, right_sidebar, row, column,
columns, container, @push, @pop, push_container, pop_container

# Application Logic
#--------------------
export start_app, @app_startup, @page_startup, @session_startup, @once,
set_app_data, get_app_data, set_page_data, get_page_data, set_session_data,
get_session_data, get_default_value, set_default_value,
is_app_first_pass, is_page_first_pass, is_session_first_pass, gen_resource_path,
fragment, @fragment, get_url_path, is_on_page, get_current_page, add_page,
add_css_rule, add_font, begin_page_config, end_page_config, set_title,
set_description

using ArgParse
using Libdl
using Parameters
using Sockets
using Logging
using JSON
using SHA
using Tables
using DataFrames
using Random
using Artifacts

# Colored log utils
#-------------------------
const AC_Reset         = "\x1b[0m"
const AC_CodeRed       = "\x1b[31m"
const AC_CodeGreen     = "\x1b[32m"
const AC_CodeYellow    = "\x1b[33m"
const AC_CodeBlue      = "\x1b[34m"
const AC_CodeMagenta   = "\x1b[35m"
const AC_CodeCyan      = "\x1b[36m"

const AC_CodeBold      = "\x1b[1m"
const AC_ResetBold     = "\x1b[22m"

const AC_Red     = (text::String) -> AC_CodeRed     * text * AC_Reset
const AC_Green   = (text::String) -> AC_CodeGreen   * text * AC_Reset
const AC_Yellow  = (text::String) -> AC_CodeYellow  * text * AC_Reset
const AC_Blue    = (text::String) -> AC_CodeBlue    * text * AC_Reset
const AC_Magenta = (text::String) -> AC_CodeMagenta * text * AC_Reset
const AC_Cyan    = (text::String) -> AC_CodeCyan    * text * AC_Reset

# WidgetKind
#------------
const WidgetKind             = Int
const WidgetKind_None        = 0
const WidgetKind_Button      = 1
const WidgetKind_Selectbox   = 2
const WidgetKind_Checkboxes  = 3
const WidgetKind_Radio       = 4
const WidgetKind_Image       = 5
const WidgetKind_DataFrame   = 6
const WidgetKind_TextInput   = 7
const WidgetKind_ColorPicker = 8
const WidgetKind_Code = 9

@with_kw mutable struct ContainerInterface
    container::Union{Dict, Nothing} = nothing

    columns::Function = (args...; kwargs...)->()
    column::Function = (args...; kwargs...)->()
    row::Function = (args...; kwargs...)->()
    button::Function = (args...; kwargs...)->()
    image::Function = (args...; kwargs...)->()
    html::Function = (args...; kwargs...)->()
    h1::Function = (args...; kwargs...)->()
    h2::Function = (args...; kwargs...)->()
    h3::Function = (args...; kwargs...)->()
    h4::Function = (args...; kwargs...)->()
    h5::Function = (args...; kwargs...)->()
    h6::Function = (args...; kwargs...)->()
    dataframe::Function = (args...; kwargs...)->()
    checkbox::Function = (args...; kwargs...)->()
    checkboxes::Function = (args...; kwargs...)->()
end

CONTAINER_INTERFACE_FUNCS = [:columns, :column, :row, :button, :image, :html, :h1, :h2, :h3, :h4, :h5, :h6, :dataframe, :checkbox, :checkboxes]

function (container_interface::ContainerInterface)(inner_func::Function)::Nothing
    push_container(container_interface)
    inner_func()
    pop_container()
    return nothing
end

@with_kw mutable struct Containers
    containers::Vector{Union{ContainerInterface, Nothing}} = Vector{Union{ContainerInterface, Nothing}}()

    main_area::Union{ContainerInterface, Nothing} = nothing
    left_sidebar::Union{ContainerInterface, Nothing} = nothing
    right_sidebar::Union{ContainerInterface, Nothing} = nothing
end

Base.getindex(containers::Containers, i) = containers.containers[i]

function (containers::Containers)(inner_func::Function, column_index::Int)
    push_container(containers.containers[column_index])
    inner_func()
    pop_container()
    return nothing
end

@with_kw mutable struct Widget
    id::String = ""
    user_id::Union{String, Nothing} = nothing
    kind::WidgetKind = WidgetKind_None
    clicked::Bool = false
    value::Any = nothing
    changes::Dict{Int, Dict{String, Any}} = Dict{Int, Dict{String, Any}}()
    alive::Bool = true
    fragment_id::String = ""

    onclick::Function = (args...; kwargs...)->()
    onchange::Function = (args...; kwargs...)->()
    args::Vector = Vector()

    props::Dict = Dict()
end

@with_kw mutable struct PageConfig
    id::String = ""
    uris::Vector{String} = Vector{String}()
    title::String = "Lit App"
    description::String = "Web app made with Lit.jl"
    style::String = ""
    file_path::String = ""
    first_pass::Bool = true
    user_page_data::Any = nothing

    set_title::Function = (args...; kwargs...)->()
    set_description::Function = (args...; kwargs...)->()
    add_font::Function = (args...; kwargs...)->()
    add_css_rule::Function = (args...; kwargs...)->()
end

@with_kw mutable struct Fragment
    id::String = ""
    func::Function = ()->()
    container_props::Dict = Dict()
end

@with_kw mutable struct AppTask
    task::Union{Task, Nothing} = nothing
    client_id::Cint = 0
    session::Any = nothing
    state::Dict{String, Any} = Dict{String, Any}()
    container_stack::Vector{ContainerInterface} = Vector{ContainerInterface}()
    fragment_stack::Vector{Fragment} = Vector{Fragment}()
    payload::Dict = Dict()
    current_page::PageConfig = PageConfig()
    layout::Containers = Containers()
end

const NetEventType            = Cint
const NetEventType_None       = Cint(0)
const NetEventType_NewClient  = Cint(1)
const NetEventType_NewPayload = Cint(2)
const NetEventType_ServerLoopInterrupted = Cint(3)

@with_kw mutable struct NetEvent
    ev_type::NetEventType = NetEventType_None
    client_id::Cint = 0
    payload::Ptr{Cchar} = Ptr{Cchar}(0)
    payload_size::Cint = 0
end

const AppEventType            = Cint
const AppEventType_None       = Cint(0)
const AppEventType_NewPayload = Cint(1)

@with_kw mutable struct AppEvent
    ev_type::AppEventType = AppEventType_None
    client_id::Cint = 0
    payload::Ptr{Cchar} = Ptr{Cchar}(0)
    payload_size::Cint = 0
end

const InternalEventType          = Cint
const InternalEventType_None     = Cint(0)
const InternalEventType_Network  = Cint(1)
const InternalEventType_Task     = Cint(2)

@with_kw mutable struct InternalEvent
    ev_type::InternalEventType = InternalEventType_None
    data::Union{NetEvent, AppTask} = Union{NetEvent, AppTask}()
end

@with_kw mutable struct RerunRequest
    payload::Dict = Dict()
end

@with_kw mutable struct Session
    client_id::Cint = 0
    widgets::Dict{String, Widget} = Dict{String, Widget}()
    fragments::Dict{String, Fragment} = Dict{String, Fragment}()
    user_session_data::Any = nothing
    first_pass::Bool = true
    widget_defaults::Dict{String, Any} = Dict{String, Any}()
    rerun_task::Union{Task, Nothing} = nothing
    rerun_queue::Vector{RerunRequest} = Vector{RerunRequest}()
end

@with_kw mutable struct Global
    initialized::Bool = false
    script_path::Union{String, Nothing} = nothing
    sessions::Dict{Cint, Session} = Dict{Ptr{Cvoid}, Session}()
    fd_read ::Int32 = -1
    fd_write::Int32 = -1
    internal_events::Channel{InternalEvent} = Channel{InternalEvent}(1024)
    user_app_data::Any = nothing
    first_pass::Bool = true
    base_page_config::PageConfig = PageConfig()
    pages::Vector{PageConfig} = Vector{PageConfig}()
    verbose::Bool = false
    dev_mode::Bool = false
end

g = Global()

macro once(def)
    struct_name = def.args[2]

    return esc(quote
        if haskey(Lit.USER_TYPES, $(QuoteNode(struct_name)))
            global $struct_name = Lit.USER_TYPES[$(QuoteNode(struct_name))]
        else
            $def

            Lit.USER_TYPES[$(QuoteNode(struct_name))] = $struct_name
        end
    end)
end

function get_dyn_lib_path()::String
    if g.dev_mode
        if Sys.islinux()
            return joinpath(@__DIR__, "../build/linux-x86_64/artifacts-linux-x86_64/liblit.so")
        elseif Sys.iswindows()
            return joinpath(@__DIR__, "../build/win64/artifacts-win64/liblit.dll")
        else
            @error "Unsupported OS"
        end
    else
        if Sys.islinux()
            return joinpath(artifact"artifacts", "liblit.so")
        else
            @error "Unsupported OS"
        end
    end
    return ""
end

# Constants and Globals
#--------------------------
LIT_SO    = nothing
LIBLIT    = nothing
START_CWD = pwd()
USER_TYPES= Dict{Symbol,DataType}()

# Container
#-----------
function push_container(i_container::ContainerInterface)::ContainerInterface
    task = task_local_storage("app_task")
    push!(task.container_stack, i_container)
    return i_container
end

function pop_container()::ContainerInterface
    task = task_local_storage("app_task")
    return pop!(task.container_stack)
end

function top_container()::Dict
    task = task_local_storage("app_task")
    return task.container_stack[end].container
end

macro push(container)
    :(Lit.push_container($(esc(container))))
end

macro pop()
    :(Lit.pop_container())
end

function push_fragment(frag::Fragment)::Nothing
    task = task_local_storage("app_task")
    push!(task.fragment_stack, frag)
    return nothing
end

function pop_fragment()::Fragment
    task = task_local_storage("app_task")
    return pop!(task.fragment_stack)
end

function top_fragment()::Fragment
    task = task_local_storage("app_task")
    return task.fragment_stack[end]
end

function define_widget_func(interface, func_name)
    f = function(args...; kwargs...)
        push_container(interface)
        value = getfield(Lit, func_name)(args...; kwargs...)
        pop_container()
        return value
    end

    setfield!(interface, func_name, f)
end

function create_interface(container::Dict)::ContainerInterface
    interface = ContainerInterface()
    interface.container = container

    task = task_local_storage("app_task")
    widgets = task.session.widgets

    for func in CONTAINER_INTERFACE_FUNCS
        define_widget_func(interface, func)
    end

    return interface
end

function define_page_config_func(page, func_name)
    f = function(args...; kwargs...)
        begin_page_config(page)
        getfield(Lit, func_name)(page, args...; kwargs...)
        end_page_config()
        return page
    end

    setfield!(page, func_name, f)
end

function create_container(parent::Dict, css::Dict, attributes::Dict, fragment_id::String="", is_fragment_container::Bool=false)::ContainerInterface
    container = Dict(
        "type" => "container",
        "children" => Vector{Dict{String, Any}}(),
        "id" => "$(parent["id"])/$(length(parent["children"]))",
        "css" => css,
        "attributes" => attributes,
        "is_fragment_container" => is_fragment_container,
        "fragment_id" => fragment_id,
    )

    push!(parent["children"], container)

    return create_interface(container)
end

function container(inner_func::Function=()->(); css::Dict=Dict(), attributes::Dict=Dict())::ContainerInterface
    combined_css = Dict(
        "display" => "flex",
        "flex-direction" => "column",
        "align-items" => "flex-start",
        "justify-content" => "flex-start",
        "gap" => ".8rem",
    )

    merge!(combined_css, css)
    i_container = create_container(top_container(), combined_css, attributes)

    push_container(i_container)
    inner_func()
    pop_container()
    return i_container
end

function fragment(func::Function; id::String=String(nameof(func)))
    task = task_local_storage("app_task")

    wrapper = create_container(top_container(), Dict("display" => "contents", "flex-direction" => get_css_value(top_container(), "flex-direction")), Dict(), id, true)

    frag = Fragment()
    frag.id = id
    frag.func = func
    frag.container_props = wrapper.container

    task.session.fragments[id] = frag

    push_fragment(frag)
    push_container(wrapper)
    func()
    pop_container()
    pop_fragment()
end

macro fragment(block)
    return :(
        Lit.fragment(() -> $(esc(block)))
    )
end

function get_css_value(element::Dict, property::String)
    if haskey(element, "css") && haskey(element["css"], property)
        return element["css"][property]
    else
        return missing
    end
end

function set_css_if_not_set(css::Dict, key::String, value::String)::Nothing
    if !haskey(css, key)
        css[key] = value
    end
    return nothing
end

function set_css_to_achieve_layout(css::Dict, parent::Dict, fill_width::Bool, fill_height::Bool)
    flex_grow = "0"
    min_width = nothing
    min_height = nothing
    width = nothing
    height = nothing

    if fill_width
        if get_css_value(parent, "flex-direction") == "row"
            flex_grow = "1"
            min_width = "0"
        else
            width = "100%"
        end
    end

    if fill_height
        if get_css_value(parent, "flex-direction") == "row"
            height = "100%"
        else
            flex_grow = "1"
            min_height = "0"
        end
    end

    css["flex-grow"] = flex_grow
    if min_width !== nothing css["min-width"] = min_width end
    if min_height !== nothing css["min-height"] = min_height end
    if width !== nothing css["width"] = width end
    if height !== nothing css["height"] = height end
end

function column(
        inner_func::Function=()->();
        fill_width::Bool=false,
        fill_height::Bool=false,
        align_items::String="flex-start",
        justify_content::String="flex-start",
        gap::String=".8rem",
        max_width::String="100%",
        max_height::String="initial",
        show_border::Bool=false,
        border::String="1px solid #d6d6d6",
        padding::String="none",
        margin::String="none",
        css::Dict=Dict(),
        attributes::Dict=Dict()
    )::ContainerInterface

    combined_css = Dict(
        "gap" => gap,
        "align-items" => align_items,
        "justify-content" => justify_content,
        "max-width" => max_width,
        "border" => show_border ? border : "none",
        "border-radius" => "0.5rem",
        "padding" => padding,
        "margin" => margin,
    )

    set_css_to_achieve_layout(combined_css, top_container(), fill_width, fill_height)
    set_css_if_not_set(css, "max-height", max_height)
    merge!(combined_css, css)

    return container(inner_func, css=combined_css, attributes=attributes)
end

function row(
        inner_func::Function=()->();
        fill_width::Bool=false,
        fill_height::Bool=false,
        align_items::String="flex-start",
        justify_content::String="flex-start",
        gap::String="0.8rem",
        margin::String="0",
        css::Dict=Dict()
    )::ContainerInterface

    combined_css = Dict(
        "flex-direction" => "row",
        "gap" => gap,
        "align-items" => align_items,
        "justify-content" => justify_content,
        "margin" => margin,
    )

    set_css_to_achieve_layout(combined_css, top_container(), fill_width, fill_height)

    merge!(combined_css, css)
    return container(inner_func, css=combined_css)
end

function columns(amount_or_widths::Union{Int, Vector}; kwargs...)::Containers
    columns = Containers()

    @push row(fill_width=true)
        if amount_or_widths isa Int
            w = 1.0/amount_or_widths
            css = Dict("flex" => "$w", "align-self" => "stretch", "min-width" => "0")
            if haskey(kwargs, :css)
                merge!(css, kwargs[:css])
            end

            for c in 1:amount_or_widths
                col = column(css=css; kwargs...)
                push!(columns.containers, col)
            end
        else
            for w in amount_or_widths
                css = Dict("flex" => "$w", "align-self" => "stretch", "min-width" => "0")

                if haskey(kwargs, :css)
                    merge!(css, kwargs[:css])
                end

                col = column(css=css; kwargs...)
                push!(columns.containers, col)
            end
        end
    @pop

    return columns
end

# Layout
#-------------
function create_sidebar(initial_state::String, side::String, initial_width::String, position::String, labels::Tuple{Union{String, Nothing}, Union{String, Nothing}})::ContainerInterface
    state_class = initial_state == "open" ? "lt-show" : ""
    side_class = "lt-$(side)"
    position_class = position == "slide-out" ? "lt-slide-out" : "lt-overlay"

    open_label = "<lt-icon lt-icon='material/arrow_forward_ios'></lt-icon>"
    close_label = "<lt-icon lt-icon='material/arrow_back_ios'></lt-icon>"

    if side == "right"
        open_label, close_label = close_label, open_label
    end

    open_label  = labels[1] != nothing ? labels[1] : open_label
    close_label = labels[2] != nothing ? labels[2] : close_label

    sidebar_wrapper = column(
        fill_height=true,
        max_height="100vh",
        attributes=Dict("class" => "lt-sidebar $(side_class) $(state_class) $(position_class)", "data-lt-open-label" => open_label, "data-lt-close-label" => close_label),
        css=Dict("--sidebar-width" => initial_width)
    )
    sidebar_lining = sidebar_wrapper.column(fill_width=true, fill_height=true, attributes=Dict("class" => "lt-sidebar-lining"))
    sidebar_lining.html("dd-button", "", attributes=Dict("onclick" => "LT_ToggleSidebar(event)", "class" => "lt-sidebar-toggle-button"))
    sidebar = sidebar_lining.column(fill_width=true, fill_height=true, attributes=Dict("class" => "lt-sidebar-content"))
    return sidebar
end

function set_page_layout(
        inner_func::Function=()->();
        style::String="basic",
        max_width::String="600px",

        left_sidebar_initial_state::Union{Nothing, String}=nothing,
        left_sidebar_initial_width::String="300px",
        left_sidebar_position::String="slide-out",
        left_sidebar_toggle_labels::Tuple{Union{String, Nothing}, Union{String, Nothing}}=(nothing, nothing),

        right_sidebar_initial_state::Union{Nothing, String}=nothing,
        right_sidebar_initial_width::String="300px",
        right_sidebar_position::String="slide-out",
        right_sidebar_toggle_labels::Tuple{Union{String, Nothing}, Union{String, Nothing}}=(nothing, nothing),
    )::Containers

    # Initialize sidebars
    #------------------------
    left_sidebar, right_sidebar = nothing, nothing

    @push row(fill_width=true, fill_height=true, gap="0px")
        if left_sidebar_initial_state != nothing
            left_sidebar = create_sidebar(left_sidebar_initial_state, "left", left_sidebar_initial_width, left_sidebar_position, left_sidebar_toggle_labels)
        end

        main_area = column(fill_width=true, fill_height=true)

        if right_sidebar_initial_state != nothing
            right_sidebar = create_sidebar(right_sidebar_initial_state, "right", right_sidebar_initial_width, right_sidebar_position, right_sidebar_toggle_labels)
        end
    @pop

    # Initialize main area
    #-------------------------
    if style == "basic"
        # Nothing to do
    elseif style == "centered"
        @push main_area
            @push column(fill_width=true, fill_height=true, align_items="center", padding="3rem 5px 5px 5px", css=Dict("overflow" => "auto"))
                main_area = column(fill_width=true, fill_height=true, align_items="center", max_width=max_width, css=Dict("align-items" => "center"))
            @pop
        @pop
    elseif style == "wide"
        @push main_area
            @push column(fill_width=true, fill_height=true, align_items="center", padding="3rem 0 0 0", css=Dict("overflow" => "auto"))
                main_area = column(fill_width=true, fill_height=true, css=Dict("padding" => "0 5%"))
            @pop
        @pop
    end

    push_container(main_area)
    inner_func()
    # NOTE: we don't pop the container, so main_area() is essentially the new
    # root where top-level elements are placed.

    containers = Containers()
    containers.containers = [main_area, left_sidebar, right_sidebar]
    containers.main_area = main_area
    containers.left_sidebar = left_sidebar
    containers.right_sidebar = right_sidebar

    task = task_local_storage("app_task")
    task.layout = containers

    return containers
end

function main_area(inner_func::Function)::ContainerInterface
    task = task_local_storage("app_task")
    push_container(task.layout.main_area)
    inner_func()
    pop_container()
    return task.layout.main_area
end

function left_sidebar(inner_func::Function)::ContainerInterface
    task = task_local_storage("app_task")
    if task.layout.left_sidebar == nothing
        throw(ArgumentError(
            "Your layout does not have a left sidebar. To create one,\n" *
            "first call `set_page_layout()` with the desired `left_sidebar` parameters."
        ))
    end
    push_container(task.layout.left_sidebar)
    inner_func()
    pop_container()
    return task.layout.left_sidebar
end

function right_sidebar(inner_func::Function)::ContainerInterface
    task = task_local_storage("app_task")
    if task.layout.right_sidebar == nothing
        throw(ArgumentError(
            "Your layout does not have a left sidebar. To create one,\n" *
            "first call `set_page_layout()` with the desired `right_sidebar` parameters."
        ))
    end
    push_container(task.layout.right_sidebar)
    inner_func()
    pop_container()
    return task.layout.right_sidebar
end

maybe_get_default_value = (user_id::Union{String, Nothing}) -> (user_id != nothing ? get_default_value(user_id) : nothing)

function coalesce(args...)
    for arg in args
        if arg !== nothing && arg !== missing
            return arg
        end
    end
    return nothing
end

# Button
#-----------
function create_button(widgets::Dict{String, Widget}, parent::Dict, label::String, style::String, icon::String, onclick::Function, args::Vector)::Bool
    props = Dict(
        "type" => "button",
        "label" => label,
        "style" => style,
        "icon" => icon,
    )

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Button
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.value = false
        widget.onclick = onclick
        widget.args = args
        widgets[props["id"]] = widget
    end

    widget.props = props

    return widget.value
end

function button(label::String=""; style::String="secondary", icon::String="", onclick::Function=(args...; kwargs...)->(), args::Vector=Vector())::Bool
    task = task_local_storage("app_task")
    widgets = task.session.widgets
    return create_button(widgets, top_container(), label, style, icon, onclick, args)
end

# Text Input
#-----------
function create_text_input(
        widgets::Dict{String, Widget},
        parent::Dict,
        user_id::Any,
        label::String,
        initial_value::Union{String, Nothing},
        placeholder::Union{String, Nothing},
        css=Dict
    )::Union{String, Nothing}

    props = Dict(
        "type" => "text_input",
        "user_id" => user_id,
        "label" => label,
        "default_value" => maybe_get_default_value(user_id),
        "initial_value" => initial_value,
        "placeholder" => placeholder,
        "css" => css,
    )

    if props["placeholder"] == nothing
        props["placeholder"] = coalesce(props["default_value"], "")
    end

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_TextInput
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.user_id = props["user_id"]
        widget.value = props["initial_value"]
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return coalesce(widget.value, props["default_value"])
end

function text_input(
        label::String;
        id::Any=nothing,
        show_label::Bool=true,
        fill_width::Bool=false,
        initial_value::Union{String, Nothing}=nothing,
        placeholder::Union{String, Nothing}=nothing,
        css::Dict=Dict()
    )::Union{String, Nothing}

    task = task_local_storage("app_task")
    parent = top_container()
    widgets = task.session.widgets

    container_css = Dict()
    set_css_to_achieve_layout(container_css, parent, fill_width, false)

    if !isempty(label) && show_label
        col = column(gap="0.3em", css=container_css)
        col.html("label", label, css=Dict("font-size" => "0.9rem"))
        parent = col.container
    else
        merge!(css, container_css)
    end

    return create_text_input(widgets, parent, id, label, initial_value, placeholder, css)
end

# Selectbox
#-----------
function create_selectbox(
        widgets::Dict{String, Widget},
        parent::Dict,
        user_id::Any,
        label::String,
        options::Vector,
        initial_value::Union{String, Vector, Nothing},
        multiple::Bool,
        placeholder::Union{String, Nothing},
        onchange::Function,
        css=Dict
    )::Union{String, Vector, Nothing}

    props = Dict(
        "type" => "selectbox",
        "user_id" => user_id,
        "default_value" => maybe_get_default_value(user_id),
        "initial_value" => initial_value,
        "label" => label,
        "options" => options,
        "multiple" => multiple,
        "placeholder" => placeholder,
        "css" => css,
    )

    if props["placeholder"] == nothing
        if props["default_value"] !== nothing
            if typeof(props["default_value"]) == String
                props["placeholder"] = props["default_value"]
            else
                props["placeholder"] = join([replace(option, "\"" => "") for option in repr.(props["default_value"])], ", ")
            end
        else
            props["placeholder"] = ""
        end
    end

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Selectbox
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.user_id = props["user_id"]
        widget.value = initial_value
        widget.onchange = onchange
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return coalesce(widget.value, props["default_value"])
end

function selectbox(
        label::String,
        options::Vector;
        initial_value::Union{String, Vector, Nothing}=nothing,
        id::Any=nothing,
        multiple::Bool=false,
        show_label::Bool=true,
        placeholder::Union{String, Nothing}=nothing,
        fill_width::Bool=false,
        onchange::Function=(args...; kwargs...)->(),
        css::Dict=Dict()
    )::Union{String, Vector, Nothing}

    task = task_local_storage("app_task")
    parent = top_container()
    widgets = task.session.widgets

    container_css = Dict()
    set_css_to_achieve_layout(container_css, parent, fill_width, false)

    if !isempty(label) && show_label
        col = column(gap="0.3em", css=container_css)
        col.html("label", label, css=Dict("font-size" => "0.9rem"))
        parent = col.container
    else
        merge!(css, container_css)
    end

    return create_selectbox(widgets, parent, id, label, options, initial_value, multiple, placeholder, onchange, css)
end

# Color Picker
#-------------------
function create_color_picker(
        widgets::Dict{String, Widget},
        parent::Dict,
        user_id::Any,
        label::String,
        initial_value::Union{String, Nothing},
        onchange::Function,
        css=Dict
    )::String

    props = Dict(
        "type" => "color_picker",
        "user_id" => user_id,
        "default_value" => maybe_get_default_value(user_id),
        "initial_value" => initial_value,
        "label" => label,
        "css" => css,
    )

    # NOTE: Color picker can't have "no" value, so initial value must be
    # set to something.
    if props["initial_value"] === nothing
        props["initial_value"] = coalesce(props["default_value"], "#999999")
    end

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_ColorPicker
        widget.id = props["id"]
        widget.user_id = props["user_id"]
        widget.fragment_id = top_fragment().id
        widget.value = props["initial_value"]
        widget.onchange = onchange
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return widget.value
end

function color_picker(
        label::String;
        initial_value::Union{String, Nothing}=nothing,
        id::Any=nothing,
        show_label::Bool=true,
        fill_width::Bool=false,
        onchange::Function=(args...; kwargs...)->(),
        css::Dict=Dict()
    )::String

    task = task_local_storage("app_task")
    parent = top_container()
    widgets = task.session.widgets

    container_css = Dict()
    set_css_to_achieve_layout(container_css, parent, fill_width, false)

    if !isempty(label) && show_label
        col = column(gap="0.3em", css=container_css)
        col.html("label", label, css=Dict("font-size" => "0.9rem"))
        parent = col.container
        if !haskey(css, "align-self") && !haskey(css, "width")
            css["align-self"] = "stretch"
            css["width"] = "initial"
        end
    else
        merge!(css, container_css)
    end

    return create_color_picker(widgets, parent, id, label, initial_value, onchange, css)
end

# Checkbox
#-----------
function create_checkboxes(
        widgets::Dict{String, Widget},
        parent::Dict,
        user_id::Any,
        label::String,
        options::Vector,
        initial_value::Union{Vector, Nothing},
        multiple::Bool,
        onchange::Function,
        args::Vector
    )::Union{Bool, Vector}

    props = Dict(
        "type" => "checkboxes",
        "label" => label,
        "options" => options,
        "initial_value" => initial_value,
        "multiple" => multiple,
        "user_id" => user_id,
    )

    if props["initial_value"] === nothing
        default_value = maybe_get_default_value(user_id)
        if default_value !== nothing
            if multiple
                props["initial_value"] = default_value
            else
                props["initial_value"] = default_value ? [label] : []
            end
        else
            props["initial_value"] = []
        end
    end

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Checkboxes
        widget.id = props["id"]
        widget.user_id = props["user_id"]
        widget.fragment_id = top_fragment().id
        if multiple
            widget.value = props["initial_value"]
        else
            widget.value = (length(props["initial_value"]) > 0)
        end
        widget.onchange = onchange
        widget.args = args
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return widget.value
end

function checkbox(
        label::String;
        id::Any=nothing,
        initial_value::Union{Bool, Nothing}=nothing,
        onchange::Function=(args...; kwargs...)->(),
        args::Vector=Vector()
    )::Bool

    task = task_local_storage("app_task")
    widgets = task.session.widgets

    init_value = nothing

    if initial_value !== nothing
        init_value = initial_value ? [label] : []
    end

    return create_checkboxes(widgets, top_container(), id, label, [label], init_value, false, onchange, args)
end

function checkboxes(
        label::String,
        options::Vector;
        id::Any=nothing,
        initial_value::Union{Vector, Nothing}=nothing,
        onchange::Function=(args...; kwargs...)->(),
        args::Vector=Vector()
    )::Vector

    task = task_local_storage("app_task")
    widgets = task.session.widgets
    return create_checkboxes(widgets, top_container(), id, label, options, initial_value, true, onchange, args)
end

# Radio
#-----------
function create_radio(
        widgets::Dict{String, Widget},
        parent::Dict,
        user_id::Any,
        label::String,
        options::Vector,
        initial_value::Union{String, Nothing}
    )::Union{String, Nothing}

    props = Dict(
        "type" => "radio",
        "label" => label,
        "options" => options,
        "user_id" => user_id,
        "initial_value" => initial_value,
    )

    if props["initial_value"] === nothing
        props["initial_value"] = coalesce(maybe_get_default_value(user_id), options[1])
    end

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Radio
        widget.id = props["id"]
        widget.user_id = props["user_id"]
        widget.fragment_id = top_fragment().id
        widget.value = props["initial_value"]
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return widget.value
end

function radio(
        label::String,
        options::Vector;
        id::Any=nothing,
        initial_value::Union{String, Nothing}=nothing
    )::Union{String, Nothing}

    task = task_local_storage("app_task")
    widgets = task.session.widgets
    return create_radio(widgets, top_container(), id, label, options, initial_value)
end

# Image
#-----------
function get_file_sha256(path::AbstractString)
    open(path, "r") do io
        return bytes2hex(sha256(io))
    end
end

function create_image(widgets::Dict{String, Widget}, parent::Dict, src::String, width::Union{Number, Nothing}, height::Union{Number, Nothing}, css::Dict)::String
    props = Dict(
        "type" => "image",
        "src" => src,
        "width" => width,
        "height" => height,
        "css" => css,
    )

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"
    props["src"] = src

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Image
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.value = props["src"]
        widgets[props["id"]] = widget
    end

    props["value"] = widget.value
    widget.props = props

    return widget.value
end

function image(src_or_path::String; fill_width::Bool=false, max_width::String="100%", width::Union{Number, Nothing}=nothing, height::Union{Number, Nothing}=nothing, css::Dict=Dict("height" => "auto"))::String
    task = task_local_storage("app_task")
    widgets = task.session.widgets

    if !haskey(css, "flex-grow") && !haskey(css, "width")
        set_css_to_achieve_layout(css, top_container(), fill_width, false)
    end

    css["height"] = "auto"

    if width != nothing && height != nothing && !haskey(css, "aspect-ratio")
        css["aspect-ratio"] = "$width / $height"
    end

    if !haskey(css, "max-width")
        css["max-width"] = max_width
    end

    src = src_or_path
    if startswith(src_or_path, ".Lit/served-files")
        src = replace(src_or_path, ".Lit/served-files" => "")
    end

    return create_image(widgets, top_container(), src, width, height, css)
end

function get_random_string(n::Integer)::String
    CHARSET = ['0':'9'; 'A':'Z'; 'a':'z']
    return String(rand(CHARSET, n))
end

function gen_resource_path(extension::String)::String
    task = task_local_storage("app_task")
    return ".Lit/served-files/cache/$(task.client_id)-$(get_random_string(32)).$(replace(extension, "." => ""))"
end

# Dataframe
#--------------------
function create_dataframe(
        widgets::Dict{String, Widget},
        parent::Dict, user_id::Any,
        data::DataFrame,
        column_config::Dict,
        height::String,
        onchange::Function,
        args::Vector
    )::DataFrame

    props = Dict(
        "type" => "dataframe",
        "data_ptr" => repr(pointer_from_objref(data)),
        "column_config" => column_config,
        "height" => height,
        "user_id" => user_id,
    )

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_DataFrame
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.user_id = props["user_id"]
        widget.onchange = onchange
        widget.args = args
        widget.value = data
        widgets[props["id"]] = widget
        props["initial_value"] = Tables.collect(Tables.rowtable(data))
    end

    widget.props = props

    return widget.value
end

function dataframe(
        data::DataFrame;
        column_config::Dict=Dict(),
        height::String="400px",
        id::Union{String, Nothing}=nothing,
        onchange::Function=(args...; kwargs...)->(),
        args::Vector=Vector()
    )::DataFrame

    task = task_local_storage("app_task")
    widgets = task.session.widgets

    cc = Dict()

    for column_name in names(data)
        cc[column_name] = Dict()

        if column_name in keys(column_config)
            merge!(cc[column_name], column_config[column_name])
        end

        column_type = eltype(data[:, column_name])
        cc[column_name]["julia_type"] = column_type

        if !("type" in keys(cc[column_name]))
            if Number <: column_type || Int <: column_type || Real <: column_type
                cc[column_name]["type"] = "Number"
            else
                cc[column_name]["type"] = "String"
            end
        end

        if !("empty_value" in keys(cc[column_name]))
            if Nothing <: column_type
                cc[column_name]["empty_value"] = "<nothing>"
            elseif Missing <: column_type
                cc[column_name]["empty_value"] = "<missing>"
            else
                cc[column_name]["empty_value"] = ""
                cc[column_name]["required"] = true
            end
        end
    end

    return create_dataframe(widgets, top_container(), id, data, cc, height, onchange, args)
end

# HTML
#----------------
function create_html(parent::Dict, tag::String, inner_html::String, attributes::Dict, css::Dict)
    html = Dict(
        "type" => "html",
        "tag" => tag,
        "inner_html" => inner_html,
        "attributes" => attributes,
        "css" => css,
    )

    push!(parent["children"], html)
    return nothing
end

function html(tag::String, inner_html::String; attributes::Dict=Dict(), css::Dict=Dict())::Nothing
    create_html(top_container(), tag, inner_html, attributes, css)
    return nothing
end

function link(label::String, url::String; style::String="secondary", fill_width=false, new_tab::Bool=false, css::Dict=Dict())::Nothing
    icon = ""
    if new_tab
        icon = "<lt-icon lt-icon='material/open_in_new'></lt-icon>"
    end

    combined_css = Dict("white-space" => "nowrap")
    set_css_to_achieve_layout(css, top_container(), fill_width, false)

    merge!(combined_css, css)

    html("a", "$label$icon", css=combined_css, attributes=Dict("class" => "lt-link dd-button lt-button-style-$(style)", "href" => url, "target" => new_tab ? "_blank" : ""))
    return nothing
end

space(; width::String="1px", height::String="1px") = html("div", "", css=Dict("width" => width, "height" => height))

# Text
#----------
function maybe_prepend_icon(text::String, icon::String, icon_color::String)::String
    if length(icon) > 0
        style = ""
        if length(icon_color) > 0
            style = "color: $icon_color"
        end
        result = "<lt-icon lt-icon='$icon' style='$(style)'></lt-icon> $text"
        return result
    end
    return text
end

h1(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h1", maybe_prepend_icon(text, icon, icon_color), css=css)
h2(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h2", maybe_prepend_icon(text, icon, icon_color), css=css)
h3(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h3", maybe_prepend_icon(text, icon, icon_color), css=css)
h4(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h4", maybe_prepend_icon(text, icon, icon_color), css=css)
h5(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h5", maybe_prepend_icon(text, icon, icon_color), css=css)
h6(text::String; icon::String="", icon_color::String="", css=Dict()) = html("h6", maybe_prepend_icon(text, icon, icon_color), css=css)

icon(icon::String; color::String="inherit", size::String="inherit", weight::String="inherit") =
    html("lt-icon", "", attributes=Dict("lt-icon" => icon), css=Dict("color" => color, "font-size" => size, "font-weight" => "bold"))

text(text::Any) = html("p", typeof(text) == String ? text : repr(text))

# Code
#------------
function create_code(widgets::Dict{String, Widget}, parent::Dict, initial_value::String, show_line_numbers::Bool, css::Dict)::String
    props = Dict(
        "type" => "code",
        "initial_value" => initial_value,
        "show_line_numbers" => show_line_numbers,
        "css" => css,
    )

    props["local_id"] = bytes2hex(sha256(JSON.json(props)))
    props["container_id"] = parent["id"]
    props["id"] = "$(props["container_id"])/$(props["local_id"])"

    push!(parent["children"], props)

    widget = nothing

    if haskey(widgets, props["id"])
        widget = widgets[props["id"]]
        widget.alive = true
    else
        widget = Widget()
        widget.kind = WidgetKind_Code
        widget.id = props["id"]
        widget.fragment_id = top_fragment().id
        widget.value = initial_value
        widgets[props["id"]] = widget
    end

    widget.props = props

    return widget.value
end

function code(initial_value::String=""; initial_value_file::Union{String, Nothing}=nothing, fill_width::Bool=true, max_width::String="100%", max_height::String="initial", padding::String="0", strip_whitespace::Bool=true, show_line_numbers::Bool=false, css::Dict=Dict("overflow-y" => "auto"))::String
    task = task_local_storage("app_task")
    widgets = task.session.widgets

    if initial_value_file != nothing
        initial_value = read(initial_value_file, String)
    end

    if !haskey(css, "flex-grow") && !haskey(css, "width")
        set_css_to_achieve_layout(css, top_container(), fill_width, false)
    end

    set_css_if_not_set(css, "padding", padding)
    set_css_if_not_set(css, "max-width", max_width)
    set_css_if_not_set(css, "max-height", max_height)

    if strip_whitespace
        initial_value = String(strip(initial_value))
    end

    return create_code(widgets, top_container(), initial_value, show_line_numbers, css)
end

# Metric
#-------------
function metric(label::String, value::String, delta::String="", higher_is_better::Bool=true)::Nothing
    deltaHTML = ""
    color, background = "#0b8a07", "#a6f9a6"

    if length(delta) > 0
        if startswith(delta, "-") || !higher_is_better
            color, background = "#bf0b0b", "#fbacac"
        end

        icon = startswith(delta, "-") ? "material/arrow_downward" : "material/arrow_upward"

        iconHTML = "<lt-icon lt-icon='$icon' style='font-size: 1.1em; color: $color; background: $background'></lt-icon>"
        deltaHTML = "$iconHTML $delta"
    end

    @push column(gap="0")
        html("label", label, css=Dict("font-size" => "0.85rem"))
        html("span", value, css=Dict("font-size" => "1.8rem"))
        if length(deltaHTML) > 0
            html("span", deltaHTML, css=Dict("font-size" => "0.85rem", "color" => color, "background" => background, "border-radius" => "100vw", "padding" => ".2em .4em", "display" => "flex", "align-items" => "center"))
        end
    @pop

    return nothing
end

function get_widget_by_user_id(widgets::Dict{String, Widget}, user_id::String)::Union{Widget, Missing}
    for widget in values(widgets)
        if widget.user_id == user_id
            return widget
        end
    end
    return missing
end

function set_default_value(user_id::String, value::Any)::Nothing
    task = task_local_storage("app_task")
    task.session.widget_defaults[user_id] = value
    widget = get_widget_by_user_id(task.session.widgets, user_id)
    if widget !== missing
        widget.props["default_value"] = value
    end
    return nothing
end

function get_default_value(user_id::String)::Any
    task = task_local_storage("app_task")
    if haskey(task.session.widget_defaults, user_id)
        return task.session.widget_defaults[user_id]
    end
    return missing
end

function set_value(user_id::String, value::Any)::Nothing
    task = task_local_storage("app_task")
    widget = get_widget_by_user_id(task.session.widgets, user_id)
    if widget !== missing
        widget.value = value
        widget.props["value"] = value
    else
        # TODO: WIDGET NOT FOUND
    end
    return nothing
end

function get_value(user_id::String)::Any
    task = task_local_storage("app_task")
    widget = get_widget_by_user_id(task.session.widgets, user_id)
    if widget === missing || widget.value === nothing
        return get_default_value(user_id)
    end
    return widget.value
end

function get_changes(user_id::String)::Union{Missing, Dict{Int, Dict{String, Any}}}
    task = task_local_storage("app_task")
    widget = get_widget_by_user_id(task.session.widgets, user_id)
    if widget === missing
        return missing
    end
    return widget.changes
end

function get_app_data()::Any
    return g.user_app_data
end

function set_app_data(app_data::Any)::Nothing
    g.user_app_data = app_data
    return nothing
end

function get_session_data()::Any
    task = task_local_storage("app_task")
    return task.session.user_session_data
end

function set_session_data(session_data::Any)::Nothing
    task = task_local_storage("app_task")
    task.session.user_session_data = session_data
    return nothing
end

function get_page(uri)::Union{PageConfig, Missing}
    for page in g.pages
        if uri in page.uris
            return page
        end
    end
    return missing
end

function get_page_data()::Any
    page = get_page(get_url_path())
    return page.user_page_data
end

function set_page_data(page_data::Any)::Nothing
    page = get_page(get_url_path())
    page.user_page_data = page_data
    return nothing
end

function add_page(inner_func::Union{Function, Nothing}, uri::Union{String, Vector{String}}; title::String="", description::String="")::PageConfig
    page = PageConfig()
    page.id = get_random_string(6)
    page.uris = uri isa Vector{String} ? uri : [uri]
    page.title = title
    page.description = description

    for func in [:set_title, :set_description, :add_font, :add_css_rule]
        define_page_config_func(page, func)
    end

    push!(g.pages, page)

    if inner_func != nothing
        begin_page_config(page)
        inner_func()
        end_page_config()
    end

    return page
end

add_page(uri::Union{String, Vector{String}}; title::String="", description::String="") = add_page(nothing, uri, title=title, description=description)

function add_css_rule(page::PageConfig, style::String)::Nothing
    page.style *= style
    return nothing
end

function add_css_rule(style::String)::Nothing
    task = task_local_storage("app_task")
    return add_css_rule(task.current_page, style)
end

function strip_prefix(str::String, prefix::String)::String
    if startswith(str, prefix)
        return replace(str, prefix => "")
    end
    return str
end

function add_font(page::PageConfig, font_name::String, src_or_path::String)::Nothing
    add_css_rule(page, """
        @font-face {
            font-family: "$(font_name)";
            src: url($(strip_prefix(src_or_path, ".Lit/served-files")));
        }
    """)
    return nothing
end

function add_font(font_name::String, src_or_path::String)::Nothing
    task = task_local_storage("app_task")
    add_font(task.current_page, font_name, src_or_path)
    return nothing
end

function set_title(page::PageConfig, title::String)::Nothing
    page.title = title
    return nothing
end

function set_title(title::String)::Nothing
    task = task_local_storage("app_task")
    return set_title(task.current_page, title)
end

function set_description(page::PageConfig, description::String)::Nothing
    page.description = description
    return nothing
end

function set_description(description::String)::Nothing
    task = task_local_storage("app_task")
    return set_description(task.current_page, description)
end

function begin_page_config(page::PageConfig)::Nothing
    task = task_local_storage("app_task")
    task.current_page = page
    return nothing
end

function end_page_config()::Nothing
    task = task_local_storage("app_task")
    task.current_page = g.base_page_config
    return nothing
end

function new_client(client_id::Cint)::Nothing
    session = Session()
    session.client_id = client_id
    session.first_pass = true

    root_container_props = Dict(
        "type" => "container",
        "is_fragment_container" => true,
        "fragment_id" => "",
        "children" => Vector{Dict{String,Any}}(),
        "id" => "0",
        "css" => Dict(
            "display" => "flex",
            "flex-direction" => "column",
            "align-items" => "flex-start",
            "justify-content" => "flex-start",
            "width" => "100%",
            "height" => "100%",
            "overflow" => "auto",
        ),
        "attributes" => Dict()
    )

    root_frag = Fragment()
    root_frag.id = ""
    root_frag.func = run_user_script
    root_frag.container_props = root_container_props
    session.fragments[""] = root_frag

    g.sessions[client_id] = session
    return nothing
end

function create_page_html(page::PageConfig, output_path::String)::Nothing
    template = read(joinpath(@__DIR__, "../served-files/LitPageTemplate.html"), String)

    title = length(page.title) > 0 ? page.title : g.base_page_config.title
    description = length(page.description) > 0 ? page.description : g.base_page_config.description

    page_html = replace(
        template,
        "<title>Lit App</title>" => "<title>$(title)</title>",
        "<meta property=\"og:description\" content=\"Web app made with Lit.jl\">" => "<meta property=\"og:description\" content=\"$(description)\">",
        "<!-- LIT PAGE STYLE -->" => "<style>$(page.style)</style>"
    )

    write(output_path, page_html)
    page.file_path = output_path
    return nothing
end

function run_user_script()::Nothing
    app_mod = Module(:LitApp)
    Core.eval(app_mod, quote
        using Base
        using Core
        const include = path -> Base.include(LitApp, path)
    end)
    Base.include(app_mod, g.script_path)
    return nothing
end

function filtered_stacktrace(bt; cutoff_file = nothing)
    frames = stacktrace(bt)

    frames = filter(f -> f.func != :include, frames)

    if cutoff_file !== nothing
        i = findfirst(f -> occursin(cutoff_file, String(f.file)), frames)
        if i !== nothing
            frames = frames[1:i-1]
        end
    end

    return frames
end

function remove_lines_starting_with(err::String, prefix::String)::String
    lines = split(err, '\n')
    keep = filter(l -> !startswith(lstrip(l), prefix), lines)
    return join(keep, '\n')
end

function rerun(client_id::Cint, payload::Dict)::Task
    session = g.sessions[client_id]

    session.rerun_task = Threads.@spawn try
        # TODO: No more than one update task should be allowed to run on a
        # session at any given moment. Queue the updates if needed. And the main
        # event loop should only touch the session at the creation moment.
        #
        # TODO: Because the task depend on the session associated with the
        # client, at the end of a session (i.e. client disconnection), we should
        # first send an interrupt signal to the task associated with the
        # session, if any. Then wait for the task to end (forcefully or not).
        # And only then remove the client and session from the global vector.
        #
        task = AppTask()
        task_local_storage("app_task", task)
        task.task = current_task()
        task.client_id = client_id
        task.session = session
        task.payload = payload
        task.current_page = g.base_page_config

        # Identify and initialize fragment
        #------------------------------------
        fragment_id = ""

        for front_event in payload["events"]
            widget = session.widgets[front_event["widget_id"]]
            fragment_id = widget.fragment_id
        end

        frag = session.fragments[fragment_id]
        frag.container_props["children"] = Vector{Dict{String, Any}}()

        task.state = Dict("root" => frag.container_props)
        root_interface = create_interface(task.state["root"])
        push_container(root_interface)
        push_fragment(frag)

        task.layout = Containers()
        task.layout.main_area = root_interface

        # Handle events
        #------------------
        for widget in values(session.widgets)
            widget.clicked = false
            if widget.kind == WidgetKind_Button
                widget.value = false
            end
            if widget.fragment_id == fragment_id
                widget.alive = false
            end
        end

        # TODO: Although we receive a list of events from the front-end, at the
        # moment we don't expect it to have more than one event.
        for front_event in payload["events"]
            widget = session.widgets[front_event["widget_id"]]
            if front_event["type"] == "click"
                widget.clicked = true
                if widget.kind == WidgetKind_Button
                    widget.value = true
                    invokelatest(widget.onclick, widget.args...)
                end
            elseif front_event["type"] == "change"
                if widget.kind == WidgetKind_Checkboxes
                    if widget.props["multiple"]
                        widget.value = front_event["new_value"]
                    else
                        widget.value = (length(front_event["new_value"]) > 0)
                    end
                    invokelatest(widget.onchange, widget.args...)
                elseif widget.kind == WidgetKind_Selectbox || widget.kind == WidgetKind_Radio || widget.kind == WidgetKind_TextInput || widget.kind == WidgetKind_ColorPicker
                    widget.value = front_event["new_value"]
                    invokelatest(widget.onchange, widget.args...)
                elseif widget.kind == WidgetKind_DataFrame
                    for change in front_event["changes"]
                        column_config = widget.props["column_config"][change["column_name"]]
                        new_value = change["new_value"]

                        if column_config["type"] == "Number" && !(new_value in ["", nothing])
                            if column_config["julia_type"] <: Integer
                                new_value = round(column_config["julia_type"], new_value)
                            end
                        end

                        if (column_config["type"] == "Number" && (new_value == "" || new_value == nothing)) ||
                           (column_config["type"] == "String" && (new_value == nothing))
                            if column_config["empty_value"] == "<nothing>"
                                new_value = nothing
                            elseif column_config["empty_value"] == "<missing>"
                                new_value = missing
                            else
                                new_value = column_config["empty_value"]
                            end
                        end

                        row_changes = get!(widget.changes, change["row_index"], Dict{String, Any}())
                        row_changes[change["column_name"]] = new_value

                        widget.value[change["row_index"], change["column_name"]] = new_value
                    end

                    invokelatest(widget.onchange, widget.args...)
                end
            end
        end

        # Run user fragment
        #-------------------
        invokelatest(frag.func)

        # Remove dead widgets
        #-------------------------
        filter!(p -> p.second.alive, session.widgets)

        g.first_pass = false
        session.first_pass = false
        get_page(get_url_path()).first_pass = false

        if (payload["request_id"] != 0)
            put!(g.internal_events, InternalEvent(InternalEventType_Task, task))
        end

    catch e
        bt = catch_backtrace()
        frames = filtered_stacktrace(bt; cutoff_file = "LitUserSpace.jl")
        err_message = remove_lines_starting_with(sprint(showerror, e), "in expression starting")
        st = sprint(Base.show_backtrace, frames)

        println(stderr, err_message)
        println(stderr, st)

        column(gap=".3em", padding="1em", margin="0 0 2rem 0", fill_width=true, css=Dict("font-family" => "monospace", "white-space" => "pre", "background" => "#fdeded", "color" => "#89454a", "overflow-x" => "auto")) do
            html("span", err_message)
            html("span", st)
        end

        task = task_local_storage("app_task")
        filter!(p -> p.second.alive, task.session.widgets)
        put!(g.internal_events, InternalEvent(InternalEventType_Task, task))
    end

    return session.rerun_task
end

function is_session_first_pass()::Bool
    task = task_local_storage("app_task")
    return task.session.first_pass
end

function is_app_first_pass()::Bool
    return g.first_pass
end

function is_page_first_pass()::Bool
    return get_page(get_url_path()).first_pass
end

macro app_startup(block)
    return :(
        if Lit.is_app_first_pass()
            $(esc(block))
        end
    )
end

macro session_startup(block)
    return :(
        if Lit.is_session_first_pass()
            $(esc(block))
        end
    )
end

macro page_startup(block)
    return :(
        if Lit.is_page_first_pass()
            Lit.begin_page_config(get_page(get_url_path()))
            $(esc(block))
            Lit.end_page_config()
        end
    )
end

function get_url_path()::String
    task = task_local_storage("app_task")
    return task.payload["location"]["pathname"]
end

get_current_page()::PageConfig = get_page(get_url_path())
is_on_page(page_path::String)::Bool = (page_path in get_current_page().uris)

function lock_client(client_id::Cint)::Nothing
    ccall((:LT_LockClient, LIT_SO), Cvoid, (Cint,), client_id)
    return nothing
end

function unlock_client(client_id::Cint)::Nothing
    ccall((:LT_UnlockClient, LIT_SO), Cvoid, (Cint,), client_id)
    return nothing
end

function pop_net_event()::NetEvent
    return ccall((:LT_PopNetEvent, LIT_SO), NetEvent, ())
end

function push_app_event(app_event::AppEvent)::Nothing
    ccall((:LT_PushAppEvent, LIT_SO), Cvoid, (AppEvent,), app_event)
    return nothing
end

function push_uri_mapping(uri::String, resource_path::String)::Nothing
    ccall((:LT_PushURIMapping, LIT_SO), Cvoid, (Cstring, Cint, Cstring, Cint), uri, Cint(sizeof(uri)), resource_path, Cint(sizeof(resource_path)))
    return nothing
end

function clear_uri_mapping()::Nothing
    ccall((:LT_ClearURIMapping, LIT_SO), Cvoid, ())
    return nothing
end

# NOTE: functions to open browser. Copied from LiveServer.jl
#-------------------------------------------------------------
function detectwsl()
    Sys.islinux() &&
    isfile("/proc/sys/kernel/osrelease") &&
    occursin(r"Microsoft|WSL"i, read("/proc/sys/kernel/osrelease", String))
end

function open_in_default_browser(url::AbstractString)::Bool
    try
        if Sys.isapple()
            Base.run(`open $url`)
            true
        elseif Sys.iswindows() || detectwsl()
            Base.run(`cmd.exe /s /c start "" /b $url`)
            true
        elseif Sys.islinux()
            Base.run(`xdg-open $url`)
            true
        else
            false
        end
    catch
        false
    end
end

function is_rerun_request_valid(session::Session, request::RerunRequest)::Bool
    payload = request.payload
    for front_event in payload["events"]
        if !haskey(session.widgets, front_event["widget_id"])
            return false
        end
    end
    return true
end

function start_app(script_path::String="app.jl"; host_name::String="localhost", port::Int=3443, docs::Bool=false, verbose::Bool=false, dev_mode::Bool=false)::Nothing
    if !isfile(script_path)
        @error "File not found: '$(script_path)'"
        return nothing
    end

    global g = Global()
    g.verbose = verbose
    g.dev_mode = dev_mode

    if g.dev_mode
        @warn "Starting Lit.jl on dev mode"
    end

    global LIT_SO = get_dyn_lib_path()
    global LIBLIT = Libdl.dlopen(LIT_SO, Libdl.RTLD_NOW)
    g.sessions = Dict{Ptr{Cvoid}, Session}()
    g.first_pass = true
    g.initialized = true
    g.script_path = joinpath(START_CWD, script_path)

    # Dry run to try and initialize the app
    #-------------------------------------------
    dry_run_payload = Dict(
        "type" => "request_rerun",
        "request_id" => 0,
        "events" => [],
        "location" => Dict(
            "href" => "https://$(host_name):$(port)/",
            "pathname" => "/",
            "host" => "$(host_name):$(port)",
            "hostname" => host_name, "search" => ""
        )
    )

    new_client(Cint(0))
    add_page("/", title="Lit App", description="Lit App")

    @info "Dry Run: First pass over '$(script_path)'.\n$(AC_Green("@app_startup")) code blocks will run now."
    wait(rerun(Cint(0), dry_run_payload))

    if is_app_first_pass()
        @error "Dry run of app '$(script_path)' failed."
        return
    end

    if length(g.pages) > 1
        popfirst!(g.pages)
    end

    for page in g.pages
        @info "Dry Run: First pass over '$(script_path)' as if loading page '$(page.uris[1])'.\n$(AC_Green("@page_startup")) code blocks will run now."
        g.sessions[Cint(0)].first_pass = true

        dry_run_payload["location"]["href"] = "https://$(host_name):$(port)" * page.uris[1]
        dry_run_payload["location"]["pathname"] = page.uris[1]

        wait(rerun(Cint(0), dry_run_payload))

        if page.first_pass
            @error "Dry run of page '$(page.uris[1])' failed."
            return
        end
    end

    # Setup net layer connection
    #--------------------------------
    ipc_server = listen(IPv4(127,0,0,1), 0)
    ipc_port = getsockname(ipc_server)[2]

    init_net_layer(host_name, port, docs, Int(ipc_port), joinpath(@__DIR__, ".."), g.verbose, g.dev_mode)

    ipc_connection = accept(ipc_server)
    @info "NetLayerStarted\nNow serving at http://$(host_name):$(port)"

    mkpath(".Lit/served-files/cache/pages")
    cp(joinpath(@__DIR__, "../served-files/LitPageTemplate.html"), ".Lit/served-files/cache/pages/first.html", force=true)
    push_uri_mapping("/", "/cache/pages/first.html")

    # Configure pages after dry runs
    #---------------------
    create_page_html(g.base_page_config, ".Lit/served-files/cache/pages/base.html")

    for page in g.pages
        create_page_html(page, ".Lit/served-files/cache/pages/$(page.id).html")
    end

    clear_uri_mapping()

    if length(g.pages) > 0
        # User explicitly configured app pages
        for page in g.pages
            for uri in page.uris
                push_uri_mapping(uri, replace(page.file_path, ".Lit/served-files" => ""))
            end
        end
    else
        push_uri_mapping("/", replace(g.base_page_config.file_path, ".Lit/served-files" => ""))
    end

    Threads.@spawn begin
        stop_loop = false

        while isopen(ipc_connection) && !stop_loop
            #readavailable(ipc_connection)
            read(ipc_connection, UInt8)

            ev = pop_net_event()
            while ev.ev_type != NetEventType_None
                put!(g.internal_events, InternalEvent(InternalEventType_Network, ev))
                ev = pop_net_event()

                if ev.ev_type == NetEventType_ServerLoopInterrupted
                    stop_loop = true
                end
            end
        end
    end

    while isopen(ipc_connection)
        ev = take!(g.internal_events)
        if ev.ev_type == InternalEventType_Network
            if ev.data.ev_type == NetEventType_NewClient
                @debug "NetEventType_NewClient | $(ev.data.client_id)"
                new_client(ev.data.client_id)
            elseif ev.data.ev_type == NetEventType_NewPayload
                @debug "NetEventType_NewPayload | $(ev.data.client_id)"
                payload_string = unsafe_string(ev.data.payload, ev.data.payload_size)

                # NOTE: Now that we've copied the payload, it is safe to destroy the event.
                destroy_net_event(ev.data)

                payload = Dict(JSON.parse(payload_string))
                #@show payload

                session = g.sessions[ev.data.client_id]

                if payload["type"] == "request_rerun"
                    if session.rerun_task === nothing
                        rerun(ev.data.client_id, payload)
                    else
                        @debug "Rerun already happening. Queueing rerun request. Current queue size: $(length(session.rerun_queue))"
                        push!(session.rerun_queue, RerunRequest(payload))
                    end
                else
                    @error "Unknown payload type '$(payload["type"])'"
                end
            elseif ev.data.ev_type == NetEventType_ServerLoopInterrupted
                @info "NetEventType_ServerLoopInterrupted"
                close(ipc_connection)
            end
        elseif ev.ev_type == InternalEventType_Task && ev.data.client_id != Cint(0)
            @debug "TaskFinished $(ev.data.client_id)"
            session = g.sessions[ev.data.client_id]

            payload = Dict(
                "type" => "response_rerun",
                "dev_mode" => g.dev_mode,
                "request_id" => ev.data.payload["request_id"],
                "root" => ev.data.state["root"],
                "error" => nothing
            )

            payload_string = JSON.json(payload)
            app_event = create_app_event(AppEventType_NewPayload, session.client_id, payload_string)
            push_app_event(app_event)

            write(ipc_connection, " ")

            session.rerun_task = nothing

            # Start next rerun request on queue, if any
            #-------------------------------------------
            if length(session.rerun_queue) > 0
                rerun_request = popfirst!(session.rerun_queue)
                if is_rerun_request_valid(session, rerun_request)
                    @debug "Running next rerun request in queue"
                    rerun(session.client_id, rerun_request.payload)
                else
                    @debug "Next rerun request in queue is invalid"
                    payload = Dict(
                        "type" => "response_rerun",
                        "dev_mode" => g.dev_mode,
                        "request_id" => ev.data.payload["request_id"],
                        "error" => Dict(
                            "type" => "InvalidState",
                        )
                    )
                    payload_string = JSON.json(payload)
                    app_event = create_app_event(AppEventType_NewPayload, session.client_id, payload_string)
                    push_app_event(app_event)
                    write(ipc_connection, " ")
                end
            end
        end
    end

    Libdl.dlclose(LIBLIT)

    @info "ServerLoopStopped"
    return nothing
end

function create_app_event(event_type::AppEventType, client_id::Cint, payload::String)::AppEvent
    return ccall((:LT_CreateAppEvent, LIT_SO), AppEvent, (AppEventType, Cint, Ptr{Cchar}, Cint), event_type, client_id, payload, Cint(sizeof(payload)))
end

function destroy_net_event(ev::NetEvent)::Nothing
    ccall((:LT_DestroyNetEvent, LIT_SO), Cvoid, (NetEvent,), ev)
end

function init_net_layer(host_name::String, port::Int, docs::Bool, ipc_port::Int, package_root_dir::String, verbose::Bool, dev_mode::Bool)
    ccall(
        (:LT_InitNetLayer, LIT_SO),
        Cvoid,
        (Cstring, Cint, Cint, Cint, Cint, Cstring, Cint, Cint, Cint),
        host_name, Cint(sizeof(host_name)), port, Cint(docs), Cint(ipc_port), package_root_dir, Cint(sizeof(package_root_dir)), Cint(verbose), Cint(dev_mode)
    )
end

function server_is_running()::Bool
    return ccall((:LT_ServerIsRunning, LIT_SO), Cint, ())
end

function do_service_work()::Int
    return ccall((:LT_DoServiceWork, LIT_SO), Cint, ())
end

function stop_server()
    return ccall((:LT_StopServer, LIT_SO), Cvoid, ())
end

#---------------------------------

function __init__()
    # Check if the host system is supported.
    if !((Sys.islinux() && Sys.ARCH === :x86_64) || (Sys.iswindows() && Sys.ARCH === :x86_64))
        printstyled("Error: ", color=:red, bold=true)
        println("Currently, Lit.jl is only supported on Windows and Linux x86_64.")
        println("       Your platform: $(Sys.KERNEL) $(Sys.ARCH).")
    end
end

function main(args::Vector{String})
    cli = ArgParseSettings()

    @add_arg_table cli begin
        "script"
            help = "Entry point script"
            arg_type = String
            default = "app.jl"

        "--hostname", "-H"
            help = "Host name to bind to"
            arg_type = String
            default = "localhost"

        "--port", "-p"
            help = "Port number"
            arg_type = Int
            default = 3443

        "--docs", "-d"
            help = "Serve Lit.jl documentation at /docs/build/"
            action = :store_true

        "--dev", "-D"
            help = "Enable development mode"
            action = :store_true
    end

    parsed = parse_args(cli)

    if parsed["script"] != nothing
        start_app(parsed["script"]; host_name=parsed["hostname"], port=parsed["port"], docs=parsed["docs"], dev_mode=parsed["dev"])
    end
end

# For compatibility with older julia versions that didn't have @main
#-------------------------------------------------
if !@isdefined(var"@main")
    macro main(args...)
        if !isempty(args)
            error("USAGE: `@main` is expected to be used as `(@main)` without macro arguments.")
        end
        Core.eval(__module__, quote
            # Force the binding to resolve to this module
            global main
            global var"#__main_is_entrypoint__#"::Bool = true
        end)
        esc(:main)
    end
end

@main

end # module
