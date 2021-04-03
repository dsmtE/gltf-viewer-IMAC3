# GLTF Viewer Tutorial

This is the code repository for https://gltf-viewer-tutorial.gitlab.io/.

### How to run the program :

```
git clone https://gitlab.com/dsmtE/gltf-viewer
cd gltf-viewer
mkdir build
cd build
cmake ..
make -j
./bin/[Debug or Release]/gltf-viewer viewer ../../path/model.gltf
```

You can also use the provided cmd (in bash-init.sh) ***cmake_prepare*** and ***cmake_build*** to run and build the project.

### What i've done

- [x] Loading GLTF files
- [x] Controlling the Camera (blender like)
- [x] Directional Lighting
- [x] PBR and Physics Based Materials
- [x] Occlusion Mapping
- [x] Deferred Rendering 
- [x] Overlay mode visualization of all textures in GUI
- [x] SSAO Post Processing  (with settings in GUI)
- [x] Normal mapping (WIP - only use them when tangent data available)
- [x] Cube map IBL (without convolution)
- [x] cubeMap skybox
- [x] equirectangularHDR To Cubemap (for HDR IBL)
- [x] Bloom Post Processing(non HDR and really dirty just for training)

### What I would like to improve

- [ ] Create tangent buffer for normal mapping if missing
- [ ] Better HDR IBL  with Cubemap convolution and Specular IBL
- [ ] Better bloom processing (HDR and better pipeline with skybox and deferred shading)
- [ ] Shadow Mapping
- [ ] Multiple lights
- [ ]  Post processing - Depth of field

### What I learned from doing this project

It was really interesting to get into the subject of Deferred rendering, something I've wanted to try for a long time. I also got a better understanding of frameBuffer and multi texture rendering to be able to handle blur for example used in Bloom or DOF Post Processing.

I'm also happy to have been able to see the power of normal mapping in visual rendering, I understand better the issues related to optimization and simplification of meshes for real time rendering  and the interest of techniques like textures and normal baking.

### Options and GUI

It is possible to choose between two camera modes via the GUi trackball and FPS like.

It is also possible to set the different rendering parameters (SSAO, BLOOM, normal mapping, ...) as well as view the different textures used in the Deferred Rendering pipeline. 

It is possible to choose an overlay mode that displays all the textures simultaneously on the screen via the shading shader pass (which could be improved by using a dedicated shader).

### Difficulties

I had difficulties implementing normal mapping and especially when tangent vectors are not provided.
I tried, but I finally focused on deferred rendering, a subject I was more interested in studying.





