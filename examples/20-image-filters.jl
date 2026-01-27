using Magic
using Images
using ImageIO
using ImageFiltering
using ImageEdgeDetection
using Colors
using ColorVectorSpace

mutable struct Session
    input_img::Union{UploadedFile, Nothing}
end

@session_startup begin
    session = Session(nothing)
    set_session_data(session)

    set_default_value("slc_filter", "Sketch")
end

function upl_image()
    session = get_session_data()
    session.input_img = get_value("upl_image")
end

session = get_session_data()

h1("Image Filters")

cols = columns(2, justify_content="flex-end")

cols(1) do
    file_uploader("Upload image", types=["image/*"], fill_width=true, onchange=upl_image, id="upl_image")
end

if session.input_img !== nothing
    cols(2) do
        selectbox("Filter", ["Sketch", "Sobel", "Oil Painting"], id="slc_filter")
    end

    cols = columns(2)

    cols(1) do
        column(fill_width=true, align_items="flex-end") do
            image(session.input_img.path)
        end
    end

    cols(2) do
        img = load(session.input_img.path)
        result = nothing

        if get_value("slc_filter") == "Sobel"
            gray_img = Gray.(img)

            gx = imfilter(gray_img, Kernel.sobel()[1])
            gy = imfilter(gray_img, Kernel.sobel()[2])

            gradient_magnitude = sqrt.(gx.^2 .+ gy.^2)
            normalized = gradient_magnitude ./ maximum(gradient_magnitude)

            # Apply contrast enhancement
            enhanced = normalized .* 2

            # Clamp to [0, 1] range
            result = clamp01nan.(enhanced)
        elseif get_value("slc_filter") == "Sketch"
            blur_sigma=10.0
            blend_intensity=1.0

            gray_img = Gray.(img)

            # Step 1: Invert the image
            inverted = 1.0 .- gray_img

            # Step 2: Apply Gaussian blur to the inverted image
            blurred = imfilter(inverted, Kernel.gaussian(blur_sigma))

            # Step 3: Color dodge blend mode
            # Formula: base / (1 - blend)
            # We need to avoid division by zero
            sketch = gray_img ./ (1.0 .- blurred .* blend_intensity .+ 1e-6)

            # Clamp to [0, 1] range
            result = clamp01nan.(sketch)
        elseif get_value("slc_filter") == "Oil Painting"
            radius=4
            intensity_levels=30

            # Convert to RGB if grayscale, keep as RGB if color
            rgb_img = RGB.(img)

            height, width = size(rgb_img)
            result = similar(rgb_img)

            # For each pixel, find the most common color in the neighborhood
            # based on intensity histogram
            for i in 1:height
                for j in 1:width
                    # Define neighborhood bounds
                    i_start = max(1, i - radius)
                    i_end = min(height, i + radius)
                    j_start = max(1, j - radius)
                    j_end = min(width, j + radius)

                    # Extract neighborhood
                    neighborhood = rgb_img[i_start:i_end, j_start:j_end]

                    # Calculate intensity for each pixel in neighborhood
                    # Using standard luminance formula: 0.299R + 0.587G + 0.114B
                    intensities = [0.299*px.r + 0.587*px.g + 0.114*px.b for px in neighborhood]

                    # Quantize intensities into bins
                    bins = round.(Int, intensities .* (intensity_levels - 1)) .+ 1
                    bins = clamp.(bins, 1, intensity_levels)

                    # Find the most frequent intensity bin
                    histogram = zeros(Int, intensity_levels)
                    for (idx, bin) in enumerate(bins)
                        histogram[bin] += 1
                    end

                    max_bin = argmax(histogram)

                    # Average all colors in the most frequent bin
                    r_sum, g_sum, b_sum = 0.0, 0.0, 0.0
                    count = 0

                    for (idx, bin) in enumerate(bins)
                        if bin == max_bin
                            px = neighborhood[idx]
                            r_sum += px.r
                            g_sum += px.g
                            b_sum += px.b
                            count += 1
                        end
                    end

                    # Set the result pixel to the average color
                    if count > 0
                        result[i, j] = RGB(r_sum/count, g_sum/count, b_sum/count)
                    else
                        result[i, j] = rgb_img[i, j]
                    end
                end
            end
        end

        serveable_path = gen_serveable_path(session.input_img.extension)
        save(serveable_path, result)
        image(serveable_path)
    end
end
