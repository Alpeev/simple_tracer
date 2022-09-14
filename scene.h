//scene.h 
//Provides scene an solid geometry functions

#ifndef TRACER_SCENE_H
#define TRACER_SCENE_H

#include <stdio.h>
#include <math.h>
#include "darr.h"
#include "vec_math.h"
#include "err_print.h"
#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>

typedef enum {
	SHAPE_TYPE_HALFSPACE, SHAPE_TYPE_BALL, SHAPE_TYPES
} Shape_Type;

typedef enum {
	BODY_SURFACE_DARKNESS, BODY_SURFACE_SMOOTH, BODY_SURFACES
} Body_Surface;

typedef struct Shape Shape;

typedef struct {
	Vec3f normal;
	float c;
} Shape_Halfspace;

typedef struct {
	Vec3f center;
	float radius;
} Shape_Ball;

struct Shape {
	Shape_Type type;
	union {
		Shape_Halfspace halfspace;
		Shape_Ball ball;
	};
};

typedef struct {
	float phongCoeff;
	float phongExponent;
	float lambertCoeff;
} Reflection_Parameters;

typedef struct {
	Body_Surface surface;
	Reflection_Parameters reflectionParameters;
	Shape shape;
} Body;	

typedef enum {
	LIGHT_TYPE_AFFINE, LIGHT_TYPE_POINT, LIGHT_TYPES
} Light_Type;

typedef struct {
	Vec3f direction;
	float intensity;
} Light_Affine;

typedef struct {
	Vec3f center;
	float intensity;
} Light_Point;

typedef struct {
	Light_Type type;
	size_t source;
	union {
		Light_Affine affine;
		Light_Point point;
	};
} Light;

DEF_DARR_TYPE(Body, Bodies);
DEF_DARR_TYPE(Light, Lights);

typedef struct {
	float ambientLight;
	float bound;
	Bodies bodies;
	Lights lights;
} Scene;


const size_t Scene_Steps = 200;
const float Scene_Eps_in = 0.001;
const float Scene_Outfactor = 0.5;
const float Scene_March_Coeff = 0.99;
const float Scene_March_Jump = 0.01;

typedef struct {
	size_t w,h;
	float dx, dy;
	Mat3f rotation;
	Vec3f position;
	float focus;
} Camera;

static inline float Shape_Distance( const Shape shape, const Vec3f point){
	switch(shape.type){
	case SHAPE_TYPE_HALFSPACE: 
		return Vec3fDot( shape.halfspace.normal, point) - shape.halfspace.c;
	case SHAPE_TYPE_BALL:
		return Vec3fNorm( Vec3fSub(point, shape.ball.center)) - shape.ball.radius;
	default: 
		ERR_PRINT("Unknown Shape_Type");
		return 0.0;
	}
}

static inline float Body_Distance( const Body body, const Vec3f point){
	return Shape_Distance( body.shape, point);
}


static inline float Scene_Distance( const Scene scene, const Vec3f point, Body **body){
	float dist = +INFINITY;
	*body = NULL;
	for(size_t i = 0; i < scene.bodies.size; i++){
		const float bd = Body_Distance(scene.bodies.data[i], point);
		if(bd < dist) dist = bd;
		if(bd <= Scene_Eps_in){
			*body = &scene.bodies.data[i];
			break;
		}
	}
	return dist;
}


static inline Vec3f Shape_Normal(const Shape shape, const Vec3f point){
	switch (shape.type){
	case SHAPE_TYPE_HALFSPACE:
		return shape.halfspace.normal;
	case SHAPE_TYPE_BALL:
		return Vec3fNormalized(Vec3fSub(point, shape.ball.center));
	default:
		ERR_PRINT("Unknown Shape_Type");
		return (Vec3f){0};
	}
}


static inline Vec3f Body_Normal( const Body body, const Vec3f point){
	return Shape_Normal(body.shape, point);
}


static inline bool Scene_March( 
		const Scene scene, 
		const Vec3f start_point, 
		const Vec3f direction, 
		Vec3f *const endpoint, 
		Body const**const body){

	size_t steps = 0;
	if(Vec3fNorm(start_point) > scene.bound) return false;
	Vec3f point = start_point;
	Body *nearest_body;
	float start_dist = Scene_Distance(scene, point, &nearest_body);
	float dist = start_dist;
	/*if(start_dist < Scene_Eps_in ){
		while(steps < Scene_Steps){
			point = Vec3fAdd(point, Vec3fMul(direction, dist * Scene_March_Coeff));
			if(Vec3fNorm(point) > scene.bound) return false;
			dist = Scene_Distance( scene, point, &nearest_body);
			if(dist < start_dist * Scene_Outfactor)
				return false;
			if(dist > Scene_Eps_in)
				break;
			steps++;
		}
	}*/
	point = Vec3fAdd(point, Vec3fMul(direction, Scene_March_Jump));
	steps = 0;
	while(steps < Scene_Steps){
		point = Vec3fAdd(point, Vec3fMul(direction, dist * Scene_March_Coeff));
		if(Vec3fNorm(point) > scene.bound) return false;
		dist = Scene_Distance( scene, point, &nearest_body);
		if(dist < Scene_Eps_in)
			break;
		steps++;
	}
	if(steps == Scene_Steps)
		return false;
	*endpoint = point;
	*body = nearest_body;
	return true;
}	

//Wow, here we completely decouple the mechanics of light source from that of a surface reflectance!!!
static inline bool Light_DirectionAndIntensity( 
		const Scene scene, 
		const Light light, 
		const Vec3f point, 
		Vec3f *const direction, 
		float *const intensity){

	Vec3f direction_normalized;
	if(Body_Distance(Bodies_get(&scene.bodies, light.source), point) < Scene_Eps_in) return false;
	switch (light.type){
	case LIGHT_TYPE_AFFINE:
		*direction = direction_normalized = light.affine.direction;
		*intensity = light.affine.intensity; 
		break;
	case LIGHT_TYPE_POINT:
	{
		Vec3f direction_unnormalized = Vec3fSub(light.point.center, point);
		float dist = Vec3fNorm(direction_unnormalized);
		*direction = direction_normalized = Vec3fNormalized(direction_unnormalized);
		*intensity = light.point.intensity/(dist * dist);
		break;
	}
	default:
		return false;
		break;
	}

	const Body* body_ptr;
	Vec3f end_point;
	if(!Scene_March(scene, point, direction_normalized, &end_point, &body_ptr)) return false;

	return (body_ptr == Bodies_at(&scene.bodies,light.source));
}	

static inline float Body_Lighting( 
		const Scene scene, 
		const Body body, 
		const Vec3f point, 
		const Vec3f view_direction){

	float res = 0.0;
	if(BODY_SURFACE_DARKNESS == body.surface){
		return 0.0;
	}
	const Vec3f normal = Body_Normal(body, point);
	const float view_normal_dot = Vec3fDot(view_direction, normal);
	//float light_normal_dot = Vec3fDot(light_direction, normal);
	if(view_normal_dot >= 0)
		return 0.0;

	res = scene.ambientLight;
	for( size_t i = 0; i < scene.lights.size; i++){ 
		
		const Light* light_ptr = Lights_at(&scene.lights, i);
		Vec3f light_direction;
		float light_intensity;

		if(!Light_DirectionAndIntensity(
					scene,
					*light_ptr,
					point,
					&light_direction,
					&light_intensity))
			continue;

		const float light_normal_dot = Vec3fDot(light_direction, normal);

		if(light_normal_dot <= 0.0)
			continue;

		res += light_intensity * light_normal_dot * body.reflectionParameters.lambertCoeff;
		
		const float view_light_dot = Vec3fDot(view_direction, light_direction);
		const float bounce_view_light_dot = view_light_dot - 2.0 * light_normal_dot * view_normal_dot;
		if(bounce_view_light_dot <= 0.0)
			continue;
		
		res += body.reflectionParameters.phongCoeff * powf(bounce_view_light_dot, body.reflectionParameters.phongExponent) * light_intensity;
	}
	return res;
}

static inline float Scene_Lighting( 
		const Scene scene, 
		const Vec3f point, 
		const Vec3f direction){
	
	const Body *body_ptr;
	Vec3f first_intersection;
	if(!Scene_March(scene, point, direction, &first_intersection, &body_ptr)) return 0.0;
	//ERR_PRINT("not null after first scene march");
	return Body_Lighting(scene, *body_ptr, first_intersection, direction);
}

static inline void Scene_Project( 
		const Scene scene, 
		const Camera camera, 
		float* const pixels){

	for(size_t j = 0; j < camera.h; j++){
		for(size_t i = 0; i < camera.w; i++){
			const Vec3f unrotated = {{
				((float)i - (float)camera.w/2) * camera.dx,
				((float)j - (float)camera.h/2) * camera.dy,
				camera.focus}};
			const Vec3f rotated = Mat3fVec3fMul(camera.rotation, unrotated);
			const Vec3f direction = Vec3fNormalized(rotated);
			const Vec3f point = Vec3fAdd(rotated, camera.position);
			float lighting = Scene_Lighting(scene,point,direction);
			//if(fabs(lighting)> 0.001) fprintf(stderr, "Yay, nonzero pixel!\n");
			pixels[3 * (camera.w * j + i) + 0] = lighting;
			pixels[3 * (camera.w * j + i) + 1] = lighting;
			pixels[3 * (camera.w * j + i) + 2] = lighting;

		}
	}
}

typedef struct {
	Scene scene;
	Camera camera;
	 float* pixels;
	size_t mod;
	size_t step;
} Scene_Project_Parameters;

extern void* Parallel_Scene_Project_Func( void* par ){

	Scene_Project_Parameters params = *(Scene_Project_Parameters*) par;
	const Scene scene = params.scene;
	const Camera camera = params.camera;
	 float * const pixels = params.pixels;
	size_t mod = params.mod;
	size_t step = params.step;
	for(size_t j = mod; j < camera.h; j+= step){
		for(size_t i = 0; i < camera.w; i++){
			const Vec3f unrotated = {{
				((float)i - (float)camera.w/2) * camera.dx,
				((float)j - (float)camera.h/2) * camera.dy,
				camera.focus}};
			const Vec3f rotated = Mat3fVec3fMul(camera.rotation, unrotated);
			const Vec3f direction = Vec3fNormalized(rotated);
			const Vec3f point = Vec3fAdd(rotated, camera.position);
			float lighting = Scene_Lighting(scene,point,direction);
			//if(fabs(lighting)> 0.001) fprintf(stderr, "Yay, nonzero pixel!\n");
			
			pixels[3 * (camera.w * j + i) + 0] = lighting;
			pixels[3 * (camera.w * j + i) + 1] = lighting;
			pixels[3 * (camera.w * j + i) + 2] = lighting;

		}
	}
	return NULL;
}

extern void Parallel_Scene_Project(
		const Scene scene, 
		const Camera camera, 
		 float* const pixels,
		const size_t numthreads){

	Scene_Project_Parameters base_params = {
		.scene = scene,
		.camera = camera,
		.pixels = pixels,
		.step = numthreads};
	Scene_Project_Parameters params[numthreads];
	pthread_t tids[numthreads];
	for(size_t i = 0; i < numthreads; i++){
		params[i] = base_params;
		params[i].mod = i;
		pthread_create(&tids[i], NULL, Parallel_Scene_Project_Func, &params[i]);
	}
	for(size_t i = 0; i < numthreads; i++){
		pthread_join(tids[i], NULL);
	}
}

static inline bool Scene_AddBody(Scene *scene, const Body body){
	return Bodies_pushback(&scene->bodies, body);
}

static inline bool Scene_AddLight(Scene *scene, const Light light, const Body source){

	if(!Scene_AddBody(scene, source))
		return false;
	if(!Lights_pushback(&scene->lights, light)){
		Bodies_resize(&scene->bodies, scene->bodies.size - 1);
		return false;
	}else{
		return false;
	}
}


static inline void Scene_Destroy( Scene *scene){

	Bodies_destroy(&scene->bodies);
	Lights_destroy(&scene->lights);
	*scene = (Scene){0};
}	

static inline bool Scene_Create( Scene *scene, const float ambientLight, const float bound){

	*scene = (Scene){0};
	scene->ambientLight = ambientLight;
	scene->bound = bound;
	scene->bodies = Bodies_create(0);
	scene->lights = Lights_create(0);
	if(Bodies_valid(&scene->bodies) && Lights_valid(&scene->lights)){
		return true;
	}else{
		Scene_Destroy(scene);
		return false;
	}

}

static inline Camera Camera_Create(size_t w , size_t h, float scrw){
	return (Camera){
		.w = w,
		.h = h,
		.dx = scrw/w,
		.dy = -scrw/w};	
}


#endif
