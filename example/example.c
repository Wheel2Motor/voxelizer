#include <voxelizer.h>
#include <string.h>


#define BUFLEN 2048


int main() {

	char buff[BUFLEN];
	FILE * fp;
	VL_Size npoints;
	VL_Vector3F * point_cloud;
	VL_Vector3F * outverts;
	VL_Size * outfaces;
	VL_Size outnverts;
	VL_Size outnfaces;
	VL_Vector3F tverts[] = {
		{  1.0,  1.0, -1.0 },
		{ -1.0, -1.0, -1.0 },
		{  0.0,  0.0,  0.0 },
	};
	VL_Size tfaces[] = {
		0, 1, 2,
	};

	// Calculate volume
	VL_Float volume = vl_volume_from_mesh(tverts, 3, tfaces, 1, 0.1);
	printf("Volume: %lf\n", volume);

	// Write obj
	vl_point_cloud_from_mesh(&point_cloud, &npoints, tverts, 3, tfaces, 1, 0.01);
	vl_mesh_from_point_cloud(&outverts, &outnverts, &outfaces, &outnfaces, point_cloud, npoints, 0.01);
	fp = fopen("example.obj", "w");
	sprintf(buff, "o Voxel\n");
	fwrite(buff, sizeof(char), strlen(buff), fp);
	for (VL_Size i = 0; i < outnverts; i++) {
		const VL_Vector3F * const v = outverts + i;
		sprintf(buff, "v %.6lf %.6lf %.6lf\n", v->x, v->y, v->z);
		fwrite(buff, sizeof(char), strlen(buff), fp);
	}
	sprintf(buff, "s off\n");
	fwrite(buff, sizeof(char), strlen(buff), fp);
	for (VL_Size i = 0; i < outnfaces; i++) {
		const VL_Size * const t0 = outfaces + i * 3;
		const VL_Size * const t1 = t0 + 1;
		const VL_Size * const t2 = t1 + 1;
		sprintf(buff, "f %zu %zu %zu\n", (*t0) + 1, (*t1) + 1, (*t2) + 1);
		fwrite(buff, sizeof(char), strlen(buff), fp);
	}
	fclose(fp);
	free(point_cloud);
	free(outverts);
	free(outfaces);

	return 0;
}

