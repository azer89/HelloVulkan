# 🌋Hello Engine🖖🏽

A real-time rendering engine using Vulkan.

### Features
* Clustered Forward Shading, no more deferred!
* Physically-Based Rendering (PBR) with Cook-Torrance BRDF.
* Image-Based Lighting (IBL) with offscreen pipelines that generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
* Compute shader to generate BRDF lookup table.
* Shadow mapping.
* Reinhard tonemap postprocessing.
* glTF mesh/texture loading and rendering.
* Multisample anti-aliasing (MSAA).
* Automatic runtime compilation from GLSL to SPIR-V using `glslang`.
* A a lightweight abstraction layer that encapsulates Vulkan API for rapid prototyping/development.
* Minor features: Basic raytracing pipeline, skybox, instancing, ImGui, SSBOs, UBOs, and push constants.

<img width="850" alt="vulkan_tachikoma_shadow_mapping" src="https://github.com/azer89/HelloVulkan/assets/790432/9600c642-6331-43f8-80de-1967cd575f420">

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910

### Clustered Forward Shading

The technique consists of two steps. The first step involves subdividing the view frustum into clusters.
The next step is light culling, where it calculates lights that intersect the clusters. This step removes lights that are too far from a fragment, leading to reduced light iteration in the final fragment shader.

Preliminary testing using a 3070M graphics card shows the technique can render a PBR Sponza scene with over 1000 dynamic lights at 60-100 FPS.
If too many lights end up inside the view frustum, especially when zooming out, there may be a drop in frame rate, but still much faster than a naive forward shading.

https://github.com/azer89/HelloVulkan/assets/790432/13a4426f-deec-40f5-816a-5594f8f0cbc0

### [Link to some other cool results](https://github.com/azer89/HelloVulkan/blob/main/GALLERY.md)

### Build
* C++20
* Vulkan SDK
* Visual Studio 2022
* Dependencies are located in folder `External/` 

### Credit
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)
* [Angel Ortiz - Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
* [The internet](https://github.com/azer89/VulkanResources)
