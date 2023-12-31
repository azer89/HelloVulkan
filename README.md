# 🌋Vulkan PBR+IBL

An implementation of Physically Based Rendering (PBR), a technique that simulates the interaction of light with materials. The scene illumination is achieved through Image-Based Lighting (IBL), which utilizes HDR images to capture nuanced lighting conditions and enhance the realism of 3D object rendering.

### Features
* Cook-Torrance BRDF
* Offscreen rendering to generate:
    * A cubemap from an equirectangular HDR image
    * Specular and diffuse maps
* Compute shader to generate BRDF lookup table
* Tonemap postprocessing
* glTF mesh rendering and skybox
* Automatic runtime compilation from GLSL to SPIR-V

https://github.com/azer89/HelloVulkan/assets/790432/c6d659d2-a525-4ba7-93ff-c264054c73b9

https://github.com/azer89/HelloVulkan/assets/790432/582930b8-7f00-427c-91d4-5de8f01d7acf

<img width="850" alt="vulkan_sponza" src="https://github.com/azer89/HelloVulkan/assets/790432/bf2f27c4-86c3-4b75-a1f7-2f6674b2a3d1">


### PBR Workflow

<img width="500" alt="PBR Wrkflow" src="https://github.com/azer89/HelloVulkan/assets/790432/686699df-6c29-4efb-8102-858955afed55">

### Build
* C++20
* Vulkan 1.3
* Visual Studio 2022
* Dependencies are located in folder `External/` 

### Credit
Technical Resources:
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Google Filament](https://google.github.io/filament/Filament.html)
* [Khronos glTF PBR](https://github.com/SaschaWillems/Vulkan-glTF-PBR)
* [Khronos IBL Sampler](https://github.com/KhronosGroup/glTF-IBL-Sampler)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)

Assets:
* [Tachikoma](https://sketchfab.com/3d-models/tachikoma-7ec03deb78de4a1b908d2bc736ff0f15)
* [Polyhaven](https://polyhaven.com/)
* [glTF Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
