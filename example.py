import os
import sys
import ctypes


dll = ctypes.cdll.LoadLibrary(os.path.abspath(os.path.join(os.path.dirname(__file__), "voxelizer.dll")))


class VL_Vector3(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_double),
        ("y", ctypes.c_double),
        ("z", ctypes.c_double),
        ]


def xyz2vec3(x, y, z):
    ret = VL_Vector3()
    ret.x = x
    ret.y = y
    ret.z = z
    return ret


_vl_volume_from_mesh = dll.vl_volume_from_mesh
_vl_volume_from_mesh.restype = ctypes.c_double
_vl_volume_from_mesh.argtypes = (
    ctypes.POINTER(VL_Vector3),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.c_size_t,
    ctypes.c_double
    )
_vl_volume_from_mesh.restype = ctypes.c_double


def vl_volume_from_mesh(verts, faces, vsize):
    nverts = len(verts)
    nfaces = len(faces)
    c_verts = (VL_Vector3 * nverts)(*verts)
    c_nverts = ctypes.c_size_t(nverts)
    c_faces = (ctypes.c_size_t * nfaces)(*faces)
    c_nfaces = ctypes.c_size_t(nfaces)
    c_vsize = ctypes.c_double(vsize)
    c_volume = _vl_volume_from_mesh(c_verts, c_nverts, c_faces, c_nfaces, c_vsize)
    return c_volume


if __name__ == "__main__":
    verts = [
            xyz2vec3( 1.0,  1.0, -1.0),
            xyz2vec3(-1.0, -1.0, -1.0),
            xyz2vec3( 0.0,  0.0,  1.0),
            ]
    faces = [0, 1, 2]
    volume = vl_volume_from_mesh(verts, faces, 0.5)
    print(volume)
