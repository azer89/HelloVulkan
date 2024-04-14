# 🌋Hello Engine🖖🏽

A 3D rendering engine built from scratch using Vulkan API and C++.

</br>

### Features

* __Clustered Forward Shading__ for efficient light culling.
* __Physically-Based Rendering__ (PBR) with Cook-Torrance microfacet and __Image-Based Lighting__ (IBL).
* __Hardware-Accelerated Path Tracing__ that can simulate indirect shading, reflections, and soft shadow.
* __Bindless__ techniques using __Indirect Draw__, __Descriptor Indexing__, and __Buffer Device Address__.
* __Compute-Based Frustum Culling__.
* __Compute-Based Skinning__ for skeletal animation.
* Shadow Maps with Poisson Disk or PCF.
* Screen Space Ambient Occlusion (SSAO).
* Multisample anti-aliasing (MSAA).
* Tonemap postprocessing.
* Automatic runtime compilation from GLSL to SPIR-V using `glslang`.
* Lightweight abstraction layer on top of Vulkan for faster development.
* Additional features: skybox, infinite grid, line rendering, and ImGui / ImGuizmo.

</br>

### Engine Overview

The engine leverages several modern GPU features to optimize rendering performance. First, bindless textures is achieved by utilizing __descriptor Indexing__. This enables the storage of all scene textures inside an unbounded array, which allows texture descriptors to be bound once at the start of a frame. 

Next, the engine takes advantage of __indirect draw__ API. The CPU prepares indirect draw commands and stores them in an indirect buffer. These draw commands are then sorted by material type. This allows the rendering process to be divided into separate render passes for each material. This improves efficiency by avoiding shader branching.

Finally, the engine pushes the concept of "bindless" even further by utilizing __buffer device addresses__. Instead of creating descriptors, device addresses act as _pointers_ so that the shaders can have direct access to buffers.

<img width="850" alt="bindless_shadow_mapping_1" src="https://github.com/azer89/HelloVulkan/assets/790432/03200177-9bc7-45c3-be0a-093286c6eef9">

</br>
</br>

### Hardware-Accelerated Path Tracing

The path tracing process begins with building acceleration structures containing multiple geometries. After creating a raytracing pipeline, the ray simulation requires several shaders. __Ray generation shader__ is responsible to generate rays, and add the hit color into an accumulator image. The final rendering is obtained by averaging the accumulator image. The next one is __Closest hit shader__ that determines the color when a ray intersects an object and can also scatter the ray for further bounces. Optionally, __Any hit shader__ is used to discard a ray hit in order to render transparent materials such as foliage textures. 

<img width="850" alt="hardware_raytracing" src="https://github.com/azer89/HelloVulkan/assets/790432/c678d37a-d808-425b-830f-82ec24df7d8d">

<img width="850" alt="hardware_raytracing" src="https://github.com/azer89/HelloVulkan/assets/790432/0b6f368f-943d-4f24-b129-6d8f9af9763b">

</br>
</br>

### Clustered Forward Shading

The technique consists of two steps that are executed in compute shaders. The first step is to subdivide the view frustum into AABB clusters.
The next step is light culling, where it calculates lights that intersect the clusters. This step removes lights that are too far from a fragment, leading to reduced light iteration in the final fragment shader.

Preliminary testing using a 3070M graphics card shows the technique can render a PBR Sponza scene in 2560x1600 resolution with over 1000 dynamic lights at 60-100 FPS.
If too many lights end up inside the view frustum, especially when zooming out, there may be a drop in frame rate, but still much faster than a naive forward shading.

https://github.com/azer89/HelloVulkan/assets/790432/13a4426f-deec-40f5-816a-5594f8f0cbc0

</br>

### Compute-Based Frustum Culling

Since the engine uses indirect draw, frustum culling can now be done entirely on the compute shader by modifying draw commands within an indirect buffer. If an object's AABB falls outside the camera frustum, the compute shader will deactivate the draw command for that object. Consequently, the CPU is unaware of the number of objects actually drawn. Using Tracy profiler, an intersection test with 10,000 AABBs only takes less than 25 microseconds (0.025 milliseconds).

The left image below shows a rendering of all objects inside the frustum. The right image shows visualizations of AABBs as translucent boxes and the frustum drawn as orange lines.

<img width="850" alt="frustum_culling" src="https://github.com/azer89/HelloVulkan/assets/790432/d42100a6-f95d-41ec-8e53-012b22e4175b">

</br>
</br>

### Compute-Based Skinning

The compute-based skinning approach is much simpler than the traditional vertex shader skinning. This is because the skinning computation is done only once using a compute shader at the beginning of the frame. The resulting skinned vertices are then stored in a buffer, enabling reuse for subsequent render passes like shadow mapping and lighting. Consequently, there is no need to modify existing pipelines and no extra shader permutations.

https://github.com/azer89/HelloVulkan/assets/790432/51f097f5-b361-4de9-9f04-99c511900f8d

</br>
</br>

### Cascade Shadow Maps

The left image below is a rendering that uses four cascade shadow maps, resulting in sharper shadows. The right image above showcases the individual cascades with color coding. Poisson disk sampling helps to reduce projective aliasing artifacts, but can create more noticeable seams between cascades with excessive blurring. 

<img width="850" alt="cascade_shadow_mapping" src="https://github.com/azer89/HelloVulkan/assets/790432/1634a491-ea8f-49f0-8214-766a038bedd1">

</br>
</br>

### [Link to More Results](https://github.com/azer89/HelloVulkan/wiki/Gallery)

</br>

### Build
* C++20
* Vulkan 1.3
* Dependencies located in `External` folder
* [Link to build instruction](https://github.com/azer89/HelloVulkan/wiki/Build-Instruction)

</br>

### Credit
* [GPU-Driven Rendering](https://vkguide.dev/docs/gpudriven)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)
* [Angel Ortiz - Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
* [The internet](https://github.com/azer89/GraphicsResources)
