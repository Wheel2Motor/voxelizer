[Simplified Chinese](README_CHS.md)

# Voxelizer
Voxelizer writen in C without any dependencies.
Fast mesh volume accumulation, point cloud generation and voxelization functionalities are provided. 

# Why I wrote this module
I hope this functionality to be a pure C implementation and simple enough with no dependency.

# Attention
This module only support triangle faces, polygons are not supported.

# Compile

## MinGW
In Mingw environments, use make command to generate a dll and an executable used for test. 
You may need to replace __a.out__ in the __Makefile__ with __a.exe__.
Dll can be used in python, follow example.py. 

## Visual Studio
If you are using Visual Studio, just create an empty C++ project, then drop voxelizer.c and voxelizer.h into the project, then compile it.

## Unix-Like
Just use __make__ command to generate a **so** file and an executable used for test. so can be used in python, follow example.py.

# Reference
[STL Mesh Voxelize](https://zhuanlan.zhihu.com/p/410306876)

# Bug
Calculation error on Mac with M1 soc

---

![img1](img/img1.png)
![img2](img/img2.png)
![img3](img/img3.png)
![img4](img/img4.png)
