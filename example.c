#include "voxelizer.h"
#include <string.h>


int main() {

	char buff[2048];

	VL_Vector3 tverts[] = {
		{  1.0,  1.0, -1.0 },
		{ -1.0, -1.0, -1.0 },
		{  0.0,  0.0,  1.0 },
	};
	size_t tfaces[] = {
		0, 1, 2,
	};

	// Calculate volume
	double volume = vl_volume_from_mesh(tverts, 3, tfaces, 1, 0.10);
	printf("Volume: %lf\n", volume);

	// Write obj
	size_t npoints;
	VL_Vector3 * point_cloud;
	VL_Vector3 * outverts;
	size_t * outfaces;
	size_t outnverts;
	size_t outnfaces;
	FILE * fp;
	vl_point_cloud_from_mesh(&point_cloud, &npoints, tverts, 3, tfaces, 1, 0.05);
	vl_mesh_from_point_cloud(&outverts, &outnverts, &outfaces, &outnfaces, point_cloud, npoints, 0.05);
	fp = fopen("example.obj", "w");
	sprintf(buff, "o Voxel\n");
	fwrite(buff, sizeof(char), strlen(buff), fp);
	for (size_t i = 0; i < outnverts; i++) {
		const VL_Vector3 * const v = outverts + i;
		sprintf(buff, "v %.6lf %.6lf %.6lf\n", v->x, v->y, v->z);
		fwrite(buff, sizeof(char), strlen(buff), fp);
	}
	sprintf(buff, "s off\n");
	fwrite(buff, sizeof(char), strlen(buff), fp);
	for (size_t i = 0; i < outnfaces; i++) {
		const size_t * const t0 = outfaces + i * 3 + 0;
		const size_t * const t1 = outfaces + i * 3 + 1;
		const size_t * const t2 = outfaces + i * 3 + 2;
		sprintf(buff, "f %zu %zu %zu\n", (*t0) + 1, (*t1) + 1, (*t2) + 1);
		fwrite(buff, sizeof(char), strlen(buff), fp);
	}
	fclose(fp);
	free(point_cloud);
	free(outverts);
	free(outfaces);

	return 0;
}

