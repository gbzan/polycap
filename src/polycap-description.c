#include "polycap-private.h"
#include <string.h>
#ifdef _WIN32
  #ifndef _CRT_RAND_S
  // needs to be define before including stdlib.h
  #define _CRT_RAND_S // for rand_s -> see https://msdn.microsoft.com/en-us/library/sxtz2fa8.aspx
  #endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <errno.h>

//===========================================
char *polycap_read_input_line(FILE *fptr, polycap_error **error)
{
	char *strPtr;
	unsigned int j = 0;
	int ch;
	unsigned int str_len_max = 128;
	unsigned int str_current_size = 128;

	// Argument sanity check
	if(fptr == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_read_input_line: fptr cannot be NULL");
		return NULL;
	}

	//assign initial string memory size
	strPtr = malloc(str_len_max);
	if(strPtr == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_read_input_line: could not allocate memory for strPtr -> %s", strerror(errno));
		return NULL;
        }

	//read in line character by character
	while( (ch = fgetc(fptr)) != '\n' && ch != EOF){
		strPtr[j++] = ch;
		//if j reached max size, then realloc size
		if(j == str_current_size){
			str_current_size = j + str_len_max;
			strPtr = realloc(strPtr,str_current_size);
		}
	}
	strPtr[j++] = '\0';
	return realloc(strPtr, sizeof(char)*j);
}
//===========================================
void polycap_description_check_weight(size_t nelem, double wi[], polycap_error **error)
{
	int i;
	double sum = 0;

	for(i=0; i<nelem; i++){
		sum += wi[i];
	}

	if(sum > 1.){
		sum = 0;
		for(i=0; i<nelem; i++){
			wi[i] /= 100.0;
			if(wi[i] < 0.0){
				polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_check_weight: Polycapillary element weights must be greater than 0.0");
				return;
			}
			sum += wi[i];
		}
	}
	if(sum == 1.){
		return;
	} else {
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_check_weight: Polycapillary element weights do not sum to 1.");
		return;
	}

}

//===========================================
// get a new polycap_description by providing all its properties
polycap_description* polycap_description_new(double sig_rough, double sig_wave, double corr_length, int64_t n_cap, unsigned int nelem, int iz[], double wi[], double density, polycap_profile *profile, polycap_error **error)
{
	int i;
	polycap_description *description;

	//Perform source_temp and description argument sanity check
	// TODO: more sanity checks??
	if (n_cap <= 1){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: n_cap must be greater than 1");
		return NULL;
	}
	if (nelem < 1){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: nelem must be 1 or greater");
		return NULL;
	}
	if (iz == NULL) {
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: iz cannot be NULL");
		return NULL;
	}
	if (wi == NULL) {
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: wi cannot be NULL");
		return NULL;
	}
	for(i=0; i<nelem; i++){
		if (iz[i] < 1 || iz[i] > 111){
			polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: iz[i] must be greater than 0 and smaller than 111");
			return NULL;
		}
		/*else {
			fprintf(stderr, "iz[%d] -> %d\n", i, iz[i]);
			fprintf(stderr, "wi[%d] -> %g\n", i, wi[i]);
		}*/
	}
	if (density < 0.0){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_description_new: density must be greater than 0.0");
		return NULL;
	}

	//allocate some memory
	description = calloc(1, sizeof(polycap_description));
	if(description == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description -> %s", strerror(errno));
		return NULL;
	}
	description->iz = malloc(sizeof(int)*nelem);
	if(description->iz == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->iz -> %s", strerror(errno));
		free(description);
		return NULL;
	}
	description->wi = malloc(sizeof(double)*nelem);
	if(description->wi == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->wi -> %s", strerror(errno));
		free(description->iz);
		free(description);
		return NULL;
	}

	//copy data into description structure
	description->sig_rough = sig_rough;
	description->sig_wave = sig_wave;
	description->corr_length = corr_length;
	description->n_cap = n_cap;
	description->nelem = nelem;
	description->density = density;
	for(i=0; i<description->nelem; i++){
		description->iz[i] = iz[i];
		description->wi[i] = wi[i]; //assumes weights are already provided as fractions (not percentages)
	}

	// Check whether weights add to 1
	polycap_description_check_weight(description->nelem, description->wi, error);

	//allocate profile memory
	description->profile = calloc(1, sizeof(polycap_profile));
	if(description->profile == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->profile -> %s", strerror(errno));
		free(description->iz);
		free(description->wi);
		free(description);
		return NULL;
	}
	description->profile->nmax = profile->nmax;
	description->profile->z = malloc(sizeof(double)*(profile->nmax+1));
	if(description->profile->z == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->profile->z -> %s", strerror(errno));
		free(description->iz);
		free(description->wi);
		free(description->profile);
		free(description);
		return NULL;
	}
	description->profile->cap = malloc(sizeof(double)*(profile->nmax+1));
	if(description->profile->cap == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->profile->cap -> %s", strerror(errno));
		free(description->iz);
		free(description->wi);
		free(description->profile->z);
		free(description->profile);
		free(description);
		return NULL;
	}
	description->profile->ext = malloc(sizeof(double)*(profile->nmax+1));
	if(description->profile->ext == NULL){
		polycap_set_error(error, POLYCAP_ERROR_MEMORY, "polycap_description_new: could not allocate memory for description->profile->ext -> %s", strerror(errno));
		free(description->iz);
		free(description->wi);
		free(description->profile->cap);
		free(description->profile->z);
		free(description->profile);
		free(description);
		return NULL;
	}
	
	// copy description->profile values to profile
	for(i=0;i<=profile->nmax;i++){
		description->profile->z[i] = profile->z[i];
		description->profile->cap[i] = profile->cap[i];
		description->profile->ext[i] = profile->ext[i];
	}
	//NOTE: user should free old profile memory him/herself

	// Calculate open area
	description->open_area = (description->profile->cap[0]/description->profile->ext[0]) * (description->profile->cap[0]/description->profile->ext[0]) * description->n_cap;

	return description;
}

//===========================================
// get the polycap_profile from a polycap_description
const polycap_profile* polycap_description_get_profile(polycap_description *description)
{
	return description->profile;
}


//===========================================
// free a polycap_description struct
void polycap_description_free(polycap_description *description)
{
	if (description == NULL)
		return;
	polycap_profile_free(description->profile);
	if (description->iz)
		free(description->iz);
	if (description->wi)
		free(description->wi);
	free(description);
}



