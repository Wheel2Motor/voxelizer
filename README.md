Voxelizer
================================================================================

标准C语言编写的Voxelizer，没有任何依赖项。

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

只有voxelizer.c和voxelizer.h这两个文件，加入你自己的工程编译即可。

# 参考
[STL模型体素化](https://zhuanlan.zhihu.com/p/410306876)

# Bug
在M1芯片的Mac上计算结果稳定复现错误；但是x86上稳定复现不出来。原因未知。
