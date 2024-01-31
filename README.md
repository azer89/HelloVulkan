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

This is a work-in-progress feature. The current unoptimized code uses compute shaders to generate AABB clusters and cull the lights in the scene. 
Achieving 60-70 FPS is possible with 1000 lights. However, the bottleneck is the light culling shader, 
as each invocation performs collision checks on all lights in a brute-force manner.

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
