// Neon glow shader
// Creates a vibrant glow around UI elements

// Bind group layout
@group(0) @binding(0) var input_texture: texture_2d<f32>;
@group(0) @binding(1) var input_sampler: sampler;
@group(0) @binding(2) var<uniform> glow_color: vec4<f32>;
@group(0) @binding(3) var<uniform> glow_intensity: f32;
@group(0) @binding(4) var<uniform> glow_size: f32;

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
    let original = textureSample(input_texture, input_sampler, uv);
    
    // Calculate the amount of glow for this pixel
    var glow_amount = 0.0;
    let samples = 16; // Number of samples around the pixel
    
    for (var i = 0; i < samples; i = i + 1) {
        // Calculate angle and distance for this sample
        let angle = f32(i) * 6.28318 / f32(samples);
        let dist = glow_size / f32(size.x);
        
        // Calculate sample offset
        let offset_x = cos(angle) * dist;
        let offset_y = sin(angle) * dist;
        
        // Sample at the offset position
        let sample_uv = uv + vec2<f32>(offset_x, offset_y);
        let sample_color = textureSample(input_texture, input_sampler, sample_uv);
        
        // Add to glow amount based on the sample brightness
        glow_amount = glow_amount + max(
            dot(sample_color.rgb, vec3<f32>(0.2126, 0.7152, 0.0722)), 
            0.0
        );
    }
    
    // Average and apply intensity
    glow_amount = glow_amount / f32(samples) * glow_intensity;
    
    // Create the glow effect
    let glow = glow_color * glow_amount;
    
    // Add the glow to the original color
    let result = original + vec4<f32>(glow.rgb * glow.a, 0.0);
    
    // Apply a slight color shift for a more vibrant effect
    let final_color = vec4<f32>(
        result.r * 1.05,
        result.g * 1.02,
        result.b * 1.08,
        result.a
    );
    
    return final_color;
} 