# Realtime Global Illumination with Voxel Cone Tracing

![alt text](https://raw.githubusercontent.com/righier/gidemo/master/screenshots/1.jpg)

video: https://www.youtube.com/watch?v=OcmgxbZ4vmA

Screenshots:
[1](https://raw.githubusercontent.com/righier/gidemo/master/screenshots/1.jpg)
[2](https://raw.githubusercontent.com/righier/gidemo/master/screenshots/2.jpg)
[3](https://raw.githubusercontent.com/righier/gidemo/master/screenshots/3.jpg)
[4](https://raw.githubusercontent.com/righier/gidemo/master/screenshots/4.jpg)

In this demo I implemented realtime global illumination using the voxel cone tracing technique (https://doi.org/10.1111/j.1467-8659.2011.02063.x)

I then improved on the original technique by propagating light temporally across frames, which approximates the effect of simulating infinite light bounces.

For more information check out the [report](https://raw.githubusercontent.com/righier/gidemo/master/report.pdf).

**Features:**
- Diffuse Cones
- Specular Cones
- Shadow Cones
- Emissive materials
- HDR RGBA16f storage
- Anisotropic voxels
- Light temporal multi-bounce
- Particle systems

**Commands**
- **WASD**: Horizontal movement
- **QE** Vertical movement
- **SPACE** Toggle GUI / free camera movement

OpenGL 4.6 required
