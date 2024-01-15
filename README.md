# 🌋Hello Engine🖖🏽

A real-time rendering engine using Vulkan.

### Features
* Physically-Based Rendering (PBR) with Cook-Torrance BRDF.
* Image-Based Lighting.
* Offscreen rendering to generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
* Reinhard tonemap postprocessing.
* Mesh loading and rendering.
* Multisample anti-aliasing (MSAA).
* Automatic runtime compilation from GLSL to SPIR-V.
* A simple abstraction layer that supports a sequence of render passes.
* Minor features: Skybox, instancing with SSBOs, ImGui, UBOs, and push constants.
  
https://github.com/azer89/HelloVulkan/assets/790432/8b0562ed-ab72-4e93-9ce9-31c61c0e986a

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910


<img width="850" alt="vulkan_sponza" src="https://github.com/azer89/HelloVulkan/assets/790432/544764b5-2bd0-4f47-9673-3a5b566e29e1">

<img width="850" alt="vulkan_tachikoma" src="https://github.com/azer89/HelloVulkan/assets/790432/535a2d75-fffd-436f-bf18-df18968b79e0">


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
* [Polyhaven](https://polyhaven.com/)
* [glTF Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
