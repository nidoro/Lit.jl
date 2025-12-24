using Main.Lit

@app_startup begin
    add_page(["/", "/counter"])
    add_page("/avatar")
    add_page("/todo")
    add_page("/seattle-weather")
end

if get_url_path() in ["/", "/counter"]
    include("01-counter.jl")
elseif get_url_path() == "/todo"
    include("02-todo.jl")
elseif get_url_path() == "/avatar"
    include("10-avatar.jl")
elseif get_url_path() == "/seattle-weather"
    include("15-seattle-weather.jl")
end
