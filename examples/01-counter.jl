using Main.Lit

@page_startup begin
    set_title("Counter | Lit.jl Demo")
    set_description("Counter | Lit.jl Demo")
end

@session_startup begin
    # Initialize the counter with 0
    set_session_data(0)
end

set_page_layout(
    style="centered",
    left_sidebar_initial_state="open",
    right_sidebar_initial_state="closed",
    right_sidebar_position="overlay",
    right_sidebar_initial_width="50%",
    right_sidebar_toggle_labels=(
        "VIEW SOURCE <lt-icon lt-icon='material/code'></lt-icon>",
        nothing
    )
)

main_area() do
    if button("Click me")
        # Increment the counter stored as the session data
        set_session_data(get_session_data()+1)
    end

    # Display the counter
    text("Click count: $(get_session_data())")

    code("""
# SOURCE CODE
#------------
@session_startup begin
    # Initialize the counter with 0
    set_session_data(0)
end

if button("Click me")
    # Increment the counter stored as the session data
    set_session_data(get_session_data()+1)
end

# Display the counter
text("Click count: \$(get_session_data())")
""")
end

left_sidebar() do
    column(fill_width=true, gap="0px") do
        space(height="3rem")
        h5("Lit.jl Demo Apps", css=Dict("margin" => "0 0 .8rem .8rem", "white-space" => "nowrap", "color" => "#444"))
        link("Counter", "/counter", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("To-Do List", "/todo", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("Avatar Creator", "/avatar", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
        link("Seattle Weather", "/seattle-weather", style="naked", fill_width=true, css=Dict("justify-content" => "flex-start"))
    end
end

right_sidebar() do
    code(initial_value_file=@__FILE__)
end
