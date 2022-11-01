#ifndef _VOXELIZER_H_
#define _VOXELIZER_H_


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define VL_MIN(a, b) ((a) > (b) ? (b) : (a))
#define VL_MAX(a, b) ((a) < (b) ? (b) : (a))


typedef struct { double x, y, z; } VL_Vector3;


/*
 * Necessary vector3 mathematics
 * Be easy when using add, sub, mul and div, I've added temp variable to avoid cyclic operation
 * eg: add(&v1, &v1, &v2);
 */
extern void vl_vec3_add(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b);
extern void vl_vec3_sub(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b);
extern void vl_vec3_mul(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b);
extern void vl_vec3_div(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b);
extern void vl_vec3_dot(double * out, const VL_Vector3 * const a, const VL_Vector3 * const b);
extern void vl_vec3_cross(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b);


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
extern VL_Vector3 * vl_point_cloud_from_mesh(
	VL_Vector3 ** const point_cloud,
	size_t * npoints,
	const VL_Vector3 * const verts,
	const size_t nverts,
	const size_t * const faces,
	const size_t nfaces,
	const double vsize
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
extern void vl_mesh_from_point_cloud(
	VL_Vector3 ** const verts,
	size_t * const nverts,
	size_t ** const faces,
	size_t * const nfaces,
	const VL_Vector3 * const point_cloud,
	size_t npoints,
	const double vsize
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
extern double vl_volume_from_mesh(
	const VL_Vector3 * verts,
	const size_t nverts,
	const size_t * faces,
	const size_t nfaces,
	const double vsize
	);


#endif

