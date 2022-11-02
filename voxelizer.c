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
static VL_Size vl_get_cpu_count() {
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
	_VL_IN_ VL_ProjectDirection project_axis,
	_VL_IN_ const VL_Vector3F * const t0,
	_VL_IN_ const VL_Vector3F * const t1,
	_VL_IN_ const VL_Vector3F * const t2,
	_VL_IN_ const VL_Vector3F * const vcenter,
	_VL_IN_ const VL_Float vsize
	);


static bool vl_is_lineseg_intersected_proj(
	_VL_OUT_ VL_Vector3F * out,
	_VL_IN_  VL_ProjectDirection project_axis,
	_VL_IN_  const VL_Vector3F * const beg1, const VL_Vector3F * end1,
	_VL_IN_  const VL_Vector3F * const beg2, const VL_Vector3F * end2
	);


static bool vl_is_vert_in_tri_proj(
	_VL_IN_ VL_ProjectDirection project_axis,
	_VL_IN_ const VL_Vector3F * const v,
	_VL_IN_ const VL_Vector3F * const t0,
	_VL_IN_ const VL_Vector3F * const t1,
	_VL_IN_ const VL_Vector3F * const t2
	);


static void vl_proj_vert_front(VL_Vector3F * dst, const VL_Vector3F * const src) {
	VL_Vector3F temp = { src->x, src->y, src->z };
	dst->x = temp.x;
	dst->y = temp.z;
	dst->z = 0.0;
}


static void vl_proj_vert_left(VL_Vector3F * dst, const VL_Vector3F * const src) {
	VL_Vector3F temp = { src->x, src->y, src->z };
	dst->x = -temp.y;
	dst->y =  temp.z;
	dst->z =  0.0;
}


static void vl_proj_vert_top(VL_Vector3F * dst, const VL_Vector3F * const src) {
	VL_Vector3F temp = { src->x, src->y, src->z };
	dst->x = temp.x;
	dst->y = temp.y;
	dst->z = 0.0;
}


static bool vl_is_lineseg_intersected_proj(
	_VL_OUT_ VL_Vector3F * out,
	_VL_IN_  VL_ProjectDirection project_axis,
	_VL_IN_  const VL_Vector3F * const beg1, const VL_Vector3F * const end1,
	_VL_IN_  const VL_Vector3F * const beg2, const VL_Vector3F * const end2
	) {
	VL_Vector3F a, b, c, d;
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
	VL_Float area_abc = (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
	VL_Float area_abd = (a.x - d.x) * (b.y - d.y) - (a.y - d.y) * (b.x - d.x);
	if ( area_abc * area_abd >= -FLT_EPSILON ) {
		return false;
	}
	VL_Float area_cda = (c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x);
	VL_Float area_cdb = area_cda + area_abc - area_abd ;
	if ( area_cda * area_cdb >= -FLT_EPSILON ) {
		return false;
	}
	VL_Float t  = area_cda / ( area_abd - area_abc );
	VL_Float dx = t * (b.x - a.x);
	VL_Float dy = t * (b.y - a.y);
	if (out) {
		out->x = a.x + dx;
		out->y = a.y + dy;
	}

	return true;
}


static bool vl_is_voxel_tri_intersected_proj(
	_VL_IN_ VL_ProjectDirection project_axis,
	_VL_IN_ const VL_Vector3F * const t0,
	_VL_IN_ const VL_Vector3F * const t1,
	_VL_IN_ const VL_Vector3F * const t2,
	_VL_IN_ const VL_Vector3F * const vcenter,
	_VL_IN_ const VL_Float vsize
	) {
	VL_Float halfsize = vsize / 2.0;
	VL_Vector3F tmin, tmax, vmin, vmax;
	VL_Vector3F pt0, pt1, pt2, pvcenter;
	VL_Vector3F box[4];
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
	_VL_IN_ VL_ProjectDirection project_axis,
	_VL_IN_ const VL_Vector3F * const v,
	_VL_IN_ const VL_Vector3F * const t0,
	_VL_IN_ const VL_Vector3F * const t1,
	_VL_IN_ const VL_Vector3F * const t2
	) {
	VL_Vector3F pv, pt0, pt1, pt2;
	VL_Vector3F ab, ac, ap, ba, bc, bp;
	VL_Vector3F r1, r2, r3, r4;
	VL_Float dot1, dot2;
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


extern void vl_vec3_add(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	out->x = a->x + b->x;
	out->y = a->y + b->y;
	out->z = a->z + b->z;
}


extern void vl_vec3_sub(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;
}


extern void vl_vec3_mul(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	out->x = a->x * b->x;
	out->y = a->y * b->y;
	out->z = a->z * b->z;
}


extern void vl_vec3_div(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	out->x = a->x / b->x;
	out->y = a->y / b->y;
	out->z = a->z / b->z;
}


extern void vl_vec3_dot(VL_Float * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	*out = a->x * b->x + a->y * b->y + a->z * b->z;
}


extern void vl_vec3_dist(VL_Float * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	VL_Float px = a->x * a->x;
	VL_Float py = a->y * a->y;
	VL_Float pz = a->z * a->z;
	*out = sqrt(px + py + pz);
}


extern void vl_vec3_cross(VL_Vector3F * out, const VL_Vector3F * const a, const VL_Vector3F * const b) {
	out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->y = a->x * b->y - a->y * b->x;
}


extern VL_Float vl_volume_from_mesh(
	_VL_IN_ const VL_Vector3F * const in_verts,
	_VL_IN_ const VL_Size             in_nverts,
	_VL_IN_ const VL_Size * const     in_faces,
	_VL_IN_ const VL_Size             in_nfaces,
	_VL_IN_ const VL_Float            in_vsize
	) {
	VL_Size npoints = 0;
	VL_Vector3F * point_cloud;
	point_cloud = vl_point_cloud_from_mesh(NULL, &npoints, in_verts, in_nverts, in_faces, in_nfaces, in_vsize);
	free(point_cloud);
	return in_vsize * in_vsize * in_vsize * npoints;
}


extern void vl_mesh_from_in_point_cloud(
	_VL_OUT_ VL_Vector3F ** const      out_verts,
	_VL_OUT_ VL_Size * const           out_nverts,
	_VL_OUT_ VL_Size ** const          out_faces,
	_VL_OUT_ VL_Size * const           out_nfaces,
	_VL_IN_  const VL_Vector3F * const in_point_cloud,
	_VL_IN_  VL_Size                   in_npoints,
	_VL_IN_  const VL_Float             in_vsize
	) {
	VL_Float       halfsize = in_vsize / 2.0;
	VL_Size       local_nverts = in_npoints * 8;
	VL_Size       local_nfaces = in_npoints * 12;
	VL_Vector3F * local_verts  = (VL_Vector3F *)malloc(sizeof(VL_Vector3F) * local_nverts);
	VL_Size *     local_faces  = (VL_Size *)malloc(sizeof(VL_Size) * local_nfaces * 3);

	*out_verts = NULL;
	*out_nverts = 0;
	*out_faces = NULL;
	*out_nfaces = 0;
	if ((NULL == local_verts) || (NULL == local_faces)) {
		if (NULL != local_verts) free(local_verts);
		if (NULL != local_faces) free(local_faces);
		return;
	}

	for (VL_Size i = 0; i < in_npoints; i++) {
		const VL_Vector3F * const point = in_point_cloud + i;

		VL_Vector3F * p0 = local_verts + i * 8 + 0;
		VL_Vector3F * p1 = p0 + 1;
		VL_Vector3F * p2 = p1 + 1;
		VL_Vector3F * p3 = p2 + 1;
		VL_Vector3F * p4 = p3 + 1;
		VL_Vector3F * p5 = p4 + 1;
		VL_Vector3F * p6 = p5 + 1;
		VL_Vector3F * p7 = p6 + 1;

		p0->x = point->x + halfsize; p0->y = point->y - halfsize; p0->z = point->z + halfsize;
		p1->x = point->x - halfsize; p1->y = point->y - halfsize; p1->z = point->z + halfsize;
		p2->x = point->x + halfsize; p2->y = point->y + halfsize; p2->z = point->z + halfsize;
		p3->x = point->x - halfsize; p3->y = point->y + halfsize; p3->z = point->z + halfsize;
		p4->x = point->x + halfsize; p4->y = point->y - halfsize; p4->z = point->z - halfsize;
		p5->x = point->x - halfsize; p5->y = point->y - halfsize; p5->z = point->z - halfsize;
		p6->x = point->x + halfsize; p6->y = point->y + halfsize; p6->z = point->z - halfsize;
		p7->x = point->x - halfsize; p7->y = point->y + halfsize; p7->z = point->z - halfsize;

		VL_Size * f0  = local_faces + i * 12 * 3;
		VL_Size * f1  = f0  + 3;
		VL_Size * f2  = f1  + 3;
		VL_Size * f3  = f2  + 3;
		VL_Size * f4  = f3  + 3;
		VL_Size * f5  = f4  + 3;
		VL_Size * f6  = f5  + 3;
		VL_Size * f7  = f6  + 3;
		VL_Size * f8  = f7  + 3;
		VL_Size * f9  = f8  + 3;
		VL_Size * f10 = f9  + 3;
		VL_Size * f11 = f10 + 3;

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
	*out_nverts = local_nverts;
	*out_nfaces = local_nfaces;
	*out_verts  = local_verts;
	*out_faces  = local_faces;
}


extern void vl_point_cloud_res_from_mesh(
	_VL_OPT_OUT_ VL_Size *                 out_cx,
	_VL_OPT_OUT_ VL_Size *                 out_cy,
	_VL_OPT_OUT_ VL_Size *                 out_cz,
	_VL_OPT_OUT_ VL_Vector3F * const       out_vmin,
	_VL_OPT_OUT_ VL_Vector3F * const       out_vmax,
	_VL_IN_      const VL_Vector3F * const in_verts,
	_VL_IN_      const VL_Size             in_nverts,
	_VL_IN_      const VL_Float            in_vsize
	) {
	VL_Vector3F temp_vmin, temp_vmax;
	VL_Size temp_cx, temp_cy, temp_cz;

	if (!(in_nverts > 0)) {
		return;
	}
	temp_vmin.x = temp_vmax.x = in_verts[0].x;
	temp_vmin.y = temp_vmax.y = in_verts[0].y;
	temp_vmin.z = temp_vmax.z = in_verts[0].z;
	for (VL_Size i = 0; i < in_nverts; i++) {
		temp_vmin.x = VL_MIN(temp_vmin.x, in_verts[i].x);
		temp_vmin.y = VL_MIN(temp_vmin.y, in_verts[i].y);
		temp_vmin.z = VL_MIN(temp_vmin.z, in_verts[i].z);
		temp_vmax.x = VL_MAX(temp_vmax.x, in_verts[i].x);
		temp_vmax.y = VL_MAX(temp_vmax.y, in_verts[i].y);
		temp_vmax.z = VL_MAX(temp_vmax.z, in_verts[i].z);
	}
	if (NULL != out_vmin) {
		out_vmin->x = temp_vmin.x;
		out_vmin->y = temp_vmin.y;
		out_vmin->z = temp_vmin.z;
	}
	if (NULL != out_vmax) {
		out_vmax->x = temp_vmax.x;
		out_vmax->y = temp_vmax.y;
		out_vmax->z = temp_vmax.z;
	}
	vl_point_cloud_res_from_bbox(&temp_cx, &temp_cy, &temp_cz, &temp_vmin, &temp_vmax, in_vsize);
	if (NULL != out_cx) { *out_cx = temp_cx; }
	if (NULL != out_cy) { *out_cy = temp_cy; }
	if (NULL != out_cz) { *out_cz = temp_cz; }
}


extern void vl_point_cloud_res_from_bbox(
	_VL_OUT_ VL_Size *                out_cx,
	_VL_OUT_ VL_Size *                out_cy,
	_VL_OUT_ VL_Size *                out_cz,
	_VL_IN_ const VL_Vector3F * const in_vmin,
	_VL_IN_ const VL_Vector3F * const in_vmax,
	_VL_IN_ const VL_Float            in_vsize
	) {
	*out_cx = (VL_Size)VL_MAX((ceil((in_vmax->x - in_vmin->x) / in_vsize)), 1);
	*out_cy = (VL_Size)VL_MAX((ceil((in_vmax->y - in_vmin->y) / in_vsize)), 1);
	*out_cz = (VL_Size)VL_MAX((ceil((in_vmax->z - in_vmin->z) / in_vsize)), 1);
}


extern VL_Vector3F * vl_point_cloud_from_mesh(
	_VL_OUT_ VL_Vector3F ** const      out_point_cloud,
	_VL_OUT_ VL_Size * const           out_npoints,
	_VL_IN_  const VL_Vector3F * const in_verts,
	_VL_IN_  const VL_Size             in_nverts,
	_VL_IN_  const VL_Size * const     in_faces,
	_VL_IN_  const VL_Size             in_nfaces,
	_VL_IN_  const VL_Float            in_vsize
	) {
	// Voxel's bounding box and center
	VL_Vector3F vmin, vmax, vcenter;
	// Half of voxel's size
	const VL_Float halfsize = in_vsize / 2.0;
	// Count of mesh's division in X, Y, Z dimensions fron voxel
	VL_Size cx, cy, cz;
	// Project plane buff which is used to store hit flag
	bool * buff_front, * buff_left, * buff_top;
	// Pre Projected in_verts in X, Y, Z dimension
	VL_Vector3F * verts_front, * verts_left, * verts_top;
	// Integer common counter
	VL_Size counter = 0;
	// IDX from X, Y, Z project plane
	VL_Size id_front, id_left, id_top;
	// Output point cloud
	VL_Vector3F * temp_point_cloud;

	// Reset out_npoints to 0 and out_point_cloud to NULL
	*out_npoints = 0;
	if (out_point_cloud) { *out_point_cloud = NULL; }

	// Return NULL if no in_verts
	if (in_nverts == 0 || in_nfaces == 0) {
		return NULL;
	}

	// Calculate voxel's bounding box and mash division in X, Y, Z direction
	vl_point_cloud_res_from_mesh(&cx, &cy, &cz, &vmin, &vmax, in_verts, in_nverts, in_vsize);

	// Allocate X, Y, Z project plane to store hit flag
	buff_front = (bool *)malloc(sizeof(bool) * cx * cz);
	buff_left  = (bool *)malloc(sizeof(bool) * cy * cz);
	buff_top   = (bool *)malloc(sizeof(bool) * cx * cy);
	if ((NULL == buff_front) || (NULL == buff_left) || (NULL == buff_top)) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		*out_npoints = 0;
		if (out_point_cloud) { *out_point_cloud = NULL; }
		return NULL;
	}
	memset(buff_front, 0, sizeof(bool) * cx * cz);
	memset(buff_left,  0, sizeof(bool) * cy * cz);
	memset(buff_top,   0, sizeof(bool) * cx * cy);

	// Pre project in_verts into X, Y, Z project plane
	verts_front = (VL_Vector3F *)malloc(sizeof(VL_Vector3F) * in_nverts);
	verts_left  = (VL_Vector3F *)malloc(sizeof(VL_Vector3F) * in_nverts);
	verts_top   = (VL_Vector3F *)malloc(sizeof(VL_Vector3F) * in_nverts);
	if ((NULL == verts_front) || (NULL == verts_left) || (NULL == verts_top)) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		if (NULL != verts_front) free(verts_front);
		if (NULL != verts_left) free(verts_left);
		if (NULL != verts_top) free(verts_top);
		*out_npoints = 0;
		if (out_point_cloud) { *out_point_cloud = NULL; }
		return NULL;
	}
	for (VL_Size i = 0; i < in_nverts; i++) {
		vl_proj_vert_front(verts_front + i, in_verts + i);
		vl_proj_vert_left (verts_left  + i, in_verts + i);
		vl_proj_vert_top  (verts_top   + i, in_verts + i);
	}

	// Trace Front
	for (VL_Size z = 0; z < cz; z++) {
		for (VL_Size x = 0; x < cx; x++) {
			int hit;
			vcenter.x = x * in_vsize + vmin.x;
			vcenter.y = 0.0;
			vcenter.z = z * in_vsize + vmin.z;
			vl_proj_vert_front(&vcenter, &vcenter);
			for (VL_Size f = 0; f < in_nfaces; f++) {
				VL_Size idx = in_faces[f * 3 + 0];
				VL_Size idy = in_faces[f * 3 + 1];
				VL_Size idz = in_faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_front + idx,
						verts_front + idy,
						verts_front + idz,
						&vcenter,
						in_vsize
						);
				if (hit) {
					buff_front[z * cx + x] = hit;
					break;
				}
			}
		}
	}

	// Trace Left
	for (VL_Size z = 0; z < cz; z++) {
		for (VL_Size y = 0; y < cy; y++) {
			int hit;
			vcenter.x = 0.0;
			vcenter.y = y * in_vsize + vmin.y;
			vcenter.z = z * in_vsize + vmin.z;
			vl_proj_vert_left(&vcenter, &vcenter);
			for (VL_Size f = 0; f < in_nfaces; f++) {
				VL_Size idx = in_faces[f * 3 + 0];
				VL_Size idy = in_faces[f * 3 + 1];
				VL_Size idz = in_faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_left + idx,
						verts_left + idy,
						verts_left + idz,
						&vcenter,
						in_vsize
						);
				if (hit) {
					buff_left[z * cy + y] = hit;
					break;
				}
			}
		}
	}

	// Trace Top
	for (VL_Size y = 0; y < cy; y++) {
		for (VL_Size x = 0; x < cx; x++) {
			int hit;
			vcenter.x = x * in_vsize + vmin.x;
			vcenter.y = y * in_vsize + vmin.y;
			vcenter.z = 0.0;
			vl_proj_vert_top(&vcenter, &vcenter);
			for (VL_Size f = 0; f < in_nfaces; f++) {
				VL_Size idx = in_faces[f * 3 + 0];
				VL_Size idy = in_faces[f * 3 + 1];
				VL_Size idz = in_faces[f * 3 + 2];
				hit = vl_is_voxel_tri_intersected_proj(
						VL_EProjectNone,
						verts_top + idx,
						verts_top + idy,
						verts_top + idz,
						&vcenter,
						in_vsize
						);
				if (hit) {
					buff_top[y * cx + x] = hit;
					break;
				}
			}
		}
	}

	// Accumulate hit voxel count for point cloud memmory allocation
	for (VL_Size x = 0; x < cx; x++) {
		for (VL_Size y = 0; y < cy; y++) {
			for (VL_Size z = 0; z < cz; z++) {
				id_front = z * cx + x;
				id_left  = z * cy + y;
				id_top   = y * cx + x;
				if (buff_front[id_front] && buff_left[id_left] && buff_top[id_top]) {
					*out_npoints += 1;
				}
			}
		}
	}
	// Allocate memmory for point cloud
	temp_point_cloud = (VL_Vector3F *)malloc(sizeof(VL_Vector3F) * (*out_npoints));
	if (NULL == temp_point_cloud) {
		if (NULL != buff_front) free(buff_front);
		if (NULL != buff_left) free(buff_left);
		if (NULL != buff_top) free(buff_top);
		if (NULL != verts_front) free(verts_front);
		if (NULL != verts_left) free(verts_left);
		if (NULL != verts_top) free(verts_top);
		if (NULL != *out_point_cloud) free(*out_point_cloud);
		*out_npoints = 0;
		if (out_point_cloud) { *out_point_cloud = NULL; }
		return NULL;
	}
	for (VL_Size x = 0; x < cx; x++) {
		for (VL_Size y = 0; y < cy; y++) {
			for (VL_Size z = 0; z < cz; z++) {
				id_front = z * cx + x;
				id_left  = z * cy + y;
				id_top   = y * cx + x;
				if (buff_front[id_front] && buff_left[id_left] && buff_top[id_top]) {
					(temp_point_cloud + counter)->x = x * in_vsize + halfsize + vmin.x;
					(temp_point_cloud + counter)->y = y * in_vsize + halfsize + vmin.y;
					(temp_point_cloud + counter)->z = z * in_vsize + halfsize + vmin.z;
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

	if (out_point_cloud) { *out_point_cloud = temp_point_cloud; }
	return temp_point_cloud;
}


