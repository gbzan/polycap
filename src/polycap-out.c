#include "polycap-private.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "hdf5.h"

//===========================================
// Write efficiencies output in a hdf5 file
void polycap_transmission_efficiencies_write_hdf5(const char *filename, polycap_transmission_efficiencies *efficiencies)
{
	hid_t file, dataset;
	hid_t dataspace; //handles
	hsize_t n_energies_temp;
	herr_t status;


	//Create new HDF5 file using H5F_ACC_TRUNC and default creation and access properties
	file = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT); 

	//Write energies array
	//Define temporary dataset dimension
	n_energies_temp = efficiencies->n_energies;
	//Describe size of the array and make fixed data space
	dataspace = H5Screate_simple(1, &n_energies_temp, NULL); //RANK = 1 due to 1D dataset
	//Create new dataset within the HDF5 file with default creation properties
	dataset = H5Dcreate2(file, "Energies", H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	//Write data to the dataset with default transfer properties
	status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, efficiencies->energies);
	if(status < 0){
		printf("Error: H5D write finished on error.\n");
		exit(4);
	}
	//Close release sources
	status = H5Sclose(dataspace);
	if(status < 0){
		printf("Error: H5S close finished on error.\n");
		exit(5);
	}
	status = H5Dclose(dataset);
	if(status < 0){
		printf("Error: H5D close finished on error.\n");
		exit(6);
	}

	//Write efficiencies array
	//Define temporary dataset dimension
	n_energies_temp = efficiencies->n_energies;
	//Describe size of the array and make fixed data space
	dataspace = H5Screate_simple(1, &n_energies_temp, NULL); //RANK = 1 due to 1D dataset
	//Create new dataset within the HDF5 file with default creation properties
	dataset = H5Dcreate2(file, "Transmission Efficiencies", H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	//Write data to the dataset with default transfer properties
	status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, efficiencies->efficiencies);
	if(status < 0){
		printf("Error: H5D write finished on error.\n");
		exit(4);
	}
	//Close release sources
	status = H5Sclose(dataspace);
	if(status < 0){
		printf("Error: H5S close finished on error.\n");
		exit(5);
	}
	status = H5Dclose(dataset);
	if(status < 0){
		printf("Error: H5D close finished on error.\n");
		exit(6);
	}

	//Close file
	H5Fclose(file);
	if(status < 0){
		printf("Error: H5F close finished on error.\n");
		exit(7);
	}

	printf("%s was written.\n",filename);
}
//===========================================
void polycap_transmission_efficiencies_free(polycap_transmission_efficiencies *efficiencies)
{
	free(efficiencies->energies);
	free(efficiencies->efficiencies);
	free(efficiencies);
}
