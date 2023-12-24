# Vulkan PBR 🌋

An implementation of physically-based lighting model to generate realistic rendering in real time.

### Features
* Skybox and mesh rendering.
* Offscreen rendering to generate a cubemap from an equirectangular HDR image.
* Offscreen rendering to generate both specular and diffuse maps.
* Automatic runtime compilation from GLSL to SPIR-V for convenient development process.

https://github.com/azer89/HelloVulkan/assets/790432/e0844ac4-eaaa-4f52-8155-06937183bec7

<img width="850" alt="Dragon" src="https://github.com/azer89/HelloVulkan/assets/790432/75930cb9-0855-4ae1-afe9-5a95aa8db93d">


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
