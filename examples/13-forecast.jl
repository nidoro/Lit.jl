ENV["GKSwstype"]="nul"
using Magic
using Shapefile
using Plots
using Colors
using Printf
using DataFrames
using Downloads
using Tar
using CodecZlib

mutable struct SessionData
    data::DataFrame
    img_path::String
end

function get_polygon_rings(polygon::Shapefile.Polygon)
    npoints = length(polygon.points)
    nparts  = length(polygon.parts)
    rings   = Vector{Vector{Shapefile.Point}}()

    for i in 1:nparts
        # Shapefile.parts is zero-based, add 1 to convert to Julia indices
        start_idx = polygon.parts[i] + 1
        end_idx   = i < nparts ? polygon.parts[i+1] : npoints  # parts[i+1] is already 1-based
        # sanity check
        if start_idx >= 1 && end_idx <= npoints && start_idx <= end_idx
            push!(rings, polygon.points[start_idx:end_idx])
        else
            @warn "Skipping invalid ring indices" start_idx end_idx npoints
        end
    end
    return rings
end

function polygon_centroid(ring)
    x = [p.x for p in ring]
    y = [p.y for p in ring]
    n = length(x)
    A = 0.0
    Cx = 0.0
    Cy = 0.0

    for i in 1:n-1
        cross = x[i]*y[i+1] - x[i+1]*y[i]
        A  += cross
        Cx += (x[i] + x[i+1])*cross
        Cy += (y[i] + y[i+1])*cross
    end

    cross = x[n]*y[1] - x[1]*y[n]
    A  += cross
    Cx += (x[n] + x[1])*cross
    Cy += (y[n] + y[1])*cross

    A *= 0.5
    return (Cx/(6A), Cy/(6A))
end

function outlined_text!(x, y, label; fontsize=8, textcolor=:black, outlinecolor=:white, offset=0.5)
    for dx in [-offset, 0, offset], dy in [-offset, 0, offset]
        if dx != 0 || dy != 0
            annotate!(x + dx, y + dy, Plots.text(label, fontsize, outlinecolor, :center, "LiberationSans-Bold"))
        end
    end

    annotate!(x, y, Plots.text(label, fontsize, textcolor, :center, "LiberationSans-Bold"))
end

function temp_to_color(temp::Number)
    tmin, tmax = 15.0, 35.0
    α = clamp((temp - tmin)/(tmax-tmin), 0, 1)

    # Yellow → Orange → Red
    yellow = RGB(1.0, 0.9, 0.0)   # bright, warm yellow
    red    = RGB(0.9, 0.1, 0.1)   # vivid red

    return RGB(
        yellow.r*(1-α) + red.r*α,
        yellow.g*(1-α) + red.g*α,
        yellow.b*(1-α) + red.b*α
    )
end


function plot_map(temps_df::DataFrame, output_path::String)
    plot(
        legend = false,
        grid = false,
        framestyle = :none,
        xaxis = false,
        yaxis = false,
        size=(750,500),
        background_color=:transparent,
    )

    for row in shp
        polygon = row.geometry

        temp_row = temps_df[temps_df.State .== row.NAME_1, :]
        temp_val = temp_row.Temperature[1]

        color = temp_val !== missing ? temp_to_color(temp_val) : temp_to_color(0)

        rings = get_polygon_rings(polygon)

        for ring in rings
            xs = [p.x for p in ring]
            ys = [p.y for p in ring]
            plot!(xs, ys, seriestype=:shape, color=color, linecolor=:black)
        end

        # Use largest ring to compute centroid
        largest_ring = findmax(length.(rings))[2] |> i -> rings[i]
        cx, cy = polygon_centroid(largest_ring)

        if temp_val !== missing
            outlined_text!(cx, cy, @sprintf("%.0f°C", temp_val);
                        fontsize=10, textcolor=:darkred, outlinecolor=:white, offset=0.1)
        end
    end

    savefig(output_path);
end

function update_map()
    session = get_session_data()
    session.img_path = gen_resource_path("png")
    plot_map(session.data, session.img_path)
end

function ensure_shape_data()::Bool
    if isdir("data/gadm41_BRA_shp")
        return true
    end

    mkpath("data")
    url = "https://coisasdodavi.net/Magic/gadm41_BRA_shp.tar.gz"

    @info "Downloading Brazil shape data from $url..."
    temp_file = Downloads.download(url)

    try
        @info "Extracting to 'data/'..."
        open(temp_file, "r") do io
            Tar.extract(GzipDecompressorStream(io), "data")
        end

        @info "Extraction complete!"
        return true
    catch e
        return false
    finally
        rm(temp_file, force=true)
    end

    return true
end

@page_startup begin
    set_title("Brazil Forecast | Magic.jl Demo")
    set_description("Brazil Forecast | Magic.jl Demo")
    ensure_shape_data()
end

@session_startup begin
    shp = Shapefile.Table("data/gadm41_BRA_shp/gadm41_BRA_1.shp")

    data = DataFrame(
        State       = String[row.NAME_1 for row in shp],
        Temperature = Number[round(rand(15.0:0.1:35.0)) for row in shp]
    )

    set_session_data(SessionData(data, ""))
    update_map()
end

session = get_session_data()

h1("🇧🇷 Brazil")
h2("Temperature Forecast")
Magic.text("Double click a temperature below to change it")

row() do
    column_config = Dict(
        "Temperature" => Dict("editable" => true)
    )

    dataframe(session.data, column_config=column_config, id="df", onchange=update_map)
    image(session.img_path, fill_width=true)
end
