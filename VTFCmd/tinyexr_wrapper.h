/*
 * C-linkage wrapper for tinyexr functions.
 */

#ifndef TINYEXR_WRAPPER_H
#define TINYEXR_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#define VTFCMD_TINYEXR_SUCCESS 0

int vtfcmd_SaveEXR(const float *data, int width, int height, int components, int save_as_fp16, const char *outfilename, const char **err);
int vtfcmd_LoadEXR(float **out_rgba, int *width, int *height, const char *filename, const char **err);
void vtfcmd_FreeEXRErrorMessage(const char *msg);

#ifdef __cplusplus
}
#endif

#endif
