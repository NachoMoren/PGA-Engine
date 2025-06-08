# PGA-Engine 
### by Nacho Moreno & Biel Rubio

![Main texture](GitHubResources/main_tex.png)

## Main features
- Forward rendering
- Deferred rendering
- Bloom Fx
- Water Fx

## Camera controls: 
The camera have the same controls as Unity camera

- WS used for zoom (camera front movement)
- AD used for right/left movement
- QE used for up/down movement
- With the mouse MIDDLE button you can drag and move the camera by clicking
- With the mouse MIDDLE button you can scroll for zoom
- With the mouse RIGHT button you can rotate the camera

## Editor parameters:

- Add primitive: Add a selected shape to the scene
- Add light: Add a selected light to the scene
- Select mode: Change between forward or deferred rendering (some implemntations are not available for forward)
- Select buffer: Change the texture attachment on display
- Bloom variables: Modify the parameters for the bloom Fx
- +Edit entities transform

## Shaders

- Geometry Render (shaders.glsl)
- Lighting Render (shaders.glsl)
- Blit Brightest  (BLIT_BRIGHTEST.gsls)
- Blur            (shaders.glsl)
- Bloom           (shaders.glsl)
- Show Lights     (shaders.glsl)
- Water effect    (shaders.glsl)
- Cubemap         (CUBEMAP.glsl)

## Gallery

![Main texture](GitHubResources/main_tex.png)
![NoBloom texture](GitHubResources/no_bloom_tex.png)
![Color texture](GitHubResources/color_tex.png)
![Normal texture](GitHubResources/normal_tex.png)
![Depth texture](GitHubResources/depth_tex.png)
![Position texture](GitHubResources/position_tex.png)
