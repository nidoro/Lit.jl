using Main.Lit
using Statistics
using Dates
using DataFrames
using VegaDatasets
using VegaLite
using Printf

@register mutable struct PageData
    full_df::DataFrame
    df_2014::DataFrame
    df_2015::DataFrame
end

@page_startup begin
    set_title("Seattle Weather | Lit.jl Demo")
    set_description("Seattle Weather | Lit.jl Demo")

    full_df = DataFrame(dataset("seattle-weather"))
    full_df.date = Date.(full_df.date)
    full_df.doy = dayofyear.(full_df.date)
    full_df.year = year.(full_df.date)
    full_df.month = month.(full_df.date)

    function rolling_mean(v, w)
        [i < w ? mean(v[1:i]) : mean(@view v[i-w+1:i]) for i in eachindex(v)]
    end

    full_df.wind_avg = similar(full_df.wind)

    for y in unique(full_df.year)
        idx = full_df.year .== y
        full_df.wind_avg[idx] = rolling_mean(full_df.wind[idx], 14)
    end

    df_2015 = full_df[year.(full_df.date) .== 2015, :]
    df_2014 = full_df[year.(full_df.date) .== 2014, :]

    set_page_data(PageData(full_df, df_2014, df_2015))
end

page_data = get_page_data()

max_temp_2015 = maximum(page_data.df_2015.temp_max)
max_temp_2014 = maximum(page_data.df_2014.temp_max)

min_temp_2015 = minimum(page_data.df_2015.temp_min)
min_temp_2014 = minimum(page_data.df_2014.temp_min)

max_wind_2015 = maximum(page_data.df_2015.wind)
max_wind_2014 = maximum(page_data.df_2014.wind)

min_wind_2015 = minimum(page_data.df_2015.wind)
min_wind_2014 = minimum(page_data.df_2014.wind)

max_prec_2015 = maximum(page_data.df_2015.precipitation)
max_prec_2014 = maximum(page_data.df_2014.precipitation)

min_prec_2015 = minimum(page_data.df_2015.precipitation)
min_prec_2014 = minimum(page_data.df_2014.precipitation)

weather_icons = Dict(
    "sun" => "☀️",
    "snow" => "☃️",
    "rain" => "💧",
    "fog" => "😶‍🌫️",
    "drizzle" => "🌧️",
)

set_page_layout(
    style="wide",
    left_sidebar_initial_state="open",
    right_sidebar_initial_state="closed",
    right_sidebar_position="overlay",
    right_sidebar_initial_width="50%",
    right_sidebar_toggle_labels=(
        "VIEW SOURCE <lt-icon lt-icon='material/code'></lt-icon>",
        nothing
    )
)

# Dashboard
#-------------------
main_area() do
    h1("Seattle Weather")

    space(height="2rem")
    h4("Summary of 2015")

    row(gap="1.5rem 3rem", margin="0 0 2rem 0", css=Dict("flex-wrap" => "wrap")) do
        metric("Max Temperature", @sprintf("%.0fºC", max_temp_2015), @sprintf("%.0fºC", max_temp_2015 - max_temp_2014))
        metric("Min Temperature", @sprintf("%.0fºC", min_temp_2015), @sprintf("%.0fºC", min_temp_2015 - min_temp_2014))

        metric("Max Precipitation", @sprintf("%.0f", max_prec_2015), @sprintf("%.0f", max_prec_2015 - max_prec_2014))
        metric("Min Precipitation", @sprintf("%.0f", min_prec_2015), @sprintf("%.0f", min_prec_2015 - min_prec_2014))

        metric("Max Wind", @sprintf("%.0f", max_wind_2015), @sprintf("%.0f", max_wind_2015 - max_wind_2014))
        metric("Min Wind", @sprintf("%.0f", min_wind_2015), @sprintf("%.0f", min_wind_2015 - min_wind_2014))

        df_counts = combine(groupby(page_data.df_2015, :weather), nrow => :count)
        sort!(df_counts, :count, rev=true)
        weather_name = df_counts.weather[1]

        metric("Most common weather", "$(weather_icons[weather_name]) $(weather_name)")

        df_counts = combine(groupby(page_data.df_2015, :weather), nrow => :count)
        sort!(df_counts, :count)
        weather_name = df_counts.weather[1]

        metric("Least common weather", "$(weather_icons[weather_name]) $(weather_name)")
    end

    @fragment begin
        # Years comparison
        #------------------------
        h4("Select years")
        selected_years = row().checkboxes("Years", ["2015", "2014", "2013", "2012"], initial_value=["2015", "2014", "2013", "2012"])
        selected_years = parse.(Int, selected_years)

        df = filter(row -> year(row.date) in selected_years, page_data.full_df)

        cols = columns([75,25], show_border=true, padding=".8rem")

        # Temperature
        #---------------------
        chart = df |>
            @vlplot(
                mark={:bar, width=1, opacity=0.7},
                width=1000,
                height=300,
                encoding={
                    x={
                        field=:doy,
                        type="quantitative",
                        title=nothing,
                        scale={
                            domain=[1, 365],
                            nice=false
                        },
                        axis={
                            values=[1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 365],
                            labelExpr="timeFormat(datetime(2001, 0, datum.value), '%b %d')"
                        }
                    },
                    y={field=:temp_max, title="temperature range (°C)"},
                    y2={field=:temp_min},

                    color={
                        field=:year,
                        type="nominal",
                        title=nothing,
                    },
                },
                config={
                    axis={labelAngle=0},
                    legend={orient="bottom"}
                }
            )

        chart_path = gen_resource_path("png")
        save(chart_path, chart)

        cols[1].h6("Temperature")
        cols[1].image(chart_path, fill_width=true)

        # Weather distribution
        #------------------------------
        weather_counts = combine(groupby(df, :weather), nrow => :count)

        chart = weather_counts |>
            @vlplot(
                mark={:arc, innerRadius=0},
                width=300,
                height=300,
                encoding={
                    theta={field=:count, type="quantitative"},
                    color={
                        field=:weather,
                        type="nominal",
                        title=nothing,
                        scale={scheme="category10"},
                        legend={orient=:bottom},
                    },
                },
                view={stroke=:transparent},
            )

        chart_path = gen_resource_path("png")
        save(chart_path, chart)

        cols[2].h6("Weather Distribution")
        cols[2].image(chart_path, fill_width=true)

        # Wind
        #-------------
        cols = columns(2, show_border=true, padding=".8rem")

        chart = df |>
            @vlplot(
                width=900,
                height=300,
                mark={:line, interpolate="monotone"},
                encoding={
                    x={
                        field=:doy,
                        type="quantitative",
                        title="date",
                        scale={domain=[1, 365], nice=false},
                        axis={
                            values=[1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 365],
                            labelExpr="timeFormat(datetime(2001, 0, datum.value), '%b %d')"
                        }
                    },
                    y={
                        field=:wind_avg,
                        type="quantitative",
                        title="average wind past 2 weeks (m/s)"
                    },

                    color={
                        field=:year,
                        type="nominal",
                    }
                },
                config={
                    legend={orient="bottom"}
                }
            )

        chart_path = gen_resource_path("png")
        save(chart_path, chart)

        cols[1].h6("Wind")
        cols[1].image(chart_path, fill_width=true)

        # Precipitation
        #---------------------
        chart = df |>
            @vlplot(
                width=900,
                height=300,
                mark={:bar},
                encoding={
                    x={
                        field=:month,
                        type="ordinal",
                        title="date",
                        sort=1:12,
                        axis={
                            labelExpr="timeFormat(datetime(2001, datum.value-1, 1), '%b')"
                        }
                    },
                    y={
                        aggregate="sum",
                        field=:precipitation,
                        type="quantitative",
                        title="precipitation (mm)"
                    },
                    color={
                        field=:year,
                        type="nominal",
                        legend={title="year"}
                    }
                },
                config={
                    legend={orient="bottom"}
                }
            )

        chart_path = gen_resource_path("png")
        save(chart_path, chart)

        cols[2].h6("Precipitation")
        cols[2].image(chart_path, fill_width=true)

        # Monthly weather breakdown
        #---------------------------
        cols = columns(2, show_border=true, padding=".8rem")

        chart = df |>
            @vlplot(
                    mark = {:bar},
                    width=900,
                    height=300,
                    x = {
                        "month(date):o",
                        title = "month"
                    },
                    y = {
                        aggregate = "count",
                        type = "quantitative",
                        stack = "normalize",
                        title = "days"
                    },
                    color = {
                        field = "weather",
                        type = "nominal",
                        legend = {orient = "bottom"}
                    }
                )

        chart_path = gen_resource_path("png")
        save(chart_path, chart)

        cols[1].h6("Monthly Weather Breakdown")
        cols[1].image(chart_path, fill_width=true)

        # Raw data
        #----------------
        cols[2].h6("Raw Data")
        cols[2].dataframe(df)
    end # fragment

    space(height="200px")
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
