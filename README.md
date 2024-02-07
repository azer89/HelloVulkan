# 🌋Hello Engine🖖🏽

A real-time rendering engine using Vulkan.

### Features
* Clustered Forward Shading, no more deferred!
* Physically-Based Rendering (PBR) with Cook-Torrance BRDF.
* Image-Based Lighting (IBL) with offscreen pipelines that generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
    * Compute shader to generate BRDF lookup table.
* Reinhard tonemap postprocessing.
* glTF mesh/texture loading and rendering.
* Multisample anti-aliasing (MSAA).
* Automatic runtime compilation from GLSL to SPIR-V.
* A a lightweight abstraction layer that encapsulates Vulkan API for rapid prototyping/development.
* Minor features: Skybox, instancing with SSBOs, ImGui, UBOs, and push constants.
  
https://github.com/azer89/HelloVulkan/assets/790432/8b0562ed-ab72-4e93-9ce9-31c61c0e986a

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910

<img width="850" alt="vulkan_tachikoma" src="https://github.com/azer89/HelloVulkan/assets/790432/535a2d75-fffd-436f-bf18-df18968b79e0">

<img width="850" alt="freya" src="https://github.com/azer89/HelloVulkan/assets/790432/c3dd2921-b46a-458c-af26-fa49fecc884b">

### Clustered Forward Shading

I finally implemented "Clustered Forward Shading" which is based on an article by Angel Ortiz. 

The current implementation consists of two steps. The first step involves subdividing the view frustum into clusters.
The next step is light culling, where we calculate lights that intersect the clusters. This step removes lights that are too far from a fragment, leading to reduced light iteration inside the final fragment shader.

Although I haven't done a proper evaluation, a Sponza scene with 1000+ point lights can be rendered around 60-100 FPS, tested using a 3060M graphics card.
If too many lights end up inside the view frustum, especially when you zoom out, the frame rate will drop regardless, but still much faster than a naive forward shading.

https://github.com/azer89/HelloVulkan/assets/790432/66b9a30a-d187-495a-9879-8eb11a497087

### Build
* C++20
* Vulkan 1.3
* Visual Studio 2022
* Dependencies are located in folder `External/` 

### Credit
Technical Resources:
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)
* [Angel Ortiz - Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
* A lot of [internet resources](https://github.com/azer89/VulkanResources)

Assets:
* [Tachikoma](https://sketchfab.com/3d-models/tachikoma-7ec03deb78de4a1b908d2bc736ff0f15)
* [Freya](https://sketchfab.com/3d-models/freya-crescent-6d8eae57c17f4a81a23301ee0afda8cf)
* [Polyhaven](https://polyhaven.com/)
* [glTF Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
