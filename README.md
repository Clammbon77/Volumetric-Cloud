# Volumetric Cloud Rendering in OPENGL
Developed in Visual Studio as IDE and mainly based on OpenGL API. Relied on libraries: **glew**, **freeglut**, **glm**, **SOIL** and **assimp**.

* Skybox\
  Create a cubemap which is a cube with 6 faces textured and then let this cube warp the camera. The cube need to move together with the camera so that background will always be the texture of the cubemap.
* Deferred Rendering
  * 1st Pass: shadow mapping to get the shadow texture
  * 2nd Pass: gbuffer pass that buffer color, normal, depth and world position as texture
  * 3rd Pass: (post-processing / light pass) caculate the light information with the texture in gbuffer

* Ray Marching
  * How to render volumetric things?\
    When shading for each pixel in the screen, the pixel should be covered with color of cloud if occlusion of cloud happens in the pixel
  * How ray marching works?\
    For each pixel of the screen, cast a ray spreading from the camera center to the pixel and iterate through the generated ray step by step and check if the current point is in the cloud. The density is accumulated during the ray is marching and thus the total thickness of the cloud the ray has intersected during marching could be utilized to calculate the final color of the pixel in the end.

   
