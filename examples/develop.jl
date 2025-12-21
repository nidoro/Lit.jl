#!/home/davidoro/.juliaup/bin/julia

using Pkg
Pkg.activate(".")

function restart(script_path::String)::Nothing
    cp("/home/davidoro/my-libs/template/website-cpp/served-files/DD_Components.js", "../served-files/DD_Components.js", force=true)
    jt = include("../src/Lit.jl")
    invokelatest(jt.start, script_path)
    return nothing
end

restart("./examples.jl")
