/*  Copyright 2018 Oxford Nanopore Technologies, Ltd */

/*  This Source Code Form is subject to the terms of the Oxford Nanopore
 *  Technologies, Ltd. Public License, v. 1.0. If a copy of the License 
 *  was not  distributed with this file, You can obtain one at
 *  http://nanoporetech.com
 */

// fast5_interface needs cleaning
#define BANANA 1
#define _C99_SOURCE  // Required for snprintf on Mac (not set by clang -std=c99)
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fast5_interface.h"
#include "flappie_stdlib.h"
#include "util.h"

struct _gop_data {
    const char *prefix;
    int latest;
};

typedef struct {
    //  Information for scaling raw data from ADC values to pA
    float digitisation;
    float offset;
    float range;
    float sample_rate;
} fast5_raw_scaling;


float read_float_attribute(hid_t group, const char *attribute) {
    float val = NAN;
    if (group < 0) {
        warnx("Invalid group passed to %s:%d.", __FILE__, __LINE__);
        return val;
    }

    hid_t attr = H5Aopen(group, attribute, H5P_DEFAULT);
    if (attr < 0) {
        warnx("Failed to open attribute '%s' for reading.", attribute);
        return val;
    }

    H5Aread(attr, H5T_NATIVE_FLOAT, &val);
    H5Aclose(attr);

    return val;
}


hid_t open_or_create_hdf5(const char * filename){
    hid_t hdf5out = -1;
    if(NULL == filename){
        return hdf5out;
    }

    int fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if(fd < 0){
        hdf5out = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
    } else {
        close(fd);
        unlink(filename);
        hdf5out = H5Fcreate(filename, H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
    }
    return hdf5out;
}


hid_t create_group(hid_t root, const char * name){
    if(root < 0 || NULL == name){
        return -1;
    }
    hid_t group = H5Gcreate(root, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    return group;
}


hid_t set_compression(int rank, hsize_t *chunk_size, int compression_level){
    assert(compression_level >= 0 && compression_level <= 9);

    hid_t properties = H5P_DEFAULT;
    if (compression_level > 0) {
        properties = H5Pcreate(H5P_DATASET_CREATE);
        if (properties < 0) {
            warnx
                ("Failed to create properties structure to write out compressed data structure.\n");
            properties = H5P_DEFAULT;
        } else {
            H5Pset_shuffle(properties);
            H5Pset_deflate(properties, compression_level);
            H5Pset_chunk(properties, rank, chunk_size);
        }
    }
    return properties;
}


herr_t write_signal(hid_t root, const float * raw, size_t n,
                    hsize_t chunk_size, int compression_level){
    if(root < 0 || NULL == raw){
        return -1;
    }
    hsize_t nh = n;

    hid_t space = H5Screate_simple(1, &nh, &nh);
    hid_t properties = set_compression(1, &chunk_size, compression_level);
    hid_t dset = H5Dcreate(root, "signal", H5T_IEEE_F32LE, space, H5P_DEFAULT, properties, H5P_DEFAULT);
    herr_t status = H5Dwrite(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, raw);

    H5Dclose(dset);
    H5Pclose(properties);
    H5Sclose(space);

    return status;    
}


herr_t write_trace(hid_t root, const int32_t * trace, size_t nstate, size_t nblk,
                   hsize_t chunk_size, int compression_level){
    if(root < 0 || NULL == trace){
        return -1;
    }
    hsize_t nh[2] = {nblk, nstate};
    hsize_t ch[2] = {chunk_size, nstate};

    hid_t space = H5Screate_simple(2, nh, nh);
    hid_t properties = set_compression(2, ch, compression_level);
    hid_t dset = H5Dcreate(root, "trace", H5T_STD_U8LE, space, H5P_DEFAULT, properties, H5P_DEFAULT);
    herr_t status = H5Dwrite(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, trace);

    H5Dclose(dset);
    H5Pclose(properties);
    H5Sclose(space);
    return status;
}


char * read_string_attribute(hid_t group, const char * attribute){
    char * str = NULL;

    if (group < 0){
        warnx("Invalid group passed to %s:%d.", __FILE__, __LINE__);
        return NULL;
    }
    hid_t attr = H5Aopen(group, attribute, H5P_DEFAULT);
    if (attr < 0) {
        warnx("Failed to open attribute '%s' for reading.", attribute);
        return NULL;
    }


    hid_t atype = H5Aget_type(attr);
    if(atype < 0){
        warnx("Failed to get type of attribute '%s'.", attribute);
        goto cleanup1;
    }
    if(H5T_STRING != H5Tget_class (atype)){
        warnx("Attribute '%s' is not a string.", attribute);
        goto cleanup2;
    }

    if(H5Tis_variable_str(atype) > 0){
        hid_t space = H5Aget_space (attr);
        hsize_t len = 0;
        H5Sget_simple_extent_dims (space, &len, NULL);

        str = calloc(len, sizeof(char));

        hid_t memtype = H5Tcopy (H5T_C_S1);
        H5Tset_size (memtype, H5T_VARIABLE);
        herr_t err = H5Aread(attr, atype, &str);
        if(err < 0){
            warnx("Failed to copy attribute '%s'.", attribute);
            free(str);
            str = NULL;
        }

        H5Tclose(memtype);
        H5Sclose(space);
    } else {
        // Fixed length
        size_t asize = H5Tget_size(atype);
        str = calloc(asize + 1, sizeof(char));
        herr_t err = H5Aread(attr, atype, str);
        if(err < 0){
            warnx("Failed to copy attribute '%s'.", attribute);
            free(str);
            str = NULL;
        }
    }

cleanup2:
    H5Tclose(atype);
cleanup1:
    H5Aclose(attr);

    return str;
}


fast5_raw_scaling get_raw_scaling(hid_t hdf5file) {
    // Add 1e-5 to sensible sample rate as a sentinel value
    fast5_raw_scaling scaling = { NAN, NAN, NAN, NAN };
    const char *scaling_path = "/UniqueGlobalKey/channel_id";

    hid_t scaling_group = H5Gopen(hdf5file, scaling_path, H5P_DEFAULT);
    if (scaling_group < 0) {
        warnx("Failed to group %s.", scaling_path);
        return scaling;
    }

    scaling.digitisation = read_float_attribute(scaling_group, "digitisation");
    scaling.offset = read_float_attribute(scaling_group, "offset");
    scaling.range = read_float_attribute(scaling_group, "range");
    scaling.sample_rate = read_float_attribute(scaling_group, "sampling_rate");

    H5Gclose(scaling_group);

    return scaling;
}


raw_table read_raw(const char *filename, bool scale_to_pA) {
    herr_t status;
    float *rawptr;
    hid_t space, dset, ugroup, hdf5file;
    char *root, *name, *dset_path, *uuid , *signal_path;
    assert(NULL != filename);
    raw_table rawtbl = { NULL, 0, 0, 0, NULL };

    hdf5file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (hdf5file < 0) {
        warnx("Failed to open %s for reading.", filename);
        return rawtbl;
    }
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    root = "/Raw/Reads/";
    const int rootstr_len = strlen(root);
    ssize_t size =
        H5Lget_name_by_idx(hdf5file, root, H5_INDEX_NAME, H5_ITER_INC, 0, NULL,
                           0, H5P_DEFAULT);
    if (size < 0) {
        warnx("Failed find read name under %s.", root);
        goto cleanup1;
    }
    name = calloc(1 + size, sizeof(char));
    H5Lget_name_by_idx(hdf5file, root, H5_INDEX_NAME, H5_ITER_INC, 0, name,
                       1 + size, H5P_DEFAULT);

    // uuid pat
    dset_path = calloc(rootstr_len + size + 1, sizeof(char));
    (void)snprintf(dset_path, rootstr_len + size + 1, "%s%s", root, name);
    ugroup = H5Gopen(hdf5file, dset_path, H5P_DEFAULT);
    if(ugroup < 0){
        warnx("Failed to find read_id under %s.", dset_path);
        goto cleanup1_1;
    }
    uuid = read_string_attribute(ugroup, "read_id");
    H5Gclose(ugroup);

    // Create group name
    signal_path = calloc(rootstr_len + size + 8, sizeof(char));
    (void)snprintf(signal_path, rootstr_len + size + 8, "%s%s/Signal", root, name);

    dset = H5Dopen(hdf5file, signal_path, H5P_DEFAULT);
    if (dset < 0) {
        warnx("Failed to open dataset '%s' to read raw signal from.",
              signal_path);
        goto cleanup2;
    }

    space = H5Dget_space(dset);
    if (space < 0) {
        warnx("Failed to create copy of dataspace for raw signal %s.",
              signal_path);
        goto cleanup3;
    }
    hsize_t nsample;
    H5Sget_simple_extent_dims(space, &nsample, NULL);
    rawptr = calloc(nsample, sizeof(float));
    status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawptr);
    if (status < 0) {
        free(rawptr);
        free(uuid);
        warnx("Failed to read raw data from dataset %s.", signal_path);
        goto cleanup4;
    }
    rawtbl = (raw_table) {
    uuid, nsample, 0, nsample, rawptr};

    if (scale_to_pA) {
        const fast5_raw_scaling scaling = get_raw_scaling(hdf5file);
        const float raw_unit = scaling.range / scaling.digitisation;
        for (size_t i = 0; i < nsample; i++) {
            rawptr[i] = (rawptr[i] + scaling.offset) * raw_unit;
        }
    }

 cleanup4:
    H5Sclose(space);
 cleanup3:
    H5Dclose(dset);
 cleanup2:
    free(signal_path);
 cleanup1_1:
    free(dset_path);
    free(name);
 cleanup1:
    H5Fclose(hdf5file);

    return rawtbl;
}


void write_summary(hid_t hdf5file, const char *readname,
                   const struct _raw_basecall_info res,
                   hsize_t chunk_size, int compression_level){
    assert(compression_level >= 0 && compression_level <= 9);
    if(hdf5file < 0){
        return;
    }

    hid_t read_group = create_group(hdf5file, readname);
    if(read_group < 0){
        warnx("Failed to create group \"%s\" %s:%d.", readname, __FILE__, __LINE__);
    }

    const size_t nsample = res.rt.end - res.rt.start;

    write_signal(read_group, res.rt.raw + res.rt.start,
                                 nsample, chunk_size, compression_level);

    int32_t * trace_flat = array_from_flappie_imatrix(res.trace);
    if(NULL != trace_flat){
        write_trace(read_group, trace_flat, res.trace->nr, res.trace->nc,
                                     chunk_size, compression_level);
        free(trace_flat);
    }

    H5Gclose(read_group);

    return;
}
