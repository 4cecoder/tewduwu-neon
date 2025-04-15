// Extract bright areas shader
// This is the first step in the bloom pipeline

// Bind group layout
@group(0) @binding(0) var input_texture: texture_2d<f32>;
@group(0) @binding(1) var input_sampler: sampler;
@group(0) @binding(2) var output_texture: texture_storage_2d<rgba8unorm, write>;
@group(0) @binding(3) var<uniform> threshold: f32;
@group(0) @binding(4) var<uniform> intensity: f32;

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
    
    // Sample the input texture
    let color = textureSample(input_texture, input_sampler, uv);
    
    // Calculate brightness
    let brightness = dot(color.rgb, vec3<f32>(0.2126, 0.7152, 0.0722)); // Luminance formula
    
    // Only keep pixels brighter than the threshold
    var bright_color = vec4<f32>(0.0, 0.0, 0.0, 0.0);
    if (brightness > threshold) {
        // Apply soft threshold
        let soft_threshold = 0.1;
        let knee = threshold * soft_threshold;
        let soft = brightness - threshold + knee;
        soft = clamp(soft / (2.0 * knee), 0.0, 1.0);
        
        // Apply threshold with smoothing
        bright_color = color * soft * intensity;
    }
    
    return bright_color;
} 