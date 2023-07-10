Voxelizer
================================================================================

用C语言编写的Voxelizer，没有任何依赖项。

提供了快速的网格体积计算和点云生成以及体素化。

<div align="left">
<img src="img/img1.png" width="512px"></img>
<img src="img/img2.png" width="512px"></img>
<img src="img/img3.png" width="512px"></img>
<img src="img/img4.png" width="512px"></img>
</div>

# 注意
该模块只支持三角面，不支持多边形。

# 编译

## MinGW及类Unix环境

Mingw环境下，直接使用make命令即可生成一个dll和一个用于测试的可执行程序。

dll可以在python中进行使用，使用步骤见example.py。

你可能需要给 __Makefile__ 中的 __a.out__ 换成 __a.exe__ 。

## Visual Studio
如果使用Visual Studio，直接新建一个普通的C++工程，将voxelizer.c和voxelizer.h加入工程编译即可。

# 参考
[STL模型体素化](https://zhuanlan.zhihu.com/p/410306876)

# Bug
在M1芯片的Mac上计算错误，原因未知。
