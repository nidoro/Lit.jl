module Lit

using ArgParse

function __init__()
    if !(Sys.islinux() && Sys.ARCH === :x86_64)
        printstyled("Error: ", color=:red, bold=true)
        println("Currently, Lit.jl is only supported on Linux x86_64.")
        println("       Your platform: $(Sys.KERNEL) $(Sys.ARCH).")
    end
end

macro start(file_path::String="app.jl", dev_mode::Bool=false)
    impl_file = joinpath(@__DIR__, "LitImpl.jl")

    return esc(:(
        included = include($impl_file);
        included.g.dev_mode = $dev_mode;
        invokelatest(included.start_lit, $file_path)
    ))
end

macro startdev(file_path::String="app.jl")
    return esc(:(@start($file_path, true)))
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

export @start, @startdev

end # module
