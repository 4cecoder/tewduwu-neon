// Post-processing renderer for bloom and glow effects
use wgpu::*;
use std::sync::Arc;
use bytemuck::{Pod, Zeroable};
use super::CyberpunkTheme;

// Define uniform buffer data structs with bytemuck
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct ExtractUniforms {
    threshold: f32,
    intensity: f32,
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct CompositeUniforms {
    intensity: f32,
    saturation: f32,
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct GlowUniforms {
    color: [f32; 4],
    intensity: f32,
    size: f32,
    _padding: [f32; 2], // Ensure 16-byte alignment
}

// BloomEffect handles the extraction, blur, and compositing for the bloom effect
pub struct BloomEffect {
    // Device and queue for operations
    device: Arc<Device>,
    queue: Arc<Queue>,
    
    // Render pipeline for each stage
    extract_pipeline: RenderPipeline,
    blur_h_pipeline: RenderPipeline,
    blur_v_pipeline: RenderPipeline,
    composite_pipeline: RenderPipeline,
    
    // Bind groups for each stage
    extract_bind_group: Option<BindGroup>,
    blur_h_bind_group: Option<BindGroup>,
    blur_v_bind_group: Option<BindGroup>,
    composite_bind_group: Option<BindGroup>,
    
    // Intermediate textures
    bright_texture: Option<Texture>,
    blur_h_texture: Option<Texture>,
    blur_v_texture: Option<Texture>,
    
    // Samplers
    sampler: Sampler,
    
    // Uniform buffers
    extract_uniform_buffer: Buffer,
    composite_uniform_buffer: Buffer,
    
    // Settings
    threshold: f32,
    intensity: f32,
    saturation: f32,
}

impl BloomEffect {
    pub fn new(device: Arc<Device>, queue: Arc<Queue>, format: TextureFormat) -> Self {
        // Create samplers for texture sampling
        let sampler = device.create_sampler(&SamplerDescriptor {
            label: Some("Bloom Sampler"),
            address_mode_u: AddressMode::ClampToEdge,
            address_mode_v: AddressMode::ClampToEdge,
            address_mode_w: AddressMode::ClampToEdge,
            mag_filter: FilterMode::Linear,
            min_filter: FilterMode::Linear,
            mipmap_filter: FilterMode::Linear,
            ..Default::default()
        });
        
        // Create uniform buffers
        let extract_uniform_buffer = device.create_buffer(&BufferDescriptor {
            label: Some("Bloom Extract Uniforms"),
            size: std::mem::size_of::<ExtractUniforms>() as u64,
            usage: BufferUsages::UNIFORM | BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        
        let composite_uniform_buffer = device.create_buffer(&BufferDescriptor {
            label: Some("Bloom Composite Uniforms"),
            size: std::mem::size_of::<CompositeUniforms>() as u64,
            usage: BufferUsages::UNIFORM | BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        
        // Load shader modules
        let extract_shader = device.create_shader_module(ShaderModuleDescriptor {
            label: Some("Bloom Extract Shader"),
            source: ShaderSource::Wgsl(include_str!("../shaders/extract_bright.wgsl").into()),
        });
        
        let blur_h_shader = device.create_shader_module(ShaderModuleDescriptor {
            label: Some("Horizontal Blur Shader"),
            source: ShaderSource::Wgsl(include_str!("../shaders/blur_horizontal.wgsl").into()),
        });
        
        let blur_v_shader = device.create_shader_module(ShaderModuleDescriptor {
            label: Some("Vertical Blur Shader"),
            source: ShaderSource::Wgsl(include_str!("../shaders/blur_vertical.wgsl").into()),
        });
        
        let composite_shader = device.create_shader_module(ShaderModuleDescriptor {
            label: Some("Bloom Composite Shader"),
            source: ShaderSource::Wgsl(include_str!("../shaders/bloom_composite.wgsl").into()),
        });
        
        // Create pipeline layouts
        let extract_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
            label: Some("Bloom Extract Layout"),
            bind_group_layouts: &[
                &Self::create_extract_bind_group_layout(&device),
            ],
            push_constant_ranges: &[],
        });
        
        let blur_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
            label: Some("Blur Layout"),
            bind_group_layouts: &[
                &Self::create_blur_bind_group_layout(&device),
            ],
            push_constant_ranges: &[],
        });
        
        let composite_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
            label: Some("Bloom Composite Layout"),
            bind_group_layouts: &[
                &Self::create_composite_bind_group_layout(&device),
            ],
            push_constant_ranges: &[],
        });
        
        // Create render pipelines
        let extract_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
            label: Some("Bloom Extract Pipeline"),
            layout: Some(&extract_layout),
            vertex: VertexState {
                module: &extract_shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(FragmentState {
                module: &extract_shader,
                entry_point: "fs_main",
                targets: &[Some(ColorTargetState {
                    format,
                    blend: Some(BlendState::ALPHA_BLENDING),
                    write_mask: ColorWrites::ALL,
                })],
            }),
            primitive: PrimitiveState {
                topology: PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });
        
        let blur_h_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
            label: Some("Horizontal Blur Pipeline"),
            layout: Some(&blur_layout),
            vertex: VertexState {
                module: &blur_h_shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(FragmentState {
                module: &blur_h_shader,
                entry_point: "fs_main",
                targets: &[Some(ColorTargetState {
                    format,
                    blend: Some(BlendState::ALPHA_BLENDING),
                    write_mask: ColorWrites::ALL,
                })],
            }),
            primitive: PrimitiveState {
                topology: PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });
        
        let blur_v_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
            label: Some("Vertical Blur Pipeline"),
            layout: Some(&blur_layout),
            vertex: VertexState {
                module: &blur_v_shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(FragmentState {
                module: &blur_v_shader,
                entry_point: "fs_main",
                targets: &[Some(ColorTargetState {
                    format,
                    blend: Some(BlendState::ALPHA_BLENDING),
                    write_mask: ColorWrites::ALL,
                })],
            }),
            primitive: PrimitiveState {
                topology: PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });
        
        let composite_pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
            label: Some("Bloom Composite Pipeline"),
            layout: Some(&composite_layout),
            vertex: VertexState {
                module: &composite_shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(FragmentState {
                module: &composite_shader,
                entry_point: "fs_main",
                targets: &[Some(ColorTargetState {
                    format,
                    blend: Some(BlendState::ALPHA_BLENDING),
                    write_mask: ColorWrites::ALL,
                })],
            }),
            primitive: PrimitiveState {
                topology: PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });
        
        // Set default settings
        let threshold = 0.7;
        let intensity = 0.5;
        let saturation = 1.1;
        
        // Update uniform buffers with initial values
        let extract_uniforms = ExtractUniforms {
            threshold,
            intensity,
        };
        
        let composite_uniforms = CompositeUniforms {
            intensity,
            saturation,
        };
        
        queue.write_buffer(
            &extract_uniform_buffer,
            0,
            bytemuck::cast_slice(&[extract_uniforms]),
        );
        
        queue.write_buffer(
            &composite_uniform_buffer,
            0,
            bytemuck::cast_slice(&[composite_uniforms]),
        );
        
        Self {
            device,
            queue,
            extract_pipeline,
            blur_h_pipeline,
            blur_v_pipeline,
            composite_pipeline,
            extract_bind_group: None,
            blur_h_bind_group: None,
            blur_v_bind_group: None,
            composite_bind_group: None,
            bright_texture: None,
            blur_h_texture: None,
            blur_v_texture: None,
            sampler,
            extract_uniform_buffer,
            composite_uniform_buffer,
            threshold,
            intensity,
            saturation,
        }
    }
    
    // Creates the bind group layout for the extract pass
    fn create_extract_bind_group_layout(device: &Device) -> BindGroupLayout {
        device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("Extract Bind Group Layout"),
            entries: &[
                BindGroupLayoutEntry {
                    binding: 0,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Texture {
                        sample_type: TextureSampleType::Float { filterable: true },
                        view_dimension: TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 1,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Sampler(SamplerBindingType::Filtering),
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 2,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::StorageTexture {
                        access: StorageTextureAccess::WriteOnly,
                        format: TextureFormat::Rgba8Unorm,
                        view_dimension: TextureViewDimension::D2,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 3,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 4,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
            ],
        })
    }
    
    // Creates the bind group layout for the blur passes
    fn create_blur_bind_group_layout(device: &Device) -> BindGroupLayout {
        device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("Blur Bind Group Layout"),
            entries: &[
                BindGroupLayoutEntry {
                    binding: 0,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Texture {
                        sample_type: TextureSampleType::Float { filterable: true },
                        view_dimension: TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 1,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Sampler(SamplerBindingType::Filtering),
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 2,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::StorageTexture {
                        access: StorageTextureAccess::WriteOnly,
                        format: TextureFormat::Rgba8Unorm,
                        view_dimension: TextureViewDimension::D2,
                    },
                    count: None,
                },
            ],
        })
    }
    
    // Creates the bind group layout for the composite pass
    fn create_composite_bind_group_layout(device: &Device) -> BindGroupLayout {
        device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("Composite Bind Group Layout"),
            entries: &[
                BindGroupLayoutEntry {
                    binding: 0,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Texture {
                        sample_type: TextureSampleType::Float { filterable: true },
                        view_dimension: TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 1,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Texture {
                        sample_type: TextureSampleType::Float { filterable: true },
                        view_dimension: TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 2,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Sampler(SamplerBindingType::Filtering),
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 3,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 4,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
            ],
        })
    }
    
    // Setup the bloom effect with the current screen size
    pub fn resize(&mut self, width: u32, height: u32) {
        // Create reduced resolution textures for the bloom effect
        // Using half resolution for better performance
        let bloom_width = width / 2;
        let bloom_height = height / 2;
        
        // Create bright extraction texture (half res)
        self.bright_texture = Some(self.device.create_texture(&TextureDescriptor {
            label: Some("Bright Texture"),
            size: Extent3d {
                width: bloom_width,
                height: bloom_height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: TextureDimension::D2,
            format: TextureFormat::Rgba8Unorm,
            usage: TextureUsages::TEXTURE_BINDING | TextureUsages::STORAGE_BINDING | TextureUsages::RENDER_ATTACHMENT,
            view_formats: &[],
        }));
        
        // Create horizontal blur texture (half res)
        self.blur_h_texture = Some(self.device.create_texture(&TextureDescriptor {
            label: Some("Horizontal Blur Texture"),
            size: Extent3d {
                width: bloom_width,
                height: bloom_height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: TextureDimension::D2,
            format: TextureFormat::Rgba8Unorm,
            usage: TextureUsages::TEXTURE_BINDING | TextureUsages::STORAGE_BINDING | TextureUsages::RENDER_ATTACHMENT,
            view_formats: &[],
        }));
        
        // Create vertical blur texture (half res)
        self.blur_v_texture = Some(self.device.create_texture(&TextureDescriptor {
            label: Some("Vertical Blur Texture"),
            size: Extent3d {
                width: bloom_width,
                height: bloom_height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: TextureDimension::D2,
            format: TextureFormat::Rgba8Unorm,
            usage: TextureUsages::TEXTURE_BINDING | TextureUsages::STORAGE_BINDING | TextureUsages::RENDER_ATTACHMENT,
            view_formats: &[],
        }));
    }
    
    // Update bloom settings
    pub fn update_settings(&mut self, threshold: f32, intensity: f32, saturation: f32) {
        self.threshold = threshold;
        self.intensity = intensity;
        self.saturation = saturation;
        
        // Update uniform buffers
        let extract_uniforms = ExtractUniforms {
            threshold,
            intensity,
        };
        
        let composite_uniforms = CompositeUniforms {
            intensity,
            saturation,
        };
        
        self.queue.write_buffer(
            &self.extract_uniform_buffer,
            0,
            bytemuck::cast_slice(&[extract_uniforms]),
        );
        
        self.queue.write_buffer(
            &self.composite_uniform_buffer,
            0,
            bytemuck::cast_slice(&[composite_uniforms]),
        );
    }
    
    // Apply the bloom effect
    pub fn apply(&self, encoder: &mut CommandEncoder, input_view: &TextureView, output_view: &TextureView) {
        // Skip if not initialized
        if self.bright_texture.is_none() 
          || self.blur_h_texture.is_none() 
          || self.blur_v_texture.is_none() {
            return;
        }
        
        // Get texture views
        let bright_view = self.bright_texture.as_ref().unwrap().create_view(&TextureViewDescriptor::default());
        let blur_h_view = self.blur_h_texture.as_ref().unwrap().create_view(&TextureViewDescriptor::default());
        let blur_v_view = self.blur_v_texture.as_ref().unwrap().create_view(&TextureViewDescriptor::default());
        
        // Create bind groups if not already created
        let extract_bind_group = self.device.create_bind_group(&BindGroupDescriptor {
            label: Some("Extract Bind Group"),
            layout: &self.extract_pipeline.get_bind_group_layout(0),
            entries: &[
                BindGroupEntry {
                    binding: 0,
                    resource: BindingResource::TextureView(input_view),
                },
                BindGroupEntry {
                    binding: 1,
                    resource: BindingResource::Sampler(&self.sampler),
                },
                BindGroupEntry {
                    binding: 2,
                    resource: BindingResource::TextureView(&bright_view),
                },
                BindGroupEntry {
                    binding: 3,
                    resource: self.extract_uniform_buffer.as_entire_binding(),
                },
                BindGroupEntry {
                    binding: 4,
                    resource: self.extract_uniform_buffer.as_entire_binding(),
                },
            ],
        });
        
        let blur_h_bind_group = self.device.create_bind_group(&BindGroupDescriptor {
            label: Some("Horizontal Blur Bind Group"),
            layout: &self.blur_h_pipeline.get_bind_group_layout(0),
            entries: &[
                BindGroupEntry {
                    binding: 0,
                    resource: BindingResource::TextureView(&bright_view),
                },
                BindGroupEntry {
                    binding: 1,
                    resource: BindingResource::Sampler(&self.sampler),
                },
                BindGroupEntry {
                    binding: 2,
                    resource: BindingResource::TextureView(&blur_h_view),
                },
            ],
        });
        
        let blur_v_bind_group = self.device.create_bind_group(&BindGroupDescriptor {
            label: Some("Vertical Blur Bind Group"),
            layout: &self.blur_v_pipeline.get_bind_group_layout(0),
            entries: &[
                BindGroupEntry {
                    binding: 0,
                    resource: BindingResource::TextureView(&blur_h_view),
                },
                BindGroupEntry {
                    binding: 1,
                    resource: BindingResource::Sampler(&self.sampler),
                },
                BindGroupEntry {
                    binding: 2,
                    resource: BindingResource::TextureView(&blur_v_view),
                },
            ],
        });
        
        let composite_bind_group = self.device.create_bind_group(&BindGroupDescriptor {
            label: Some("Composite Bind Group"),
            layout: &self.composite_pipeline.get_bind_group_layout(0),
            entries: &[
                BindGroupEntry {
                    binding: 0,
                    resource: BindingResource::TextureView(input_view),
                },
                BindGroupEntry {
                    binding: 1,
                    resource: BindingResource::TextureView(&blur_v_view),
                },
                BindGroupEntry {
                    binding: 2,
                    resource: BindingResource::Sampler(&self.sampler),
                },
                BindGroupEntry {
                    binding: 3,
                    resource: self.composite_uniform_buffer.as_entire_binding(),
                },
                BindGroupEntry {
                    binding: 4,
                    resource: self.composite_uniform_buffer.as_entire_binding(),
                },
            ],
        });
        
        // Step 1: Extract bright areas
        {
            let mut pass = encoder.begin_render_pass(&RenderPassDescriptor {
                label: Some("Bloom Extract Pass"),
                color_attachments: &[Some(RenderPassColorAttachment {
                    view: &bright_view,
                    resolve_target: None,
                    ops: Operations {
                        load: LoadOp::Clear(Color::BLACK),
                        store: StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            
            pass.set_pipeline(&self.extract_pipeline);
            pass.set_bind_group(0, &extract_bind_group, &[]);
            pass.draw(0..3, 0..1); // Full-screen triangle
        }
        
        // Step 2: Horizontal blur
        {
            let mut pass = encoder.begin_render_pass(&RenderPassDescriptor {
                label: Some("Horizontal Blur Pass"),
                color_attachments: &[Some(RenderPassColorAttachment {
                    view: &blur_h_view,
                    resolve_target: None,
                    ops: Operations {
                        load: LoadOp::Clear(Color::BLACK),
                        store: StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            
            pass.set_pipeline(&self.blur_h_pipeline);
            pass.set_bind_group(0, &blur_h_bind_group, &[]);
            pass.draw(0..3, 0..1); // Full-screen triangle
        }
        
        // Step 3: Vertical blur
        {
            let mut pass = encoder.begin_render_pass(&RenderPassDescriptor {
                label: Some("Vertical Blur Pass"),
                color_attachments: &[Some(RenderPassColorAttachment {
                    view: &blur_v_view,
                    resolve_target: None,
                    ops: Operations {
                        load: LoadOp::Clear(Color::BLACK),
                        store: StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            
            pass.set_pipeline(&self.blur_v_pipeline);
            pass.set_bind_group(0, &blur_v_bind_group, &[]);
            pass.draw(0..3, 0..1); // Full-screen triangle
        }
        
        // Step 4: Composite
        {
            let mut pass = encoder.begin_render_pass(&RenderPassDescriptor {
                label: Some("Bloom Composite Pass"),
                color_attachments: &[Some(RenderPassColorAttachment {
                    view: output_view,
                    resolve_target: None,
                    ops: Operations {
                        load: LoadOp::Load, // Load the existing content
                        store: StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            
            pass.set_pipeline(&self.composite_pipeline);
            pass.set_bind_group(0, &composite_bind_group, &[]);
            pass.draw(0..3, 0..1); // Full-screen triangle
        }
    }
}

// NeonGlowEffect creates a vibrant glow around UI elements
pub struct NeonGlowEffect {
    // Device and queue for operations
    device: Arc<Device>,
    queue: Arc<Queue>,
    
    // Render pipeline
    pipeline: RenderPipeline,
    
    // Bind group
    bind_group: Option<BindGroup>,
    
    // Sampler
    sampler: Sampler,
    
    // Uniform buffer
    uniform_buffer: Buffer,
    
    // Settings
    color: [f32; 4],
    intensity: f32,
    size: f32,
}

impl NeonGlowEffect {
    pub fn new(device: Arc<Device>, queue: Arc<Queue>, format: TextureFormat, theme: &CyberpunkTheme) -> Self {
        // Create sampler
        let sampler = device.create_sampler(&SamplerDescriptor {
            label: Some("Neon Glow Sampler"),
            address_mode_u: AddressMode::ClampToEdge,
            address_mode_v: AddressMode::ClampToEdge,
            address_mode_w: AddressMode::ClampToEdge,
            mag_filter: FilterMode::Linear,
            min_filter: FilterMode::Linear,
            mipmap_filter: FilterMode::Linear,
            ..Default::default()
        });
        
        // Create uniform buffer
        let uniform_buffer = device.create_buffer(&BufferDescriptor {
            label: Some("Neon Glow Uniforms"),
            size: std::mem::size_of::<GlowUniforms>() as u64,
            usage: BufferUsages::UNIFORM | BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        
        // Load shader module
        let shader = device.create_shader_module(ShaderModuleDescriptor {
            label: Some("Neon Glow Shader"),
            source: ShaderSource::Wgsl(include_str!("../shaders/neon_glow.wgsl").into()),
        });
        
        // Create bind group layout
        let bind_group_layout = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
            label: Some("Neon Glow Bind Group Layout"),
            entries: &[
                BindGroupLayoutEntry {
                    binding: 0,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Texture {
                        sample_type: TextureSampleType::Float { filterable: true },
                        view_dimension: TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 1,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Sampler(SamplerBindingType::Filtering),
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 2,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 3,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                BindGroupLayoutEntry {
                    binding: 4,
                    visibility: ShaderStages::FRAGMENT,
                    ty: BindingType::Buffer {
                        ty: BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
            ],
        });
        
        // Create pipeline layout
        let pipeline_layout = device.create_pipeline_layout(&PipelineLayoutDescriptor {
            label: Some("Neon Glow Pipeline Layout"),
            bind_group_layouts: &[&bind_group_layout],
            push_constant_ranges: &[],
        });
        
        // Create render pipeline
        let pipeline = device.create_render_pipeline(&RenderPipelineDescriptor {
            label: Some("Neon Glow Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: VertexState {
                module: &shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(FragmentState {
                module: &shader,
                entry_point: "fs_main",
                targets: &[Some(ColorTargetState {
                    format,
                    blend: Some(BlendState::ALPHA_BLENDING),
                    write_mask: ColorWrites::ALL,
                })],
            }),
            primitive: PrimitiveState {
                topology: PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });
        
        // Default settings
        let color = theme.cyan();
        let intensity = theme.glow_intensity();
        let size = 10.0;
        
        // Write initial uniform data
        let glow_uniforms = GlowUniforms {
            color,
            intensity,
            size,
            _padding: [0.0, 0.0], // Ensure 16-byte alignment
        };
        
        queue.write_buffer(
            &uniform_buffer,
            0,
            bytemuck::cast_slice(&[glow_uniforms]),
        );
        
        Self {
            device,
            queue,
            pipeline,
            bind_group: None,
            sampler,
            uniform_buffer,
            color,
            intensity,
            size,
        }
    }
    
    // Update glow settings
    pub fn update_settings(&mut self, color: [f32; 4], intensity: f32, size: f32) {
        self.color = color;
        self.intensity = intensity;
        self.size = size;
        
        // Update uniform buffer
        let glow_uniforms = GlowUniforms {
            color,
            intensity,
            size,
            _padding: [0.0, 0.0],
        };
        
        self.queue.write_buffer(
            &self.uniform_buffer,
            0,
            bytemuck::cast_slice(&[glow_uniforms]),
        );
    }
    
    // Apply the neon glow effect
    pub fn apply(&self, encoder: &mut CommandEncoder, input_view: &TextureView, output_view: &TextureView) {
        // Create bind group
        let bind_group = self.device.create_bind_group(&BindGroupDescriptor {
            label: Some("Neon Glow Bind Group"),
            layout: &self.pipeline.get_bind_group_layout(0),
            entries: &[
                BindGroupEntry {
                    binding: 0,
                    resource: BindingResource::TextureView(input_view),
                },
                BindGroupEntry {
                    binding: 1,
                    resource: BindingResource::Sampler(&self.sampler),
                },
                BindGroupEntry {
                    binding: 2,
                    resource: self.uniform_buffer.as_entire_binding(),
                },
                BindGroupEntry {
                    binding: 3,
                    resource: self.uniform_buffer.as_entire_binding(),
                },
                BindGroupEntry {
                    binding: 4,
                    resource: self.uniform_buffer.as_entire_binding(),
                },
            ],
        });
        
        // Render pass
        let mut pass = encoder.begin_render_pass(&RenderPassDescriptor {
            label: Some("Neon Glow Pass"),
            color_attachments: &[Some(RenderPassColorAttachment {
                view: output_view,
                resolve_target: None,
                ops: Operations {
                    load: LoadOp::Load, // Load existing content
                    store: StoreOp::Store,
                },
            })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
        });
        
        pass.set_pipeline(&self.pipeline);
        pass.set_bind_group(0, &bind_group, &[]);
        pass.draw(0..3, 0..1); // Full-screen triangle
    }
}

// Export the module in mod.rs
pub mod prelude {
    pub use super::BloomEffect;
    pub use super::NeonGlowEffect;
} 