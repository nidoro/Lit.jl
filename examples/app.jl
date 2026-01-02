using Lit

@app_startup begin
    add_page("/")
    add_page("/counter")
    add_page("/avatar")
    add_page("/todo")
    add_page("/forecast")
    add_page("/seattle-weather")
    add_page("/fragment-example")
end

page_script = "01-counter.jl"
layout_style = "centered"

if is_on_page("/")
    page_script = @__FILE__
elseif is_on_page("/counter")
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

if is_on_page("/")
    @page_startup begin
        set_title("Lit Demo Apps | Lit.jl")
        set_description("Lit Demo Apps | Lit.jl")
    end

    column(fill_width=true, align_items="center") do
        h1("✨ Lit.jl Demo Apps")
        space(height="1rem")

        text("A few simple apps that showcase Lit.jl features!")
        text("Each one contains its own source code for you to explore.")

        cols = columns(2, show_border=true, padding=".8rem", justify_content="space-between")

        cols(1) do
            h5("Counter")
            text("Basic interactivity")
            link("Open", "/counter", style="primary")
        end

        cols(2) do
            h5("To-Do List")
            text("text_input and checkbox")
            link("Open", "/todo", style="primary")
        end

        cols = columns(2, show_border=true, padding=".8rem", justify_content="space-between")

        cols(1) do
            h5("Avatar Creator")
            text("image, selectbox, color_picker and custom fonts")
            link("Open", "/avatar", style="primary")
        end

        cols(2) do
            h5("Brazil Forecast")
            text("Interactive dataframe, image and integration with other libraries")
            link("Open", "/forecast", style="primary")
        end

        cols = columns(2)

        cols(1) do
            column(show_border=true, padding=".8rem", justify_content="space-between") do
                h5("Seattle Weather")
                text("Dashboard with metric, checkboxes, plots and dataframe")
                link("Open", "/seattle-weather", style="primary")
            end
        end

        space(height="1rem")
        h6("Other resources")

        row() do
            link("Getting Started", "/docs/build/docs/getting-started/install", style="naked", new_tab=true)
            link("API Reference", "/docs/build/docs/category/api-reference", style="naked", new_tab=true)
            link("GitHub", "https://github.com/nidoro/Lit.jl", style="naked", new_tab=true)
        end
    end
else
    include(page_script)
end

left_sidebar() do
    column(fill_width=true, gap="0px") do
        space(height="3rem")
        h5("Lit.jl Demo Apps", css=Dict("margin" => "0 0 .8rem .8rem", "white-space" => "nowrap", "color" => "#444"))
        link("Overview", "/", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
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
