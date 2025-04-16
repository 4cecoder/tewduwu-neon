// Bloom composite shader
// Combines the original scene with the bloom effect

// Bind group layout
@group(0) @binding(0) var original_texture: texture_2d<f32>;
@group(0) @binding(1) var bloom_texture: texture_2d<f32>;
@group(0) @binding(2) var texture_sampler: sampler;
@group(0) @binding(3) var<uniform> bloom_intensity: f32;
@group(0) @binding(4) var<uniform> bloom_saturation: f32;

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

// Helper function to adjust color saturation
fn adjust_saturation(color: vec3<f32>, saturation: f32) -> vec3<f32> {
    let luminance = dot(color, vec3<f32>(0.2126, 0.7152, 0.0722));
    return mix(vec3<f32>(luminance), color, saturation);
}

// Fragment shader
@fragment
fn fs_main(@builtin(position) frag_coord: vec4<f32>) -> @location(0) vec4<f32> {
    // Get UV coordinates
    let size = textureDimensions(original_texture);
    let uv = frag_coord.xy / vec2<f32>(f32(size.x), f32(size.y));
    
    // Sample both textures
    let original = textureSample(original_texture, texture_sampler, uv);
    let bloom = textureSample(bloom_texture, texture_sampler, uv);
    
    // Adjust bloom saturation
    let adjusted_bloom = vec4<f32>(
        adjust_saturation(bloom.rgb, bloom_saturation),
        bloom.a
    );
    
    // Combine original with bloom
    let result = original + (adjusted_bloom * bloom_intensity);
    
    // Add a subtle color shift to enhance the cyberpunk feel
    let glow_tint = vec3<f32>(1.1, 0.9, 1.2); // Slight purple tint
    let final_color = result.rgb * glow_tint;
    
    return vec4<f32>(final_color, result.a);
} 