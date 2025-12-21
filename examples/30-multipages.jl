using Main.Lit

@app_startup begin
    add_page(["/", "/avatar"])
    add_page("/todo", title="To-Do List | Lit.jl Example", description="To-Do List | Lit.jl Example")
    add_page("/seattle-weather", title="Seattle Weather | Lit.jl Example", description="Seattle Weather Data Exploration | Lit.jl Example")
end

if get_url_path() in ["/", "/avatar"]
    include("10-avatar.jl")
elseif get_url_path() == "/todo"
    include("02-todo.jl")
elseif get_url_path() == "/seattle-weather"
    include("15-seattle-weather.jl")
end
