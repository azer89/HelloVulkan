# 🌋Hello Engine🖖🏽

A real-time rendering engine built from scratch using Vulkan API and C++.

### Features
* __Clustered forward shading__.
* __Physically-Based Rendering__ (PBR) with Cook-Torrance BRDF.
* __Image-Based Lighting__ (IBL) with offscreen pipelines that generate:
    * A cubemap from an equirectangular HDR image.
    * Specular and diffuse cubemaps.
* Compute shader to generate BRDF lookup table.
* Utilized descriptor indexing to store an array of textures, allowing one descriptor binding per indirect draw call.
* __Cascade Shadow Maps__.
* Reinhard tonemap postprocessing.
* glTF mesh/texture loading and rendering.
* Multisample anti-aliasing (MSAA).
* Automatic runtime compilation from GLSL to SPIR-V using `glslang`.
* A a lightweight abstraction layer that encapsulates Vulkan API for rapid prototyping/development.
* Minor features: Basic raytracing pipeline, skybox, instancing, ImGui, SSBOs, UBOs, and push constants.

### Gallery

The images below shows a demo of PBR, IBL, shadow mapping, and bindless rendering.

<img width="850" alt="bindless_shadow_mapping_1" src="https://github.com/azer89/HelloVulkan/assets/790432/c926d003-8df2-464e-a8f7-e04b66494214">

<img width="850" alt="bindless_shadow_mapping_2" src="https://github.com/azer89/HelloVulkan/assets/790432/7111e3f7-51e2-47fa-9fad-a0a19b4a1f1b">

Bindless rendering is implemented using indirect draw and descriptor indexing. Bindless rendering enables the storage of all scene textures inside an unbounded array, consequently eliminating the need for descriptor rebinding for each draw call.

</br>
</br>
The image below is another example of realistic rendering of the damaged helmet demonstrating PBR and IBL techniques.

https://github.com/azer89/HelloVulkan/assets/790432/2f6ff30b-9276-4998-b6fd-259d130bf910

### Clustered Forward Shading

The technique consists of two steps that are done in compute shaders. The first step is to subdivide the view frustum into AABB clusters.
The next step is light culling, where it calculates lights that intersect the clusters. This step removes lights that are too far from a fragment, leading to reduced light iteration in the final fragment shader.

Preliminary testing using a 3070M graphics card shows the technique can render a PBR Sponza scene with over 1000 dynamic lights at 60-100 FPS.
If too many lights end up inside the view frustum, especially when zooming out, there may be a drop in frame rate, but still much faster than a naive forward shading.

https://github.com/azer89/HelloVulkan/assets/790432/13a4426f-deec-40f5-816a-5594f8f0cbc0

### [Link to some other cool results](https://github.com/azer89/HelloVulkan/wiki/Gallery)

### Build
* C++20
* Vulkan 1.3
* Visual Studio 2022
* Dependencies: glfw, ImGui, VMA, assimp, glm, stb, and volk.

### Credit
* [learnopengl.com PBR](https://learnopengl.com/PBR/Theory)
* [Epic Games PBR Notes](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
* [3D Graphics Rendering Cookbook](https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook)
* [Angel Ortiz - Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
* [The internet](https://github.com/azer89/GraphicsResources)
