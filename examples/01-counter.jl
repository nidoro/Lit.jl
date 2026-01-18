using Magic

@page_startup begin
    set_title("Counter | Magic.jl Demo")
    set_description("Counter | Magic.jl Demo")
end

@session_startup begin
    # Initialize the counter with 0
    set_session_data(0)
end

if button("Click me")
    # Increment the counter stored as the session data
    set_session_data(get_session_data()+1)
end

# Display the counter
text("Click count: $(get_session_data())")

text("View source 👉")

