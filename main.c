#include "scene.h"
#include "vec_math.h"
#include "video_sdl.h"
#include <stdio.h>
#include "err_print.h"
#include <time.h>

double timediff(struct timespec t1, struct timespec t2){
	return (t2.tv_sec - t1.tv_sec) + 1e-9 * (t2.tv_nsec - t1.tv_nsec);
}

int main( void ){
	size_t w = 1280, h = 800;
	Video video;
	if(!Video_Create(&video, w, h, "Hui")){ 
		ERR_PRINT("Error while initializeing video\n");
		Video_Destroy(&video);
		return EXIT_FAILURE;
	}
	Scene scene;
	Scene_Create(&scene, 0.1, 100.0);
	Camera camera = Camera_Create( w, h, 0.5);
	camera.focus = 0.5;
	camera.rotation = Mat3f_Unity;
	camera.position = (Vec3f){0};
	/*Scene_AddLight(
			&scene, 
			(Light){
			.type = LIGHT_TYPE_AFFINE,
			.affine.direction = Vec3fNormalized((Vec3f){{0.3, 1.0, 0.3}}),
			.affine.intensity = 1.0},
			(Body){
			.surface = BODY_SURFACE_DARKNESS,
			.shape.type = SHAPE_TYPE_HALFSPACE,
			.shape.halfspace.normal = {{0.0, -1.0, 0.0}},
			.shape.halfspace.c = -10.0});*/
	Scene_AddLight(
			&scene, 
			(Light){
			.type = LIGHT_TYPE_POINT,
			.point.center = {{5.0, 15.0, -5.0 }},
			.point.intensity = 400.0},
			(Body){
			.surface = BODY_SURFACE_DARKNESS,
			.shape.type = SHAPE_TYPE_HALFSPACE,
			.shape.halfspace.normal = {{0.0, -1.0, 0.0}},
			.shape.halfspace.c = -10.0});
	
	
	Reflection_Parameters smooth_ha = {.phongCoeff = 1.0, .lambertCoeff = 0.9, .phongExponent = 4.0};	
	Reflection_Parameters smooth_la = {.phongCoeff = 0.7, .lambertCoeff = 0.2, .phongExponent = 4.0};
	Scene_AddBody(
			&scene,
			(Body){
			.surface = BODY_SURFACE_SMOOTH,
			.shape.type = SHAPE_TYPE_HALFSPACE,
			.shape.halfspace.normal = {{0.0, 1.0, 0.0}},
			.shape.halfspace.c = - 3.0,
			.reflectionParameters = smooth_la});
	Scene_AddBody(
			&scene,
			(Body){
			.surface = BODY_SURFACE_SMOOTH,
			.shape.type = SHAPE_TYPE_BALL,
			.reflectionParameters = smooth_ha,
			.shape.ball.center = {{0.0, 0.0, 8.5}},
			.shape.ball.radius = 0.2});

	Scene_AddBody(
			&scene,
			(Body){
			.surface = BODY_SURFACE_SMOOTH,
			.shape.type = SHAPE_TYPE_BALL,
			.reflectionParameters = smooth_ha,
			.shape.ball.center = {{0.0, -1.0, 9.0}},
			.shape.ball.radius = 0.5});
	//Vec3f p;
	//scanf("%f%f%f", p.x, p.x+1, p.x+2);
	//Body *body;
	//printf("dist: %f\n", Scene_Distance(scene, p, &body));
	clock_t t1 = clock();
	struct timespec pt1;
	clock_gettime(CLOCK_MONOTONIC, &pt1);
	//Scene_Project(scene,camera, video.realmap);
	//for(size_t i = 0; i < 10; i++ )
	Parallel_Scene_Project(scene,camera, video.realmap, 16);
	clock_t t2 = clock();
	struct timespec pt2;
       	clock_gettime(CLOCK_MONOTONIC, &pt2);
	printf("Rendering took %f seconds of processor time\n", (double)(t2 -t1)/CLOCKS_PER_SEC);
	printf("Rendering took %f seconds of real time\n", timediff(pt1, pt2));

	Video_RealmapDraw(video);
	WaitExit();
	Scene_Destroy(&scene);
	Video_Destroy(&video);
	return EXIT_SUCCESS;
}
