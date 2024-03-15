# 🌋Hello Engine🖖🏽

A real-time rendering engine built from scratch using Vulkan API and C++.

### Features
* __Clustered forward shading__ for efficient light culling.
* __Physically-Based Rendering__ (PBR) with Cook-Torrance BRDF.
* __Image-Based Lighting__ (IBL) with offscreen pipelines to generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
* Compute shader to generate BRDF lookup table.
* __Bindless textures__, all textures needed for rendering the entire scene are bound just once at the start of the frame.
* __Shadow maps__ with Poisson Disk or PCF.
* glTF mesh/texture support.
* Multisample anti-aliasing (MSAA).
* Simple __raytracing__ pipeline with basic intersection testing.
* __Buffer device address__ for direct shader access to buffers without the need to create descriptors.
* Programmable vertex pulling.
* Reinhard tonemap postprocessing.
* Automatic runtime compilation from GLSL to SPIR-V using `glslang`.
* Lightweight abstraction layer on top of Vulkan for faster development.
* Minor features: skybox, instancing, ImGui, SSBOs, UBOs, and push constants.

### Gallery

The images below showcase examples of PBR, IBL, bindless textures, and PCF shadow mapping.

<img width="850" alt="bindless_shadow_mapping_1" src="https://github.com/azer89/HelloVulkan/assets/790432/c926d003-8df2-464e-a8f7-e04b66494214">

<img width="850" alt="bindless_shadow_mapping_2" src="https://github.com/azer89/HelloVulkan/assets/790432/7111e3f7-51e2-47fa-9fad-a0a19b4a1f1b">

Bindless textures is achieved by utilizing __Descriptor Indexing__. This enables the storage of all scene textures inside an unbounded array, which allows the texture descriptors to be bound once at the start of a frame. To push the "bindless" approach further, the rendering uses a feature called __buffer device address__. Instead of creating descriptors, device addresses act as _pointers_ so that the shaders can have direct access to buffers.

</br>
</br>
The image below is another example of realistic rendering of the damaged helmet demonstrating PBR and IBL techniques.

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910

### Clustered Forward Shading

The technique consists of two steps that are done in compute shaders. The first step is to subdivide the view frustum into AABB clusters.
The next step is light culling, where it calculates lights that intersect the clusters. This step removes lights that are too far from a fragment, leading to reduced light iteration in the final fragment shader.

Preliminary testing using a 3070M graphics card shows the technique can render a PBR Sponza scene in 2560x1600 resolution with over 1000 dynamic lights at 60-100 FPS.
If too many lights end up inside the view frustum, especially when zooming out, there may be a drop in frame rate, but still much faster than a naive forward shading.

https://github.com/azer89/HelloVulkan/assets/790432/13a4426f-deec-40f5-816a-5594f8f0cbc0

### Cascade Shadow Maps

<img width="850" alt="cascade_shadow_mapping" src="https://github.com/azer89/HelloVulkan/assets/790432/1634a491-ea8f-49f0-8214-766a038bedd1">

The left image is a rendering that uses four cascade shadow maps, resulting in sharper shadows. The right image above showcases the individual cascades with color coding. Poisson disk sampling helps to reduce projective aliasing artifacts, but can create more noticeable seams between cascades with excessive blurring. Additionally, camera movement causes shimmering edges due to the constantly updating shadow maps. 

### Hardware Raytracing
<img width="425" alt="hardware_raytracing" src="https://github.com/azer89/HelloVulkan/assets/790432/7f6771b3-ab52-41c4-89d4-b3bed05e724e">

The engine also features a basic ray tracing pipeline. This process begins with building Bottom Level Acceleration Structures (BLAS) and Top Level Acceleration Structures (TLAS). For each pixel on the screen, a ray is cast and intersected with the acceleration structures to determine the final color. However, light bouncing, a key feature for realistic lighting effects, is not yet implemented.

### [Link to some other cool results](https://github.com/azer89/HelloVulkan/wiki/Gallery)

### Build
* C++20
* Vulkan 1.3
* Dependencies: assimp, glm, glfw, ImGui, stb, tracy, volk, and VMA.
* [Link to build instruction](https://github.com/azer89/HelloVulkan/wiki/Build-Instruction)

### Credit
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)
* [Angel Ortiz - Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
* [The internet](https://github.com/azer89/GraphicsResources)
