#include "voxelizer.h"

#include <math.h>
#include <float.h>
#include <malloc.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif


typedef enum {
	VL_EProjectNone,
	VL_EProjectFront,
	VL_EProjectLeft,
	VL_EProjectTop,
} VL_ProjectDirection;



/*
 * STATIC
 */


/*
 * TODO
 * I'm going to add multi-thread support
 */
static size_t vl_get_cpu_count() {
	long nprocs = -1;
#ifdef _WIN32
	#ifndef _SC_NPROCESSORS_ONLN
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		#define sysconf(a) info.dwNumberOfProcessors
		#define _SC_NPROCESSORS_ONLN
	#endif
#endif

#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	return nprocs;
#else
	return 0;
#endif
}


static bool vl_is_voxel_tri_intersected_proj(
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const t0,
	const VL_Vector3 * const t1,
	const VL_Vector3 * const t2,
	const VL_Vector3 * const vcenter,
	const double vsize
	);


static bool vl_is_lineseg_intersected_proj(
	VL_Vector3 * out,
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const beg1, const VL_Vector3 * end1,
	const VL_Vector3 * const beg2, const VL_Vector3 * end2
	);


static bool vl_is_vert_in_tri_proj(
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const v,
	const VL_Vector3 * const t0,
	const VL_Vector3 * const t1,
	const VL_Vector3 * const t2
	);


static void vl_proj_vert_front(VL_Vector3 * dst, const VL_Vector3 * const src) {
	VL_Vector3 temp = { src->x, src->y, src->z };
	dst->x = temp.x;
	dst->y = temp.z;
	dst->z = 0.0;
}


static void vl_proj_vert_left(VL_Vector3 * dst, const VL_Vector3 * const src) {
	VL_Vector3 temp = { src->x, src->y, src->z };
	dst->x = -temp.y;
	dst->y =  temp.z;
	dst->z =  0.0;
}


static void vl_proj_vert_top(VL_Vector3 * dst, const VL_Vector3 * const src) {
	VL_Vector3 temp = { src->x, src->y, src->z };
	dst->x = temp.x;
	dst->y = temp.y;
	dst->z = 0.0;
}


static bool vl_is_lineseg_intersected_proj(
	VL_Vector3 * out,
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const beg1, const VL_Vector3 * const end1,
	const VL_Vector3 * const beg2, const VL_Vector3 * const end2
	) {
	VL_Vector3 a, b, c, d;
	switch (project_axis) {
		case VL_EProjectNone:
			a.x = beg1->x; a.y = beg1->y; a.z = beg1->z;
			b.x = end1->x; b.y = end1->y; b.z = end1->z;
			c.x = beg2->x; c.y = beg2->y; c.z = beg2->z;
			d.x = end2->x; d.y = end2->y; d.z = end2->z;
			break;
		case VL_EProjectFront:
			vl_proj_vert_front(&a, beg1);
			vl_proj_vert_front(&b, end1);
			vl_proj_vert_front(&c, beg2);
			vl_proj_vert_front(&d, end2);
			break;
		case VL_EProjectLeft:
			vl_proj_vert_left(&a, beg1);
			vl_proj_vert_left(&b, end1);
			vl_proj_vert_left(&c, beg2);
			vl_proj_vert_left(&d, end2);
			break;
		case VL_EProjectTop:
			vl_proj_vert_top(&a, beg1);
			vl_proj_vert_top(&b, end1);
			vl_proj_vert_top(&c, beg2);
			vl_proj_vert_top(&d, end2);
			break;
	}
	double area_abc = (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
	double area_abd = (a.x - d.x) * (b.y - d.y) - (a.y - d.y) * (b.x - d.x);
	if ( area_abc * area_abd >= -FLT_EPSILON ) {
		return false;
	}
	double area_cda = (c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x);
	double area_cdb = area_cda + area_abc - area_abd ;
	if ( area_cda * area_cdb >= -FLT_EPSILON ) {
		return false;
	}
	double t  = area_cda / ( area_abd - area_abc );
	double dx = t * (b.x - a.x);
	double dy = t * (b.y - a.y);
	if (out) {
		out->x = a.x + dx;
		out->y = a.y + dy;
	}

	return true;
}


static bool vl_is_voxel_tri_intersected_proj(
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const t0,
	const VL_Vector3 * const t1,
	const VL_Vector3 * const t2,
	const VL_Vector3 * const vcenter,
	const double vsize
	) {
	double halfsize = vsize / 2.0;
	VL_Vector3 tmin, tmax, vmin, vmax;
	VL_Vector3 pt0, pt1, pt2, pvcenter;
	VL_Vector3 box[4];
	switch (project_axis) {
		case VL_EProjectNone:
			pt0.x = t0->x; pt0.y = t0->y; pt0.z = t0->z;
			pt1.x = t1->x; pt1.y = t1->y; pt1.z = t1->z;
			pt2.x = t2->x; pt2.y = t2->y; pt2.z = t2->z;
			pvcenter.x = vcenter->x; pvcenter.y = vcenter->y; pvcenter.z = vcenter->z;
			break;
		case VL_EProjectFront:
			vl_proj_vert_front(&pt0, t0);
			vl_proj_vert_front(&pt1, t1);
			vl_proj_vert_front(&pt2, t2);
			vl_proj_vert_front(&pvcenter, vcenter);
			break;
		case VL_EProjectLeft:
			vl_proj_vert_left(&pt0, t0);
			vl_proj_vert_left(&pt1, t1);
			vl_proj_vert_left(&pt2, t2);
			vl_proj_vert_left(&pvcenter, vcenter);
			break;
		case VL_EProjectTop:
			vl_proj_vert_top(&pt0, t0);
			vl_proj_vert_top(&pt1, t1);
			vl_proj_vert_top(&pt2, t2);
			vl_proj_vert_top(&pvcenter, vcenter);
			break;
	}
	tmin.x = VL_MIN(VL_MIN(pt0.x, pt1.x), pt2.x);
	tmin.y = VL_MIN(VL_MIN(pt0.y, pt1.y), pt2.y);
	tmax.x = VL_MAX(VL_MAX(pt0.x, pt1.x), pt2.x);
	tmax.y = VL_MAX(VL_MAX(pt0.y, pt1.y), pt2.y);
	vmin.x = pvcenter.x - halfsize;
	vmin.y = pvcenter.y - halfsize;
	vmax.x = pvcenter.x + halfsize;
	vmax.y = pvcenter.y + halfsize;
	box[0].x = pvcenter.x - halfsize; box[0].y = pvcenter.y + halfsize;
	box[1].x = pvcenter.x + halfsize; box[1].y = pvcenter.y + halfsize;
	box[2].x = pvcenter.x - halfsize; box[2].y = pvcenter.y - halfsize;
	box[3].x = pvcenter.x + halfsize; box[3].y = pvcenter.y - halfsize;
	// Seperated
	if ((tmin.x > vmax.x) ||
		(tmin.y > vmax.y) ||
		(tmax.x < vmin.x) ||
		(tmax.y < vmin.y)) {
		return false;
	}
	// Voxel contains triangle
	if ((tmin.x >= vmin.x) && (tmax.x <= vmax.x) &&
		(tmin.y >= vmin.y) && (tmax.y <= vmax.y)) {
		return true;
	}
	// Triangle contains voxel
	if (vl_is_vert_in_tri_proj(VL_EProjectNone, box + 0, &pt0, &pt1, &pt2) ||
		vl_is_vert_in_tri_proj(VL_EProjectNone, box + 1, &pt0, &pt1, &pt2) ||
		vl_is_vert_in_tri_proj(VL_EProjectNone, box + 2, &pt0, &pt1, &pt2) ||
		vl_is_vert_in_tri_proj(VL_EProjectNone, box + 3, &pt0, &pt1, &pt2)) {
		return true;
	}
	// Triangle intersected with voxel but no vertex of voxel is in triangle
	if (vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt1, box + 0, box + 1) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt1, box + 2, box + 3) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt1, box + 0, box + 2) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt1, box + 1, box + 3) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt2, box + 0, box + 1) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt2, box + 2, box + 3) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt2, box + 0, box + 2) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt0, &pt2, box + 1, box + 3) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt1, &pt2, box + 0, box + 1) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt1, &pt2, box + 2, box + 3) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt1, &pt2, box + 0, box + 2) ||
		vl_is_lineseg_intersected_proj(NULL, VL_EProjectNone, &pt1, &pt2, box + 1, box + 3)) {
		    return true;
	}
	return false;
}


static bool vl_is_vert_in_tri_proj(
	VL_ProjectDirection project_axis,
	const VL_Vector3 * const v,
	const VL_Vector3 * const t0,
	const VL_Vector3 * const t1,
	const VL_Vector3 * const t2
	) {
	VL_Vector3 pv, pt0, pt1, pt2;
	VL_Vector3 ab, ac, ap, ba, bc, bp;
	VL_Vector3 r1, r2, r3, r4;
	double dot1, dot2;
	switch (project_axis) {
		case VL_EProjectNone:
			pv.x = v->x; pv.y = v->y; pv.z = v->z;
			pt0.x = t0->x; pt0.y = t0->y; pt0.z = t0->z;
			pt1.x = t1->x; pt1.y = t1->y; pt1.z = t1->z;
			pt2.x = t2->x; pt2.y = t2->y; pt2.z = t2->z;
			break;
		case VL_EProjectFront:
			vl_proj_vert_front(&pv, v);
			vl_proj_vert_front(&pt0, t0);
			vl_proj_vert_front(&pt1, t1);
			vl_proj_vert_front(&pt2, t2);
			break;
		case VL_EProjectLeft:
			vl_proj_vert_left(&pv, v);
			vl_proj_vert_left(&pt0, t0);
			vl_proj_vert_left(&pt1, t1);
			vl_proj_vert_left(&pt2, t2);
			break;
		case VL_EProjectTop:
			vl_proj_vert_top(&pv, v);
			vl_proj_vert_top(&pt0, t0);
			vl_proj_vert_top(&pt1, t1);
			vl_proj_vert_top(&pt2, t2);
			break;
	}
	vl_vec3_sub(&ab, &pt1, &pt0);
	vl_vec3_sub(&ac, &pt2, &pt0);
	vl_vec3_sub(&ap, &pv, &pt0);
	vl_vec3_sub(&ba, &pt0, &pt1);
	vl_vec3_sub(&bc, &pt2, &pt1);
	vl_vec3_sub(&bp, &pv, &pt1);
	vl_vec3_cross(&r1, &ab, &ap);
	vl_vec3_cross(&r2, &ap, &ac);
	vl_vec3_cross(&r3, &bc, &bp);
	vl_vec3_cross(&r4, &bp, &ba);
	vl_vec3_dot(&dot1, &r1, &r2);
	vl_vec3_dot(&dot2, &r3, &r4);
	if ((dot1 < -FLT_EPSILON) || (dot2 < -FLT_EPSILON)) {
		return false;
	}
	return true;
}


/*
 * EXTERN
 */


extern void vl_vec3_add(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	out->x = a->x + b->x;
	out->y = a->y + b->y;
	out->z = a->z + b->z;
}


extern void vl_vec3_sub(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;
}


extern void vl_vec3_mul(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	out->x = a->x * b->x;
	out->y = a->y * b->y;
	out->z = a->z * b->z;
}


extern void vl_vec3_div(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	out->x = a->x / b->x;
	out->y = a->y / b->y;
	out->z = a->z / b->z;
}


extern void vl_vec3_dot(double * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	*out = a->x * b->x + a->y * b->y + a->z * b->z;
}


extern void vl_vec3_cross(VL_Vector3 * out, const VL_Vector3 * const a, const VL_Vector3 * const b) {
	out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->y = a->x * b->y - a->y * b->x;
}


extern double vl_volume_from_mesh(
	const VL_Vector3 * const verts,
	const size_t nverts,
	const size_t * const faces,
	const size_t nfaces,
	const double vsize
	) {
	size_t npoints = 0;
	VL_Vector3 * point_cloud;
	point_cloud = vl_point_cloud_from_mesh(NULL, &npoints, verts, nverts, faces, nfaces, vsize);
	free(point_cloud);
	return vsize * vsize * vsize * npoints;
}


extern void vl_mesh_from_point_cloud(
	VL_Vector3 ** const verts,
	size_t * const nverts,
	size_t ** const faces,
	size_t * const nfaces,
	const VL_Vector3 * const point_cloud,
	size_t npoints,
	const double vsize
	) {
	double       halfsize = vsize / 2.0;
	size_t       local_nverts = npoints * 8;
	size_t       local_nfaces = npoints * 12;
	VL_Vector3 * local_verts  = (VL_Vector3 *)malloc(sizeof(VL_Vector3) * local_nverts);
	size_t *     local_faces  = (size_t *)malloc(sizeof(size_t) * local_nfaces * 3);

	for (size_t i = 0; i < npoints; i++) {
		const VL_Vector3 * const point = point_cloud + i;

		VL_Vector3 * p0 = local_verts + i * 8 + 0;
		VL_Vector3 * p1 = p0 + 1;
		VL_Vector3 * p2 = p1 + 1;
		VL_Vector3 * p3 = p2 + 1;
		VL_Vector3 * p4 = p3 + 1;
		VL_Vector3 * p5 = p4 + 1;
		VL_Vector3 * p6 = p5 + 1;
		VL_Vector3 * p7 = p6 + 1;

		p0->x = point->x + halfsize; p0->y = point->y - halfsize; p0->z = point->z + halfsize;
		p1->x = point->x - halfsize; p1->y = point->y - halfsize; p1->z = point->z + halfsize;
		p2->x = point->x + halfsize; p2->y = point->y + halfsize; p2->z = point->z + halfsize;
		p3->x = point->x - halfsize; p3->y = point->y + halfsize; p3->z = point->z + halfsize;
		p4->x = point->x + halfsize; p4->y = point->y - halfsize; p4->z = point->z - halfsize;
		p5->x = point->x - halfsize; p5->y = point->y - halfsize; p5->z = point->z - halfsize;
		p6->x = point->x + halfsize; p6->y = point->y + halfsize; p6->z = point->z - halfsize;
		p7->x = point->x - halfsize; p7->y = point->y + halfsize; p7->z = point->z - halfsize;

		size_t * f0  = local_faces + i * 12 * 3;
		size_t * f1  = f0  + 3;
		size_t * f2  = f1  + 3;
		size_t * f3  = f2  + 3;
		size_t * f4  = f3  + 3;
		size_t * f5  = f4  + 3;
		size_t * f6  = f5  + 3;
		size_t * f7  = f6  + 3;
		size_t * f8  = f7  + 3;
		size_t * f9  = f8  + 3;
		size_t * f10 = f9  + 3;
		size_t * f11 = f10 + 3;

		*f0  = i * 8 + 0; *(f0  + 1) = i * 8 + 1; *(f0  + 2) = i * 8 + 2; // 0 1 2
		*f1  = i * 8 + 1; *(f1  + 1) = i * 8 + 2; *(f1  + 2) = i * 8 + 3; // 1 2 3
		*f2  = i * 8 + 4; *(f2  + 1) = i * 8 + 5; *(f2  + 2) = i * 8 + 6; // 4 5 6
		*f3  = i * 8 + 5; *(f3  + 1) = i * 8 + 6; *(f3  + 2) = i * 8 + 7; // 5 6 7
		*f4  = i * 8 + 0; *(f4  + 1) = i * 8 + 1; *(f4  + 2) = i * 8 + 4; // 0 1 4
		*f5  = i * 8 + 1; *(f5  + 1) = i * 8 + 4; *(f5  + 2) = i * 8 + 5; // 1 4 5
		*f6  = i * 8 + 2; *(f6  + 1) = i * 8 + 3; *(f6  + 2) = i * 8 + 6; // 2 3 6
		*f7  = i * 8 + 3; *(f7  + 1) = i * 8 + 6; *(f7  + 2) = i * 8 + 7; // 3 6 7
		*f8  = i * 8 + 1; *(f8  + 1) = i * 8 + 3; *(f8  + 2) = i * 8 + 5; // 1 3 5
		*f9  = i * 8 + 3; *(f9  + 1) = i * 8 + 5; *(f9  + 2) = i * 8 + 7; // 3 5 7
		*f10 = i * 8 + 0; *(f10 + 1) = i * 8 + 2; *(f10 + 2) = i * 8 + 4; // 0 2 4
		*f11 = i * 8 + 2; *(f11 + 1) = i * 8 + 4; *(f11 + 2) = i * 8 + 6; // 2 4 6

	}
	*nverts = local_nverts;
	*nfaces = local_nfaces;
	*verts  = local_verts;
	*faces  = local_faces;
}


extern VL_Vector3 * vl_point_cloud_from_mesh(
	VL_Vector3 ** const point_cloud,
	size_t * npoints,
	const VL_Vector3 * const verts,
	const size_t nverts,
	const size_t * const faces,
	const size_t nfaces,
	const double vsize
	) {
	// Voxel's bounding box and center
	VL_Vector3 vmin, vmax, vcenter;
	// Half of voxel's size
	const double halfsize = vsize / 2.0;
	// Count of mesh's division in X, Y, Z dimensions fron voxel
	size_t c_x, c_y, c_z;
	// Project plane buff which is used to store hit flag
	bool * buff_front, * buff_left, * buff_top;
	// Pre Projected verts in X, Y, Z dimension
	VL_Vector3 * verts_front, * verts_left, * verts_top;
	// Integer common counter
	size_t counter = 0;
	// IDX from X, Y, Z project plane
	size_t id_front, id_left, id_top;
	// Output point cloud
	VL_Vector3 * out_point_cloud;

	// Reset npoints to 0 and return NULL if no verts
	*npoints = 0;
	if (point_cloud) { *point_cloud = NULL; }
	if (nverts == 0 || nfaces == 0) {
		return NULL;
	}

	// Calculate voxel's bounding box
	vmin.x = vmax.x = verts[0].x;
	vmin.y = vmax.y = verts[0].y;
	vmin.z = vmax.z = verts[0].z;
	for (size_t i = 0; i < nverts; i++) {
		vmin.x = VL_MIN(vmin.x, verts[i].x);
		vmin.y = VL_MIN(vmin.y, verts[i].y);
		vmin.z = VL_MIN(vmin.z, verts[i].z);
		vmax.x = VL_MAX(vmax.x, verts[i].x);
		vmax.y = VL_MAX(vmax.y, verts[i].y);
		vmax.z = VL_MAX(vmax.z, verts[i].z);
	}

	// Mesh division in X, Y, Z direction
	c_x = (size_t)VL_MAX((ceil((vmax.x - vmin.x) / vsize)), 1);
	c_y = (size_t)VL_MAX((ceil((vmax.y - vmin.y) / vsize)), 1);
	c_z = (size_t)VL_MAX((ceil((vmax.z - vmin.z) / vsize)), 1);

	// Allocate X, Y, Z project plane to store hit flag
	buff_front = (bool *)malloc(sizeof(bool) * c_x * c_z);
	buff_left  = (bool *)malloc(sizeof(bool) * c_y * c_z);
	buff_top   = (bool *)malloc(sizeof(bool) * c_x * c_y);
	if ((NULL == buff_front) || (NULL == buff_left) || (NULL == buff_top)) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		*npoints = 0;
		if (point_cloud) { *point_cloud = NULL; }
		return NULL;
	}
	memset(buff_front, 0, sizeof(bool) * c_x * c_z);
	memset(buff_left,  0, sizeof(bool) * c_y * c_z);
	memset(buff_top,   0, sizeof(bool) * c_x * c_y);

	// Pre project verts into X, Y, Z project plane
	verts_front = (VL_Vector3 *)malloc(sizeof(VL_Vector3) * nverts);
	verts_left  = (VL_Vector3 *)malloc(sizeof(VL_Vector3) * nverts);
	verts_top   = (VL_Vector3 *)malloc(sizeof(VL_Vector3) * nverts);
	if ((NULL == verts_front) || (NULL == verts_left) || (NULL == verts_top)) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		if (NULL != verts_front) free(verts_front);
		if (NULL != verts_left) free(verts_left);
		if (NULL != verts_top) free(verts_top);
		*npoints = 0;
		if (point_cloud) { *point_cloud = NULL; }
		return NULL;
	}
	for (size_t i = 0; i < nverts; i++) {
		vl_proj_vert_front(verts_front + i, verts + i);
		vl_proj_vert_left (verts_left  + i, verts + i);
		vl_proj_vert_top  (verts_top   + i, verts + i);
	}

	size_t cnt_front = 0;
	size_t cnt_left = 0;
	size_t cnt_top = 0;
	// Trace Front
	for (size_t z = 0; z < c_z; z++) {
		for (size_t x = 0; x < c_x; x++) {
			int hit;
			vcenter.x = x * vsize + vmin.x;
			vcenter.y = 0.0;
			vcenter.z = z * vsize + vmin.z;
			vl_proj_vert_front(&vcenter, &vcenter);
			for (size_t f = 0; f < nfaces; f++) {
				size_t idx = faces[f * 3 + 0];
				size_t idy = faces[f * 3 + 1];
				size_t idz = faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_front + idx,
						verts_front + idy,
						verts_front + idz,
						&vcenter,
						vsize
						);
				if (hit) {
					buff_front[z * c_x + x] = hit;
					cnt_front++;
					break;
				}
			}
		}
	}

	// Trace Left
	for (size_t z = 0; z < c_z; z++) {
		for (size_t y = 0; y < c_y; y++) {
			int hit;
			vcenter.x = 0.0;
			vcenter.y = y * vsize + vmin.y;
			vcenter.z = z * vsize + vmin.z;
			vl_proj_vert_left(&vcenter, &vcenter);
			for (size_t f = 0; f < nfaces; f++) {
				size_t idx = faces[f * 3 + 0];
				size_t idy = faces[f * 3 + 1];
				size_t idz = faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_left + idx,
						verts_left + idy,
						verts_left + idz,
						&vcenter,
						vsize
						);
				if (hit) {
					buff_left[z * c_y + y] = hit;
					cnt_left++;
					break;
				}
			}
		}
	}

	// Trace Top
	for (size_t y = 0; y < c_y; y++) {
		for (size_t x = 0; x < c_x; x++) {
			int hit;
			vcenter.x = x * vsize + vmin.x;
			vcenter.y = y * vsize + vmin.y;
			vcenter.z = 0.0;
			vl_proj_vert_top(&vcenter, &vcenter);
			for (size_t f = 0; f < nfaces; f++) {
				size_t idx = faces[f * 3 + 0];
				size_t idy = faces[f * 3 + 1];
				size_t idz = faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_top + idx,
						verts_top + idy,
						verts_top + idz,
						&vcenter,
						vsize
						);
				if (hit) {
					buff_top[y * c_x + x] = hit;
					cnt_top++;
					break;
				}
			}
		}
	}

	// Accumulate hit voxel count for point cloud memmory allocation
	for (size_t x = 0; x < c_x; x++) {
		for (size_t y = 0; y < c_y; y++) {
			for (size_t z = 0; z < c_z; z++) {
				id_front = z * c_x + x;
				id_left  = z * c_y + y;
				id_top   = y * c_x + x;
				if (buff_front[id_front] && buff_left[id_left] && buff_top[id_top]) {
					*npoints += 1;
				}
			}
		}
	}
	// Allocate memmory for point cloud
	out_point_cloud = (VL_Vector3 *)malloc(sizeof(VL_Vector3) * (*npoints));
	if (NULL == out_point_cloud) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		if (NULL != verts_front) free(verts_front);
		if (NULL != verts_left) free(verts_left);
		if (NULL != verts_top) free(verts_top);
		if (NULL != *point_cloud) free(*point_cloud);
		*npoints = 0;
		if (point_cloud) { *point_cloud = NULL; }
		return NULL;
	}
	for (size_t x = 0; x < c_x; x++) {
		for (size_t y = 0; y < c_y; y++) {
			for (size_t z = 0; z < c_z; z++) {
				id_front = z * c_x + x;
				id_left  = z * c_y + y;
				id_top   = y * c_x + x;
				if (buff_front[id_front] && buff_left[id_left] && buff_top[id_top]) {
					(out_point_cloud + counter)->x = x * vsize + halfsize + vmin.x;
					(out_point_cloud + counter)->y = y * vsize + halfsize + vmin.y;
					(out_point_cloud + counter)->z = z * vsize + halfsize + vmin.z;
					counter++;
				}
			}
		}
	}

	free(buff_front);
	free(buff_left);
	free(buff_top);
	free(verts_front);
	free(verts_left);
	free(verts_top);

	if (point_cloud) { *point_cloud = out_point_cloud; }
	return out_point_cloud;
}


