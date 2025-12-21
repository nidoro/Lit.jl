using Main.Lit

@session_startup begin
    set_session_data(0)
end

if button("Click me")
    set_session_data(get_session_data()+1)
end

text("Click count: $(get_session_data())")
