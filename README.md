# Overview
This is a tool that computes and renders the collision hulls of GoldSrc maps (BSP version 30), such as Half-Life 1 and Opposing Force maps.

The ```hlbsp.out``` utility inputs a **bsp** file and outputs a **col** file. This custom file format is specified in ```hlbsp.h```, and it is structured similarly to the **bsp** format.

The ```view.out``` utility is a OpenGL+Glut utility that renders a **col** file. A camera can be moved through the space with **WASD** (no-clip). The hull (projectile, standing and crouched) can be switched with **123**. The full option decides whether only worldspawn is visible or all the models are. It can be switched with **X**. The camera speed can be doubled and halved, with **+** and **-** respectively. The mouse grab can be turned on/off with **\***.

For example, below is the results for **c1a0e.bsp** (Test chamber): ![preview](/preview.png)

# Issues
Maps **c2a2a** and **c3a2c** crash the algorithm: ***Apparently both contain some out-of-bounds indices***! If these are panic-ignored, the first map looks ok.
