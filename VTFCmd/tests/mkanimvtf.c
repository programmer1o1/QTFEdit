/*
 * Small test helper: build a multi-image VTF from N input PNGs.
 *
 * Usage: mkanimvtf [--cube] <output.vtf> <img1.png> [img2.png ...]
 *
 * Every input must have identical dimensions. Default layout: N frames, 1 face, 1 slice.
 * With --cube: 1 frame, N faces, 1 slice (N must be 6 for a valid cubemap).
 * Always RGBA8888 with mipmaps.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <VTFLib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int arg_start = 1;
    int cube_mode = 0;
    if(argc > 1 && strcmp(argv[1], "--cube") == 0) {
        cube_mode = 1;
        arg_start = 2;
    }
    if(argc - arg_start < 2) {
        fprintf(stderr, "Usage: mkanimvtf [--cube] <output.vtf> <img1.png> [img2.png ...]\n");
        return 2;
    }

    const char *out_path = argv[arg_start];
    const int frame_count = argc - arg_start - 1;
    if(frame_count < 1) return 2;
    if(cube_mode && frame_count != 6) {
        fprintf(stderr, "mkanimvtf --cube requires exactly 6 images (got %d)\n", frame_count);
        return 2;
    }
    const int img_arg_start = arg_start + 1;

    int w = 0, h = 0, channels = 0;
    unsigned char **frames = (unsigned char **)calloc(frame_count, sizeof(*frames));
    if(!frames) { fprintf(stderr, "mkanimvtf: oom\n"); return 1; }

    for(int i = 0; i < frame_count; ++i) {
        int fw = 0, fh = 0, fc = 0;
        unsigned char *px = stbi_load(argv[img_arg_start + i], &fw, &fh, &fc, 4);
        if(!px) {
            fprintf(stderr, "mkanimvtf: failed to load '%s': %s\n", argv[img_arg_start + i], stbi_failure_reason());
            for(int j = 0; j < i; ++j) stbi_image_free(frames[j]);
            free(frames);
            return 1;
        }
        if(i == 0) { w = fw; h = fh; channels = fc; }
        else if(fw != w || fh != h) {
            fprintf(stderr, "mkanimvtf: frame %d has %dx%d, expected %dx%d\n", i, fw, fh, w, h);
            stbi_image_free(px);
            for(int j = 0; j < i; ++j) stbi_image_free(frames[j]);
            free(frames);
            return 3;
        }
        frames[i] = px;
        (void)channels;
    }

    if(!vlInitialize()) {
        fprintf(stderr, "mkanimvtf: vlInitialize failed: %s\n", vlGetLastError());
        for(int i = 0; i < frame_count; ++i) stbi_image_free(frames[i]);
        free(frames);
        return 1;
    }

    vlUInt image_id = 0;
    if(!vlCreateImage(&image_id) || !vlBindImage(image_id)) {
        fprintf(stderr, "mkanimvtf: vlCreateImage/vlBindImage failed: %s\n", vlGetLastError());
        vlShutdown();
        for(int i = 0; i < frame_count; ++i) stbi_image_free(frames[i]);
        free(frames);
        return 1;
    }

    SVTFCreateOptions opts;
    vlImageCreateDefaultCreateStructure(&opts);
    opts.ImageFormat = IMAGE_FORMAT_RGBA8888;
    opts.bMipmaps = vlTrue;
    if(cube_mode) {
        opts.uiFlags |= TEXTUREFLAGS_ENVMAP;
    }

    const vlUInt vFrames = cube_mode ? 1u : (vlUInt)frame_count;
    const vlUInt vFaces  = cube_mode ? 6u : 1u;
    const vlUInt vSlices = 1u;

    if(!vlImageCreateMultiple((vlUInt)w, (vlUInt)h,
                              vFrames, vFaces, vSlices,
                              (vlByte **)frames, &opts)) {
        fprintf(stderr, "mkanimvtf: vlImageCreateMultiple failed: %s\n", vlGetLastError());
        vlDeleteImage(image_id);
        vlShutdown();
        for(int i = 0; i < frame_count; ++i) stbi_image_free(frames[i]);
        free(frames);
        return 1;
    }

    if(!vlImageSave(out_path)) {
        fprintf(stderr, "mkanimvtf: vlImageSave('%s') failed: %s\n", out_path, vlGetLastError());
        vlDeleteImage(image_id);
        vlShutdown();
        for(int i = 0; i < frame_count; ++i) stbi_image_free(frames[i]);
        free(frames);
        return 1;
    }

    fprintf(stdout, "mkanimvtf: wrote %s (%dx%d, %s %d)\n",
            out_path, w, h, cube_mode ? "faces" : "frames", frame_count);

    vlDeleteImage(image_id);
    vlShutdown();
    for(int i = 0; i < frame_count; ++i) stbi_image_free(frames[i]);
    free(frames);
    return 0;
}
