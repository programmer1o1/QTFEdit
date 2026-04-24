/*
 * Thin C++ wrapper for tinyexr, exposing C-linkage functions
 * so that image_io.c (compiled as C) can call them.
 */

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_IMPLEMENTATION
#include <zlib.h>
#include "tinyexr.h"

extern "C" {

int vtfcmd_SaveEXR(const float *data, int width, int height, int components, int save_as_fp16, const char *outfilename, const char **err)
{
	return SaveEXR(data, width, height, components, save_as_fp16, outfilename, err);
}

int vtfcmd_LoadEXR(float **out_rgba, int *width, int *height, const char *filename, const char **err)
{
	return LoadEXR(out_rgba, width, height, filename, err);
}

void vtfcmd_FreeEXRErrorMessage(const char *msg)
{
	FreeEXRErrorMessage(msg);
}

} // extern "C"
