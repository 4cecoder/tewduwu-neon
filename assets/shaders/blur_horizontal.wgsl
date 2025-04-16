// Horizontal Gaussian blur shader
// Used in the bloom/glow effect pipeline

// Bind group layout
@group(0) @binding(0) var input_texture: texture_2d<f32>;
@group(0) @binding(1) var input_sampler: sampler;
@group(0) @binding(2) var output_texture: texture_storage_2d<rgba8unorm, write>;

// Vertex shader
@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> @builtin(position) vec4<f32> {
    // Generate a full-screen triangle
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(3.0, -1.0),
        vec2<f32>(-1.0, 3.0)
    );

    return vec4<f32>(positions[vertex_index], 0.0, 1.0);
}

// Fragment shader
@fragment
fn fs_main(@builtin(position) frag_coord: vec4<f32>) -> @location(0) vec4<f32> {
    // Get UV coordinates
    let size = textureDimensions(input_texture);
    let uv = frag_coord.xy / vec2<f32>(f32(size.x), f32(size.y));
    
    // Gaussian kernel weights (1D)
    let weights = array<f32, 9>(
        0.051, 0.0918, 0.1231, 0.1353, 0.1772, 0.1353, 0.1231, 0.0918, 0.051
    );
    
    // Pixel size for offset calculation
    let pixel_size = 1.0 / f32(size.x);
    
    // Apply horizontal blur
    var color = vec4<f32>(0.0, 0.0, 0.0, 0.0);
    
    for (var i = -4; i <= 4; i = i + 1) {
        let offset = vec2<f32>(f32(i) * pixel_size, 0.0);
        let sample_uv = uv + offset;
        let sample = textureSample(input_texture, input_sampler, sample_uv);
        color = color + sample * weights[i + 4];
    }
    
    return color;
} 