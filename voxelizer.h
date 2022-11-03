#ifndef _VOXELIZER_H_
#define _VOXELIZER_H_


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define _VL_IN_
#define _VL_OUT_
#define _VL_OPT_IN_
#define _VL_OPT_OUT_


#ifdef VL_TEST
#define _VL_STATIC_ extern
#else
#define _VL_STATIC_ static
#endif
#define _VL_EXTERN_ extern


#define VL_MIN(a, b) ((a) > (b) ? (b) : (a))
#define VL_MAX(a, b) ((a) < (b) ? (b) : (a))


#ifdef VL_HIGHP
typedef double VL_Float;
typedef size_t VL_Size;
#else
typedef float VL_Float;
typedef unsigned int VL_Size;
#endif



typedef struct { VL_Float x, y, z; } VL_Vector3F;


/*
 * Necessary vector3 mathematics
 * Be easy when using add, sub, mul and div, I've added temp variable to avoid cyclic operation
 * eg: add(&v1, &v1, &v2);
 */
_VL_STATIC_ void vl_vec3_add(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_sub(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_mul(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_div(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_dist(VL_Float * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_dot(VL_Float * out, const VL_Vector3F * const a, const VL_Vector3F * const b);
_VL_STATIC_ void vl_vec3_cross(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b);


/*
 * Get point cloud difinition from mesh in pre calculation
 *
 * @outcx        Output definition in x axis
 * @outcy        Output definition in y axis
 * @outcz        Output definition in z axis
 * @outvmin:     Output bbox min
 * @outvmax:     Output bbox max
 * @inverts:     Input verts
 * @innverts:    Input vert count
 * @invsize:     Input voxel size
 */
_VL_EXTERN_ void vl_point_cloud_res_from_mesh(
	_VL_OPT_OUT_ VL_Size *                 out_cx,
	_VL_OPT_OUT_ VL_Size *                 out_cy,
	_VL_OPT_OUT_ VL_Size *                 out_cz,
	_VL_OPT_OUT_ VL_Vector3F * const       out_vmin,
	_VL_OPT_OUT_ VL_Vector3F * const       out_vmax,
	_VL_IN_      const VL_Vector3F * const in_verts,
	_VL_IN_      const VL_Size             in_nverts,
	_VL_IN_      const VL_Float            in_vsize
	);


/*
 * Get point cloud difinition from bbox in pre calculation
 *
 * @outcx        Output definition in x axis
 * @outcy        Output definition in y axis
 * @outcz        Output definition in z axis
 * @invmin:      Input bbox min
 * @invmax:      Input bbox max
 * @invsize:     Input voxel size
 */
_VL_EXTERN_ void vl_point_cloud_res_from_bbox(
	_VL_OUT_ VL_Size *                 out_cx,
	_VL_OUT_ VL_Size *                 out_cy,
	_VL_OUT_ VL_Size *                 out_cz,
	_VL_IN_  const VL_Vector3F * const in_vmin,
	_VL_IN_  const VL_Vector3F * const in_vmax,
	_VL_IN_  const VL_Float invsize
	);


/*
 * Generate point cloud fron mesh, result point cloud should be freed mannually
 *
 * Return:       Output point cloud pointer
 * @point_cloud: Output point cloud pointer, result will be returned although NULL is passed
 * @npoints:     Ouput point cloud count
 * @verts:       Input vertices
 * @nverts:      Input vertex count
 * @faces:       Input faces
 * @nfaces:      Input face count
 * @vsize:       Input voxel size
 */
_VL_EXTERN_ VL_Vector3F * vl_point_cloud_from_mesh(
	_VL_OPT_OUT_ VL_Vector3F ** const      out_point_cloud,
	_VL_OUT_     VL_Size * const           out_npoints,
	_VL_IN_      const VL_Vector3F * const in_verts,
	_VL_IN_      const VL_Size             in_nverts,
	_VL_IN_      const VL_Size * const     in_faces,
	_VL_IN_      const VL_Size             in_nfaces,
	_VL_IN_      const VL_Float            in_vsize
	);


/*
 * Generate mesh fron point cloud, verts and faces pointer should be freed manually after use
 *
 * @verts:       Ouput vertices pointer
 * @nverts:      Ouput vertex count pointer
 * @faces:       Ouput faces pointer
 * @nfaces:      Ouput face count pointer
 * @point_cloud: Input point cloud pointer
 * @npoints:     Input point cloud
 * @vsize:       Input voxel size
 */
_VL_EXTERN_ void vl_mesh_from_point_cloud(
	_VL_OUT_ VL_Vector3F ** const      out_verts,
	_VL_OUT_ VL_Size * const           out_nverts,
	_VL_OUT_ VL_Size ** const          out_faces,
	_VL_OUT_ VL_Size * const           out_nfaces,
	_VL_IN_  const VL_Vector3F * const in_point_cloud,
	_VL_IN_  VL_Size                   in_npoints,
	_VL_IN_  const VL_Float            in_vsize
	);


/*
 * Get mesh volume
 * This implementation uses vl_point_cloud_from_mesh to generate point cloud and cound point for volume
 *
 * Return:       Mesh volume
 * @verts:       Input vertices
 * @nverts:      Input vertex count
 * @faces:       Input faces
 * @nfaces:      Input face count
 * @vsize:       Input voxel size
 */
_VL_EXTERN_ VL_Float vl_volume_from_mesh(
	_VL_IN_ const VL_Vector3F * in_verts,
	_VL_IN_ const VL_Size       in_nverts,
	_VL_IN_ const VL_Size *     in_faces,
	_VL_IN_ const VL_Size       in_nfaces,
	_VL_IN_ const VL_Float      in_vsize
	);


#endif
