using Magic
using Images
using ImageIO
using ImageFiltering
using ImageEdgeDetection
using Colors

mutable struct Session
    input_img::Union{UploadedFile, Nothing}
end

@session_startup begin
    session = Session(nothing)
    set_session_data(session)
end

function upl_image()
    session = get_session_data()
    session.input_img = get_value("upl_image")
end

session = get_session_data()

h1("Image Filters")

cols = columns(2)

cols(1) do
    file_uploader("Upload image", types=["image/*"], fill_width=true, onchange=upl_image, id="upl_image")
end

cols = columns(2)

cols(1) do
    if session.input_img !== nothing
        image(session.input_img.path)
    end
end

cols(2) do
    if session.input_img !== nothing
        img = load(session.input_img.path)
        gray_img = Gray.(img)

        gx = imfilter(gray_img, Kernel.sobel()[1])
        gy = imfilter(gray_img, Kernel.sobel()[2])

        gradient_magnitude = sqrt.(gx.^2 .+ gy.^2)
        normalized = gradient_magnitude ./ maximum(gradient_magnitude)

        # Apply contrast enhancement
        enhanced = normalized .* 2

        # Clamp to [0, 1] range
        result = clamp01nan.(enhanced)

        serveable_path = gen_serveable_path(session.input_img.extension)
        save(serveable_path, result)
        image(serveable_path)
    end
end
