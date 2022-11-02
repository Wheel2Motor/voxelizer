import os
import sys
import ctypes


dll = ctypes.cdll.LoadLibrary(os.path.abspath(os.path.join(os.path.dirname(__file__), "voxelizer.dll")))


class VL_Vector3F(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_double),
        ("y", ctypes.c_double),
        ("z", ctypes.c_double),
        ]

    def __repr__(self):
        return "xyz2vec3(%f, %f, %f) " % (self.x, self.y, self.z)


def xyz2vec3(x, y, z):
    ret = VL_Vector3F()
    ret.x = x
    ret.y = y
    ret.z = z
    return ret


_vl_volume_from_mesh = dll.vl_volume_from_mesh
_vl_volume_from_mesh.argtypes = (
    ctypes.POINTER(VL_Vector3F),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.c_size_t,
    ctypes.c_double
    )
_vl_volume_from_mesh.restype = ctypes.c_double


def vl_volume_from_mesh(verts, faces, vsize):
    nverts = len(verts)
    nfaces = int(len(faces) / 3)
    c_verts = (VL_Vector3F * nverts)(*verts)
    c_nverts = ctypes.c_size_t(nverts)
    c_faces = (ctypes.c_size_t * int(nfaces * 3))(*faces)
    c_nfaces = ctypes.c_size_t(nfaces)
    c_vsize = ctypes.c_double(vsize)
    c_volume = _vl_volume_from_mesh(c_verts, c_nverts, c_faces, c_nfaces, c_vsize)
    return c_volume


_vl_point_cloud_res_from_mesh = dll.vl_point_cloud_res_from_mesh
_vl_point_cloud_res_from_mesh.argtypes = (
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(VL_Vector3F),
    ctypes.POINTER(VL_Vector3F),
    ctypes.POINTER(VL_Vector3F),
    ctypes.c_size_t,
    ctypes.c_double
    )
_vl_point_cloud_res_from_mesh.restype = None


def point_cloud_res_from_mesh(verts, vsize):
    cx = ctypes.c_size_t()
    cy = ctypes.c_size_t()
    cz = ctypes.c_size_t()
    vmin = VL_Vector3F()
    vmax = VL_Vector3F()
    nverts = len(verts)
    c_verts = (VL_Vector3F * nverts)(*verts)
    c_nverts = ctypes.c_size_t(nverts)
    c_vsize = ctypes.c_double(vsize)
    _vl_point_cloud_res_from_mesh(
        ctypes.byref(cx),
        ctypes.byref(cy),
        ctypes.byref(cz),
        ctypes.byref(vmin),
        ctypes.byref(vmax),
        c_verts,
        c_nverts,
        c_vsize
        )
    return cx.value, cy.value, cz.value


if __name__ == "__main__":
    verts = [
        xyz2vec3( 1.0,  1.0, -1.0),
        xyz2vec3(-1.0, -1.0, -1.0),
        xyz2vec3( 0.0,  0.0,  1.0),
        ]
    faces = [0, 1, 2]
    cx, cy, cz = point_cloud_res_from_mesh(verts, 0.1)
    volume = vl_volume_from_mesh(verts, faces, 0.1)
    print(cx, cy, cz)
    print(volume)
