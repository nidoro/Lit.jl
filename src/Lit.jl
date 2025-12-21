module Lit

using ArgParse

macro start(file_path::String="app.jl")
    impl_file = joinpath(@__DIR__, "LitImpl.jl")

    return esc(:(
        included = include($impl_file);
        invokelatest(included.start_lit, $file_path)
    ))
end

function main(args::Vector{String})
    global LIBLIT = Libdl.dlopen(LIT_SO)
    g.sessions = Dict{Ptr{Cvoid}, Session}()
    g.first_pass = true

    s = ArgParseSettings()

    @add_arg_table s begin
        "script"
        help = "Lit.jl script"
    end

    parsed = parse_args(s)

    if parsed["script"] != nothing
        start(parsed["script"])
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    main(ARGS)
end

@main

export @start

end # module
