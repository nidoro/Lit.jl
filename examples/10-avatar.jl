using Lit
using Random

HAIR_OPTIONS      = ["bangs", "bowlCutHair", "braids", "bunHair", "curlyBob", "curlyShortHair", "froBun", "halfShavedHead", "mohawk", "shavedHead", "shortHair", "straightHair", "wavyBob"]
EYES_OPTIONS      = ["angry", "cheery", "confused", "normal", "sad", "sleepy", "starstruck", "winking"]
MOUTH_OPTIONS     = ["awkwardSmile", "braces", "gapSmile", "kawaii", "openedSmile", "openSad", "teethSmile", "unimpressed"]
ACCESSORY_OPTIONS = ["catEars", "clownNose", "faceMask", "glasses", "mustache", "sailormoonCrown", "sleepMask", "sunglasses"]

function to_csv(vec::Vector)::String
    return join(vec, ",")
end

function random_skin_color()
    return rand(["#8c5a2b", "#643d19", "#a47539", "#c99c62", "#e2ba87", "#efcc9f", "#f5d7b1", "#ffe4c0"])
end

function random_hex_color()
    rgb = rand(UInt8, 3)
    return "#" * join(string.(rgb, base=16, pad=2))
end

function randomize_options()
    set_value("slc_hair"     , rand([HAIR_OPTIONS; nothing]))
    set_value("clr_hair"     , random_hex_color())
    set_value("slc_eyes"     , rand([EYES_OPTIONS; nothing]))
    set_value("clr_skin"     , random_skin_color())
    set_value("slc_mouth"    , rand([MOUTH_OPTIONS; nothing]))
    set_value("slc_accessory", rand([ACCESSORY_OPTIONS; nothing]))
end

function generate_avatar_url()::String
    params = Dict(
        "seed" => "abc",
        "accessoriesProbability" => "100",
        "accessories" => get_value("slc_accessory"),
        "eyes" => get_value("slc_eyes"),
        "hair" => get_value("slc_hair"),
        "hairColor" => get_value("clr_hair"),
        "mouth" => get_value("slc_mouth"),
        "skinColor" => get_value("clr_skin"),
    )

    params["accessoriesProbability"] = params["accessories"] != nothing ? "100" : "0"

    if params["hairColor"] != nothing
        params["hairColor"] = replace(params["hairColor"], "#" => "")
    end

    if params["skinColor"] != nothing
        params["skinColor"] = replace(params["skinColor"], "#" => "")
    end

    url_search_params = ""

    for pair in params
        if pair.second != nothing
            url_search_params *= "&$(pair.first)=$(pair.second)"
        end
    end

    return "https://api.dicebear.com/7.x/big-smile/svg?$(url_search_params)"
end

@page_startup begin
    set_title("Avatar Creator | Lit.jl Demo")
    set_description("Avatar Creator using DiceBear | Lit.jl Demo")
    add_font("Pacifico", ".Lit/served-files/fonts/Pacifico-Regular.ttf")
    add_css_rule("""
        h1 {
            font-family: Pacifico;
        }
    """)
end

@session_startup begin
    set_default_value("slc_accessory", nothing)
    set_default_value("slc_eyes", "normal")
    set_default_value("slc_hair", "shavedHead")
    set_default_value("clr_hair", "#000000")
    set_default_value("slc_mouth", "unimpressed")
    set_default_value("clr_skin", "#ffffff")
end

h1("Avatar Creator", icon="material/auto_awesome")

img_slot = column()
randomize = button("Randomize", icon="material/replay") || is_session_first_pass()

column() do
    cols = columns(2)

    cols(1) do
        selectbox("Hair", HAIR_OPTIONS, fill_width=true, id="slc_hair")
        selectbox("Eyes", EYES_OPTIONS, fill_width=true, id="slc_eyes")
        selectbox("Mouth", MOUTH_OPTIONS, fill_width=true, id="slc_mouth")
    end

    cols(2) do
        color_picker("Hair Color", fill_width=true, id="clr_hair")
        color_picker("Skin Color", fill_width=true, id="clr_skin")
        selectbox("Accessory", ACCESSORY_OPTIONS, fill_width=true, id="slc_accessory")
    end
end

if randomize
    randomize_options()
end

img_src = generate_avatar_url()
img_slot.image(img_src, width=200, height=200, max_width="200px")

row(align_items="center", gap=".3rem", margin="2rem 0 0 0", css=Dict("opacity" => ".7", "font-size" => ".9rem")) do
    text("Check out")
    link("DiceBear", "http://dicebear.com", style="naked", new_tab=true)
end

