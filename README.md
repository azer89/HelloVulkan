# Vulkan PBR 🌋

An implementation of Physically Based Rendering (PBR), a technique that seeks to realistically simulate the interaction of light with materials. The scene illumination is done by using Image-Based Lighting (IBL), which uses high dynamic range (HDR) images of real-world environments to capture nuanced lighting conditions and enhance the realism of 3D object rendering.

### Features
* Skybox and mesh rendering.
* Offscreen rendering to generate a cubemap from an equirectangular HDR image.
* Offscreen rendering to generate both specular and diffuse maps.
* Compute shader to generate BRDF lookup table.
* Automatic runtime compilation from GLSL to SPIR-V for convenient development process.

https://github.com/azer89/HelloVulkan/assets/790432/9386c4a0-1c6d-4298-8a1a-7590b44f487b

https://github.com/azer89/HelloVulkan/assets/790432/58010a6c-5a90-42e0-a37a-295fa03956e4

https://github.com/azer89/HelloVulkan/assets/790432/582930b8-7f00-427c-91d4-5de8f01d7acf

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
