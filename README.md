# Vulkan PBR 🌋

An implementation of physically-based lighting model to generate realistic rendering in real time.

### Features
* Skybox and mesh rendering.
* Offscreen rendering to generate a cubemap from an equirectangular HDR image.
* Offscreen rendering to generate specular and diffuse maps.
* Automatic runtime compilation from GLSL to SPIR-V.


https://github.com/azer89/HelloVulkan/assets/790432/10edb1a5-ff3f-4e15-8199-1b3aa1c92a62

<img width="850" alt="Dragon" src="https://github.com/azer89/HelloVulkan/assets/790432/0d1c1897-2006-448f-b391-c49d6ffaf8b1">


### Build
* C++20
* Vulkan 1.3
* Visual Studio 2022
* Dependencies are located in folder `External/` 

### Credit
Technical Resources:
* [learnopengl.com](https://learnopengl.com/)
* [Khronos IBL Sampler](https://github.com/KhronosGroup/glTF-IBL-Sampler)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)

Assets:
* [Polyhaven](https://polyhaven.com/)
* [glTF Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
