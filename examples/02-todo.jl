using Lit

@register mutable struct Item
    name::String
    status::Bool
end

@register mutable struct Session
    items::Vector{Item}
end

function delete_item(i::Int)
    session = get_session_data()
    deleteat!(session.items, i)
end

function toggle_item(new_value::Bool, i::Int)
    session = get_session_data()
    session.items[i].status = new_value
end

function delete_all_checked()
    session = get_session_data()
    filter!(item -> !item.status, session.items)
end

@page_startup begin
    set_title("To-Do List | Lit.jl Demo")
    set_description("To-Do List | Lit.jl Demo")
end

@session_startup begin
    session = Session([
        Item("Buy milk", false),
        Item("Wash dishes", false),
        Item("Learn Julia", false)
    ])

    set_session_data(session)
end

session = get_session_data()

set_page_layout(
    style="centered",
    left_sidebar_initial_state="open",
    right_sidebar_initial_state="closed",
    right_sidebar_position="overlay",
    right_sidebar_initial_width="50%",
    right_sidebar_toggle_labels=(
        "VIEW SOURCE <lt-icon lt-icon='material/code'></lt-icon>",
        nothing
    )
)

main_area() do
    h1("To-do list", icon="material/checklist", icon_color="green")

    row(fill_width=true, align_items="flex-end") do
        new_item_name = text_input("Text input", fill_width=true, placeholder="Add to-do item", show_label=false, id="input_new_item")
        if button("Add", style="primary", icon="material/add") && length(new_item_name) > 0
            push!(session.items, Item(new_item_name, false))
            set_value("input_new_item", "")
        end
    end

    if length(session.items) > 0
        column(fill_width=true, show_border=true, padding="0.8rem") do
            for (i, item) in enumerate(session.items)
                row(fill_width=true, align_items="center", justify_content="space-between") do
                    checkbox(item.name, initial_value=item.status, onchange=toggle_item, args=[i])
                    button("", icon="material/delete", style="naked", onclick=delete_item, args=[i])
                end
            end
        end

        button("Delete all checked", style="naked", icon="material/delete_forever", onclick=delete_all_checked)
    else
        text("No pending items! Good job!")
    end
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
