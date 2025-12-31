using Lit

@app_startup begin
    add_page(["/", "/counter"])
    add_page("/avatar")
    add_page("/todo")
    add_page("/seattle-weather")
end

if     is_on_page("/counter")
    include("01-counter.jl")
elseif is_on_page("/todo")
    include("02-todo.jl")
elseif is_on_page("/avatar")
    include("10-avatar.jl")
elseif is_on_page("/seattle-weather")
    include("15-seattle-weather.jl")
end
