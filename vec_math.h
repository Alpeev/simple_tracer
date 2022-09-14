#ifndef VEC_MATH_H
#define VEC_MATH_H

#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include "err_print.h"


typedef struct {
	float x[3];
} Vec3f;

typedef struct {
	float x[3][3];
} Mat3f;

static const Mat3f Mat3f_Unity = {.x = {
	{1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0}}};

static inline Vec3f ArrToVec3f( const float a[static 3]){
	return (Vec3f){{a[0], a[1], a[2]}};
}

static inline void Vec3fToArr( float a[], const Vec3f v){
	for(size_t i = 0; i < 3; i++) a[i] = v.x[i];
}

static inline float Vec3fDot(const Vec3f a, const Vec3f b){
	float res = 0.0;
	for(size_t i = 0; i < 3; i++) res+= a.x[i] * b.x[i];
	return res;
}

static inline float Vec3fNorm(const Vec3f a){
	return sqrtf(Vec3fDot(a,a));
}

static inline Vec3f Vec3fMul(const Vec3f a, const float b){
	Vec3f res = a;
	for(size_t i = 0; i < 3; i++) res.x[i] *= b;
	return res;
}

static inline Vec3f Vec3fAdd(const Vec3f a, const Vec3f b){
	Vec3f res = a;
	for(size_t i = 0; i < 3; i++) res.x[i]+= b.x[i];
	return res;
}

static inline Vec3f Vec3fSub(const Vec3f a, const Vec3f b){
	Vec3f res = a;
	for(size_t i = 0; i < 3; i++) res.x[i]-= b.x[i];
	return res;
}

static inline Vec3f Vec3fPMul(const Vec3f a, const Vec3f b){
	Vec3f res = a;
	for(size_t i = 0; i < 3; i++) res.x[i]*= b.x[i];
	return res;
}

static inline Vec3f Vec3fNormalized(const Vec3f a){
	float norm = Vec3fNorm(a);
	return Vec3fMul(a, 1.0/norm);
}

static inline Vec3f Mat3fVec3fMul( const Mat3f a, const Vec3f b){
	Vec3f res;
	for(size_t i = 0; i < 3; i++) res.x[i] = Vec3fDot(ArrToVec3f(a.x[i]), b);
	return res;

}

static inline Mat3f Mat3fMat3fMul( const Mat3f a, const Mat3f b){
	Mat3f res;
	for(size_t i = 0; i < 3; i++){
		for(size_t j = 0; j < 3; j++){
			for(size_t k = 0; k < 3; k++){
				res.x[i][j] = a.x[i][k] * b.x[k][j];
			}
		}
	}
	return res;
}

static inline Mat3f Mat3fUnitarised( const Mat3f a, bool *tok){
	Mat3f res = a;
	Vec3f tres[3];
	for(size_t i = 0; i < 3; i++) tres[i] = ArrToVec3f(a.x[i]);
	for( size_t i = 0; i < 3; i++){
		if(Vec3fNorm(tres[i]) == 0.0) {
#ifdef DEBUG 
			ERR_PRINT("Matrix unitarisation failed, zero norm");
#endif
			if(tok) *tok = false;
			return res;
		}
		tres[i] = Vec3fNormalized(tres[i]);
		for(size_t j = i ; j < 3; j++){
			tres[j] = Vec3fAdd( tres[j], Vec3fMul( tres[i], -Vec3fDot(tres[i], tres[j])));
		}
	}
	for(size_t i = 0 ; i < 3; i++) Vec3fToArr(res.x[i], tres[i]);
	return res;
}

#endif
