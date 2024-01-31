# 🌋Hello Engine🖖🏽

A real-time rendering engine using Vulkan.

### Features
* Physically-Based Rendering (PBR) with Cook-Torrance BRDF.
* Image-Based Lighting.
* Offscreen rendering to generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
    * Compute shader to generate BRDF lookup table.
* Reinhard tonemap postprocessing.
* Mesh loading and rendering.
* Multisample anti-aliasing (MSAA).
* Automatic runtime compilation from GLSL to SPIR-V.
* A simple abstraction layer that supports a sequence of render passes.
* Simple synchronization between compute queue and graphics queue.
* Minor features: Skybox, instancing with SSBOs, ImGui, UBOs, and push constants.
  
https://github.com/azer89/HelloVulkan/assets/790432/8b0562ed-ab72-4e93-9ce9-31c61c0e986a

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910

<img width="850" alt="vulkan_tachikoma" src="https://github.com/azer89/HelloVulkan/assets/790432/535a2d75-fffd-436f-bf18-df18968b79e0">

<img width="850" alt="freya" src="https://github.com/azer89/HelloVulkan/assets/790432/c3dd2921-b46a-458c-af26-fa49fecc884b">

### Clustered Forward Shading

I finally implemented "Clustered Forward Shading" algorithm into my Vulkan engine. It's based on a tutorial written by [Angel Ortiz](https://www.aortiz.me/2018/12/21/CG.html). So far, it can render a scene with 1000 lights at 60-80 FPS using a 3060M graphics card.

The first step is to subdivide the view frustum into clusters.

The next step is light culling: for each cluster, I calculate the intersecting lights. This way, I can remove lights that are too far from the view frustum, leading to reduced light iteration inside the final fragment shader.

It's still a simplified implementation. The light culling part is obviously the bottleneck since each compute shader invocation performs intersection test in a brute-force manner. 🥵

https://github.com/azer89/HelloVulkan/assets/790432/b1b3ff16-e1dc-4514-9688-789771531165

<img width="850" alt="vulkan_clustered_01" src="https://github.com/azer89/HelloVulkan/assets/790432/887ff61a-4883-449a-a476-ab4d54dcc39f">
<img width="850" alt="vulkan_clustered_02" src="https://github.com/azer89/HelloVulkan/assets/790432/dae33ee8-5df0-4ca0-a7ff-8ddc5a98d6ad">


### Build
* C++20
* Vulkan 1.3
* Visual Studio 2022
* Dependencies are located in folder `External/` 

### Credit
Technical Resources:
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Khronos glTF PBR](https://github.com/SaschaWillems/Vulkan-glTF-PBR)
* [Khronos IBL Sampler](https://github.com/KhronosGroup/glTF-IBL-Sampler)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)

Assets:
* [Tachikoma](https://sketchfab.com/3d-models/tachikoma-7ec03deb78de4a1b908d2bc736ff0f15)
* [Freya](https://sketchfab.com/3d-models/freya-crescent-6d8eae57c17f4a81a23301ee0afda8cf)
* [Polyhaven](https://polyhaven.com/)
* [glTF Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
