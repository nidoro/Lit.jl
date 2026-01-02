using Lit

@app_startup begin
    add_page(["/", "/counter"])
    add_page("/avatar")
    add_page("/todo")
    add_page("/forecast")
    add_page("/seattle-weather")
    add_page("/fragment-example")
end

page_script = "01-counter.jl"
layout_style = "centered"

if     is_on_page("/counter")
    page_script = "01-counter.jl"
elseif is_on_page("/todo")
    page_script = "02-todo.jl"
elseif is_on_page("/avatar")
    page_script = "10-avatar.jl"
elseif is_on_page("/forecast")
    page_script = "13-forecast.jl"
    layout_style = "wide"
elseif is_on_page("/seattle-weather")
    page_script = "15-seattle-weather.jl"
    layout_style = "wide"
elseif is_on_page("/fragment-example")
    page_script = "50-fragment-example.jl"
end

set_page_layout(
    style=layout_style,
    left_sidebar_initial_state="open",
    right_sidebar_initial_state="closed",
    right_sidebar_position="overlay",
    right_sidebar_initial_width="50%",
    right_sidebar_toggle_labels=(
        "VIEW SOURCE <lt-icon lt-icon='material/code'></lt-icon>",
        nothing
    )
)

include(page_script)

left_sidebar() do
    column(fill_width=true, gap="0px") do
        space(height="3rem")
        h5("Lit.jl Demo Apps", css=Dict("margin" => "0 0 .8rem .8rem", "white-space" => "nowrap", "color" => "#444"))
        link("Counter", "/counter", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("To-Do List", "/todo", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("Avatar Creator", "/avatar", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("Brazil Forecast", "/forecast", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("Seattle Weather", "/seattle-weather", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
    end
end

right_sidebar() do
    code(initial_value_file=page_script)
end
