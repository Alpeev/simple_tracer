#ifndef DARR_H
#define DARR_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define DEF_DARR_TYPE(type, name) \
	typedef struct{ size_t size; size_t p; size_t capacity; type* data;} name; \
	\
	static inline name name##_create(size_t n){\
		name res;\
		res.size = n;\
		res.p = 0;\
		res.capacity = 1;\
		while(res.capacity < res.size){\
			res.capacity*=2;\
			res.p++;\
		}\
		res.data = malloc(res.capacity * sizeof(*res.data));\
		return res;\
	}\
	static inline void name##_destroy(name* vec){\
		free(vec->data);\
		vec->data = NULL;\
	};\
	static inline bool name##_valid(const name* vec){\
		return (vec->data);\
	}\
	static inline name name##_create_filled(size_t n, type val){\
		name res = name##_create(n);\
		if(name##_valid(&res)){\
			for(size_t i = 0; i<n; i++){\
			       res.data[i] = val;\
			}\
		}\
		return res;\
	}\
	static inline bool name##_resize(name* vec, size_t n){\
		vec->size = n;\
		bool rneed =false;\
		if((3*n < vec->capacity) && (vec->capacity > 4)){\
			bool rneed = true;\
			vec->capacity = 1;\
			vec->p=0;\
		}\
		if(n > vec->capacity){\
			rneed = true;\
			while(vec->size > vec->capacity){\
				vec->capacity*=2;\
				vec->p++;\
			}\
		}\
		type *tmp = vec->data;\
		if(rneed){\
			tmp = realloc(vec->data, vec->capacity * sizeof(*vec->data));\
			if(tmp) vec->data = tmp;\
		}\
		return (NULL!=vec->data);\
	}\
	static inline bool name##_pushback(name* vec, type val){\
		if(name##_resize(vec, vec->size+1)){\
			vec->data[vec->size-1] = val;\
			return true;\
		}else{\
			return false;\
		}\
	}\
	static inline void name##_put(name* vec, size_t pos,  type val){\
		vec->data[pos] = val;\
	}\
	static inline type name##_get(const name* vec, size_t pos){\
		return vec->data[pos];\
	}\
	static inline type* name##_at(const name* vec, size_t pos){\
		return vec->data + pos;\
	}\

#endif
