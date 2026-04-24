/*
 * Small test helper: compare two images within a per-channel absolute tolerance.
 *
 * Usage: imgdiff <reference> <candidate> [max_abs_diff]
 *   max_abs_diff defaults to 4 (0..255). Exits 0 when max(|ref - cand|) across all
 *   RGBA channels and pixels is <= tolerance, non-zero otherwise.
 *
 * Decodes via stb_image (header-only). Both images are forced to 4-channel RGBA.
 * Sizes must match exactly (no resampling).
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(FILE *out) {
    fprintf(out, "Usage: imgdiff [--ignore-alpha] <reference.png> <candidate.png> [max_abs_diff [min_psnr_db]]\n");
}

int main(int argc, char **argv) {
    int ignore_alpha = 0;
    int arg_start = 1;
    if(argc > 1 && strcmp(argv[1], "--ignore-alpha") == 0) {
        ignore_alpha = 1;
        arg_start = 2;
    }
    if(argc - arg_start < 2) {
        print_usage(stderr);
        return 2;
    }
    int tol = 4;
    double min_psnr = -1.0; /* disabled by default */
    if(argc - arg_start >= 3) {
        tol = atoi(argv[arg_start + 2]);
        if(tol < 0) tol = 0;
        if(tol > 255) tol = 255;
    }
    if(argc - arg_start >= 4) {
        min_psnr = atof(argv[arg_start + 3]);
    }
    const char *ref_path = argv[arg_start];
    const char *cand_path = argv[arg_start + 1];

    int rw = 0, rh = 0, rc = 0;
    stbi_uc *ref = stbi_load(ref_path, &rw, &rh, &rc, 4);
    if(!ref) {
        fprintf(stderr, "imgdiff: failed to load reference '%s': %s\n", ref_path, stbi_failure_reason());
        return 2;
    }
    int cw = 0, ch_ = 0, cc = 0;
    stbi_uc *cand = stbi_load(cand_path, &cw, &ch_, &cc, 4);
    if(!cand) {
        fprintf(stderr, "imgdiff: failed to load candidate '%s': %s\n", cand_path, stbi_failure_reason());
        stbi_image_free(ref);
        return 2;
    }

    if(rw != cw || rh != ch_) {
        fprintf(stderr, "imgdiff: dimensions differ (ref %dx%d vs cand %dx%d)\n", rw, rh, cw, ch_);
        stbi_image_free(ref);
        stbi_image_free(cand);
        return 3;
    }

    const size_t pixels = (size_t)rw * (size_t)rh;
    int max_diff = 0;
    long long sum_sq = 0;
    size_t samples = 0;
    const int last_channel = ignore_alpha ? 3 : 4; /* 0..3 = RGB only, 0..4 = RGBA */
    for(size_t p = 0; p < pixels; ++p) {
        const size_t base = p * 4;
        const int ar = ref[base + 3];
        const int ac = cand[base + 3];
        /* Fully transparent in both — RGB is visually undefined, skip those channels. */
        if(!ignore_alpha && ar == 0 && ac == 0) {
            continue;
        }
        for(int c = 0; c < last_channel; ++c) {
            int d = (int)ref[base + c] - (int)cand[base + c];
            int ad = d < 0 ? -d : d;
            if(ad > max_diff) max_diff = ad;
            sum_sq += (long long)d * (long long)d;
            ++samples;
        }
    }

    const double mse = samples > 0 ? (double)sum_sq / (double)samples : 0.0;
    double psnr = 99.0;
    if(mse > 0.0) {
        psnr = 10.0 * log10((255.0 * 255.0) / mse);
    }

    fprintf(stdout, "imgdiff: %dx%d  max_abs_diff=%d  mse=%.3f  psnr=%.2fdB  tol=%d%s%.1f\n",
            rw, rh, max_diff, mse, psnr, tol,
            min_psnr > 0.0 ? "  min_psnr=" : "",
            min_psnr > 0.0 ? min_psnr : 0.0);

    stbi_image_free(ref);
    stbi_image_free(cand);

    if(max_diff > tol) {
        fprintf(stderr, "imgdiff: FAIL — max_abs_diff %d exceeds tolerance %d\n", max_diff, tol);
        return 1;
    }
    if(min_psnr > 0.0 && psnr < min_psnr) {
        fprintf(stderr, "imgdiff: FAIL — PSNR %.2fdB below threshold %.2fdB\n", psnr, min_psnr);
        return 1;
    }
    return 0;
}
