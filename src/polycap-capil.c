/*
 * Copyright (C) 2018 Pieter Tack, Tom Schoonjans and Laszlo Vincze
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include "polycap-private.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h> //complex numbers required for Fresnel equation
#include <errno.h>

#define NSPOT 1000  /* The number of bins in the grid for the spot*/
#define BINSIZE 20.e-4 /* cm */
#define EPSILON 1.0e-30

//===========================================
// calculates the intersection point coordinates of the photon trajectory and a given linear segment of the capillary wall
STATIC int polycap_capil_segment(polycap_vector3 cap_coord0, polycap_vector3 cap_coord1, double cap_rad0, double cap_rad1, polycap_vector3 *photon_coord, polycap_vector3 photon_dir, polycap_vector3 *surface_norm, double *alfa, polycap_error **error)
{
	double disc, solution1, solution2, sol_final; //discriminant and solutions of formed quadratic equation
	polycap_vector3 photon_coord_rel, cap_coord1_rel; //coordinates of previous photon interaction and current point capillary axis, with previous point capillary axis set as origin [0,0,0]
	double phot_axs_scalar; //cosine of angle between photon propagation and capillary central axis
	double a, b;
	polycap_vector3 aa, bb;
	double a0, b0, c0;
	double d_travel; //distance traveled by photon until next interaction
	polycap_vector3 s, u; //coordinates of capillary axis at interaction distance (s) and normalized interaction coordinates (u)
	double au, ads; //distance between capillary axis and interaction point (au), distance between cap_coords
	double tga, sga, cga, gam; //tan(gamma), sin(ga) and cos(ga) and gamma where gamma is angle between capillary wall and axis

	//argument sanity check
	if (cap_coord0.z < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: cap_coord0.z must be greater than 0");
		return -1;
	}
	if (cap_coord1.z < 0){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: cap_coord0.z must be greater than 0");
		return -1;
	}
	if (cap_rad0 < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: cap_rad0 must be greater than 0");
		return -1;
	}
	if (cap_rad1 < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: cap_rad1 must be greater than 0");
		return -1;
	}
	if (photon_coord == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: photon_coord must not be NULL");
		return -1;
	}
	if (photon_dir.z < 0){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: photon_dir.z must be greater than 0");
		return -1;
	}
	if (photon_dir.z < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: photon_dir.z must be greater than 0");
		return -1;
	}
	if (surface_norm == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: surface_norm must not be NULL");
		return -1;
	}
	if (alfa == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_segment: alfa must not be NULL");
		return -1;
	}

	*alfa = 0.0; //angle between surface normal and photon direction, set to 0 for now in case of premature return
	sol_final = -1000;

	photon_coord_rel.x = photon_coord->x - cap_coord0.x;
	photon_coord_rel.y = photon_coord->y - cap_coord0.y;
	photon_coord_rel.z = photon_coord->z - cap_coord0.z;

	cap_coord1_rel.x = cap_coord1.x - cap_coord0.x;
	cap_coord1_rel.y = cap_coord1.y - cap_coord0.y;
	cap_coord1_rel.z = cap_coord1.z - cap_coord0.z;
	phot_axs_scalar = polycap_scalar(photon_dir, cap_coord1_rel); //cos(angle)/(|v1|*|v2|)
	if(fabs(phot_axs_scalar) < EPSILON){
		return -2; //selects new section of capillary to check interaction for
	}

	a = -1*polycap_scalar(photon_coord_rel, cap_coord1_rel) / phot_axs_scalar;
	b = polycap_scalar(cap_coord1_rel, cap_coord1_rel) / phot_axs_scalar;

	aa.x = photon_coord->x + a*photon_dir.x - cap_coord0.x;	
	aa.y = photon_coord->y + a*photon_dir.y - cap_coord0.y;	
	aa.z = photon_coord->z + a*photon_dir.z - cap_coord0.z;

	bb.x = b*photon_dir.x - cap_coord1.x + cap_coord0.x;
	bb.y = b*photon_dir.y - cap_coord1.y + cap_coord0.y;
	bb.z = b*photon_dir.z - cap_coord1.z + cap_coord0.z;

	a0 = polycap_scalar(bb, bb) - (cap_rad1 - cap_rad0)*(cap_rad1 - cap_rad0);
	b0 = 2.* ( polycap_scalar(aa, bb) - cap_rad0 * (cap_rad1 - cap_rad0) );
	c0 = polycap_scalar(aa, aa) - cap_rad0 * cap_rad0;
	if(fabs(a0) < EPSILON){
		solution1 = -c0/b0;
		solution2 = -1000.;
	} else {
		disc = b0*b0 - 4.*a0*c0;
		if(disc < 0.){
			return -3; //no solution so select new section of capillary
		}
		disc = sqrt(disc);
		solution1 = (-b0+disc)/(2.*a0);
		solution2 = (-b0-disc)/(2.*a0);
	}
	if(solution1 > EPSILON && solution1 <= 1.) sol_final = solution1;
	if(solution2 > EPSILON && solution2 <= 1.) sol_final = solution2;
	if(sol_final == -1000){
		return -4;
	}

	d_travel = a + sol_final*b;
	if(d_travel < 1.e-10){
		return -5;
	}

	//location of next intersection point
	photon_coord->x = photon_coord->x + d_travel * photon_dir.x;
	photon_coord->y = photon_coord->y + d_travel * photon_dir.y;
	photon_coord->z = photon_coord->z + d_travel * photon_dir.z;

	//new point along capillary axis at intersection distance
	s.x = cap_coord0.x + sol_final * cap_coord1_rel.x;
	s.y = cap_coord0.y + sol_final * cap_coord1_rel.y;
	s.z = cap_coord0.z + sol_final * cap_coord1_rel.z;
	//normalized coordinates of intersection point compared to axis
	u.x = photon_coord->x - s.x;
	u.y = photon_coord->y - s.y;
	u.z = photon_coord->z - s.z;

	//surface normal at the new intersection point
	au = sqrt(polycap_scalar(u, u));
	ads = sqrt(polycap_scalar(cap_coord1_rel, cap_coord1_rel));

	tga = (cap_rad0 - cap_rad1)/ads; //note: this will be negative if rad0 < rad1 (confocal geometry)
	gam = atan(tga);
	sga = sin(gam);
	cga = cos(gam);

	surface_norm->x = cga * u.x / au + sga * cap_coord1_rel.x / ads;
	surface_norm->y = cga * u.y / au + sga * cap_coord1_rel.y / ads;
	surface_norm->z = cga * u.z / au + sga * cap_coord1_rel.z / ads;
	polycap_norm(surface_norm);

	*alfa = acos(polycap_scalar(*surface_norm,photon_dir)); //angle between surface normal and photon direction
	if(cos(*alfa) < 0.0){
		return -6;
	}

	return 0;
}

//===========================================
STATIC double polycap_refl(double e, double theta, double density, double scatf, double lin_abs_coeff, polycap_error **error) {
	// scatf = SUM( (weight/A) * (Z + f')) over all elements in capillary material
	double complex alfa, beta; //alfa and beta component for Fresnel equation delta term (delta = alfa - i*beta)
	double complex rtot; //reflectivity

	//argument sanity check
	if (e < 1. || e > 100.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_refl: e must be greater than 1 and smaller than 100.");
		return -1;
	}
	if (theta < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_refl: theta must be greater than 0");
		return -1;
	}
	if (density <= 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_refl: density must be greater than 0");
		return -1;
	}
	if (scatf < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_refl: scatf must be greater than 0");
		return -1;
	}
	if (lin_abs_coeff < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_refl: lin_abs_coeff must be greater than 0");
		return -1;
	}
	

	alfa = (double)(HC/e)*(HC/e)*((N_AVOG*R0*density)/(2*M_PI)) * scatf;
	beta = (double) (HC)/(4.*M_PI) * (lin_abs_coeff/e);

	rtot = ((complex double)theta - csqrt(cpow((complex double)theta,2) - 2.*(alfa - beta*I))) / ((complex double)theta + csqrt(cpow((complex double)theta,2) - 2.*(alfa - beta*I)));
	rtot = creal(cpow(cabs(rtot),2.));

	return rtot;
}

//===========================================
STATIC int polycap_capil_reflect(polycap_photon *photon, double alfa, polycap_error **error)
{
	int i, iesc=0;
	double d_esc;  //distance in capillary at which photon escaped divided by propagation vector in z direction
	double cons1, r_rough;
	double complex rtot; //reflectivity
//	double w_leak; //leak weight
//	double xp, yp; //position on screen where photon will end up if unobstructed
//	int ind_x, ind_y; //indices of screen where photon will hit screen

	//argument sanity check
	if (alfa < 0.){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_reflect: alfa must be greater than 0");
		return -1;
	}
	if (photon == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_reflect: photon must not be NULL");
		return -1;
	}
	polycap_description *description = photon->description;
	if (description == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_reflect: description must not be NULL");
		return -1;
	}
	

	d_esc = (description->profile->z[description->profile->nmax] - photon->exit_coords.z) / photon->exit_direction.z;
	if(d_esc < 0) d_esc = description->profile->z[description->profile->nmax];
	for(i=0; i < photon->n_energies; i++){
		cons1 = (1.01358e0*photon->energies[i])*alfa*description->sig_rough;
		r_rough = exp(-1.*cons1*cons1);

		//reflectivity according to Fresnel expression
		rtot = polycap_refl(photon->energies[i], alfa, description->density, photon->scatf[i], photon->amu[i], error);

//		w_leak = (1.-rtot) * photon->weight[i] * exp(-1.*d_esc * photon->amu[i]);
//		leak[i] = leak[i] + w_leak;
//		if(i == 0){ //NOTE: essentially do this for each energy to obtain photon flux image for each energy
//			xp = photon->exit_coords.x + d_esc * photon->exit_direction.x;
//			yp = photon->exit_coords.y + d_esc * photon->exit_direction.y;
//			ind_x = (int)floor(xp/BINSIZE)+NSPOT/2;
//			ind_y = (int)floor(yp/BINSIZE)+NSPOT/2;
//			if(ind_x < NSPOT && ind_x >= 0){
//				if(ind_y < NSPOT && ind_y >= 0){
//					lspot[ind_x][ind_y] = lspot[ind_x][ind_y] + wleak;
//				}
//			}
//		}
		photon->weight[i] = photon->weight[i] * rtot * r_rough;
	}

	if(photon->weight[0] < 1.e-4) iesc=-2;

	return iesc;
}

//===========================================
// trace photon through capillary
HIDDEN int polycap_capil_trace(int *ix, polycap_photon *photon, polycap_description *description, double *cap_x, double *cap_y, polycap_error **error)
{
	int i, iesc=0;
	double cap_rad0, cap_rad1;
	polycap_vector3 cap_coord0, cap_coord1;
	polycap_vector3 photon_coord, photon_dir;
	polycap_vector3 surface_norm; //surface normal of capillary at interaction point
	double alfa; //angle between capillary normal at interaction point and photon direction before interaction
	polycap_vector3 photon_coord_rel; //relative coordinates of new interaction point compared to previous interaction
	double d_travel; //distance between interactions
//	double w0, w1; //photon weights

	//argument sanity check
	if (ix == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_trace: ix must not be NULL");
		return -1;
	}
	if (photon == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_trace: photon must not be NULL");
		return -1;
	}
	if (description == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_trace: description must not be NULL");
		return -1;
	}
	if (cap_x == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_trace: cap_x must not be NULL");
		return -1;
	}
	if (cap_y == NULL){
		polycap_set_error_literal(error, POLYCAP_ERROR_INVALID_ARGUMENT, "polycap_capil_trace: cap_y must not be NULL");
		return -1;
	}
	
	//calculate next intersection point
	if(photon->i_refl == 0) *ix = 0;
	photon_coord.x = photon->exit_coords.x;
	photon_coord.y = photon->exit_coords.y;
	photon_coord.z = photon->exit_coords.z;
	photon_dir.x = photon->exit_direction.x;
	photon_dir.y = photon->exit_direction.y;
	photon_dir.z = photon->exit_direction.z;
	for(i=*ix+1; i<=description->profile->nmax; i++){
		cap_coord0.x = cap_x[i-1];
		cap_coord0.y = cap_y[i-1];
		cap_coord0.z = description->profile->z[i-1];
		cap_rad0 = description->profile->cap[i-1];
		cap_coord1.x = cap_x[i];
		cap_coord1.y = cap_y[i];
		cap_coord1.z = description->profile->z[i];
		cap_rad1 = description->profile->cap[i];
		iesc = polycap_capil_segment(cap_coord0, cap_coord1, cap_rad0, cap_rad1, &photon_coord, photon_dir, &surface_norm, &alfa, error);
		if(iesc == 0){
			*ix = i-1;
			break;
		}
	}

	if(iesc != 0){
		iesc = 1;
	} else { //iesc == 0, so broke out of above for loop and thus found next interaction point
		photon_coord_rel.x = photon_coord.x - photon->exit_coords.x;
		photon_coord_rel.y = photon_coord.y - photon->exit_coords.y;
		photon_coord_rel.z = photon_coord.z - photon->exit_coords.z;
		d_travel = sqrt(polycap_scalar(photon_coord_rel, photon_coord_rel));
		photon->d_travel += d_travel;

		//store new interaction coordiantes in apprpriate array
		photon->exit_coords.x = photon_coord.x;
		photon->exit_coords.y = photon_coord.y;
		photon->exit_coords.z = photon_coord.z;
		if(fabs(cos(alfa)) >1.0){
			printf("COS(alfa) > 1\n");
			iesc = -1;
		} else {
			alfa = M_PI_2 - alfa;
//			w0 = photon->weight[0];
			
			iesc = polycap_capil_reflect(photon, alfa, error);

			if(iesc != -2){
//				w1 = photon->weight[0];
//				calc->absorb[*ix] = calc->absorb[*ix] + w0 - w1;

				photon->exit_direction.x = photon->exit_direction.x - 2.0*sin(alfa) * surface_norm.x;
				photon->exit_direction.y = photon->exit_direction.y - 2.0*sin(alfa) * surface_norm.y;
				photon->exit_direction.z = photon->exit_direction.z - 2.0*sin(alfa) * surface_norm.z;
				polycap_norm(&photon->exit_direction);
				photon->i_refl++;
			}
		}
	}

	return iesc;
}

//===========================================
