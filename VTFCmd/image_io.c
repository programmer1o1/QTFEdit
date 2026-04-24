/*
 * VTFCmd
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "image_io.h"

#ifndef VTFCMD_USE_DEVIL
#define VTFCMD_USE_DEVIL 1
#endif

// QOI and EXR are always available (header-only, no external dependency).
#define QOI_IMPLEMENTATION
#include "qoi.h"

#include "tinyexr_wrapper.h"

#ifndef VTFCMD_HAS_WEBP
#define VTFCMD_HAS_WEBP 0
#endif

#if VTFCMD_HAS_WEBP
#include <webp/encode.h>
#include <webp/decode.h>
#endif

// Shared helpers used by both backends.
static vlBool ext_equals_shared(const char *lpExt, const char *lpWant)
{
	while(*lpExt == '.')
		lpExt++;

	while(*lpExt && *lpWant)
	{
		char a = (char)tolower((unsigned char)*lpExt++);
		char b = (char)tolower((unsigned char)*lpWant++);
		if(a != b)
			return vlFalse;
	}

	return *lpExt == '\0' && *lpWant == '\0';
}

static const char *find_extension_shared(const char *lpPath)
{
	const char *dot = strrchr(lpPath, '.');
	if(dot == NULL || dot[1] == '\0')
		return "";
	return dot + 1;
}

static vlBool vtfcmdWriteQOI(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	qoi_desc desc;
	desc.width = uiWidth;
	desc.height = uiHeight;
	desc.channels = uiChannels;
	desc.colorspace = QOI_SRGB;

	int written = qoi_write(lpPath, lpData, &desc);
	if(!written)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "qoi_write() failed.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

static vlBool vtfcmdWriteEXR(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	if(uiChannels < 3)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, ".exr export requires at least 3 channels.");
		return vlFalse;
	}

	// Convert byte data to float RGBA for tinyexr.
	vlUInt numChannels = uiChannels >= 4 ? 4 : 3;
	float *pixelsf = (float *)malloc((size_t)uiWidth * (size_t)uiHeight * numChannels * sizeof(float));
	if(pixelsf == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	for(size_t i = 0; i < (size_t)uiWidth * (size_t)uiHeight; i++)
	{
		for(vlUInt c = 0; c < numChannels; c++)
		{
			pixelsf[i * numChannels + c] = lpData[i * uiChannels + c] / 255.0f;
		}
	}

	const char *exrErr = NULL;
	int ret = vtfcmd_SaveEXR(pixelsf, (int)uiWidth, (int)uiHeight, (int)numChannels, 0, lpPath, &exrErr);
	free(pixelsf);

	if(ret != VTFCMD_TINYEXR_SUCCESS)
	{
		if(lpError && uiErrorSize > 0)
		{
			if(exrErr)
				snprintf(lpError, uiErrorSize, "tinyexr: %s", exrErr);
			else
				snprintf(lpError, uiErrorSize, "tinyexr failed with code %d.", ret);
		}
		vtfcmd_FreeEXRErrorMessage(exrErr);
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

static vlBool vtfcmdLoadQOI(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	qoi_desc desc;
	void *pixels = qoi_read(lpPath, &desc, 4);
	if(pixels == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "qoi_read() failed.");
		return vlFalse;
	}

	pOut->uiWidth = desc.width;
	pOut->uiHeight = desc.height;
	pOut->uiChannelsInFile = desc.channels;
	if(pOut->uiChannelsInFile < 1) pOut->uiChannelsInFile = 1;
	if(pOut->uiChannelsInFile > 4) pOut->uiChannelsInFile = 4;
	pOut->lpRGBA = (vlByte *)pixels;

	if(lpError) *lpError = '\0';
	return vlTrue;
}

static vlBool vtfcmdvtfcmd_LoadEXR(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	float *pixelsf = NULL;
	int w = 0, h = 0;
	const char *exrErr = NULL;

	int ret = vtfcmd_LoadEXR(&pixelsf, &w, &h, lpPath, &exrErr);
	if(ret != VTFCMD_TINYEXR_SUCCESS)
	{
		if(lpError && uiErrorSize > 0)
		{
			if(exrErr)
				snprintf(lpError, uiErrorSize, "tinyexr: %s", exrErr);
			else
				snprintf(lpError, uiErrorSize, "tinyexr failed with code %d.", ret);
		}
		vtfcmd_FreeEXRErrorMessage(exrErr);
		return vlFalse;
	}

	// Convert float RGBA to byte RGBA.
	vlByte *pixels8 = (vlByte *)malloc((size_t)w * (size_t)h * 4);
	if(pixels8 == NULL)
	{
		free(pixelsf);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	for(size_t i = 0; i < (size_t)w * (size_t)h * 4; i++)
	{
		float v = pixelsf[i];
		if(v < 0.0f) v = 0.0f;
		if(v > 1.0f) v = 1.0f;
		pixels8[i] = (vlByte)(v * 255.0f + 0.5f);
	}

	free(pixelsf);

	pOut->uiWidth = (vlUInt)w;
	pOut->uiHeight = (vlUInt)h;
	pOut->uiChannelsInFile = 4;
	pOut->lpRGBA = pixels8;

	if(lpError) *lpError = '\0';
	return vlTrue;
}

#if VTFCMD_HAS_WEBP
static vlBool vtfcmdWriteWebP(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	uint8_t *output = NULL;
	size_t outputSize = 0;

	if(uiChannels == 4)
		outputSize = WebPEncodeLosslessRGBA(lpData, (int)uiWidth, (int)uiHeight, (int)(uiWidth * 4), &output);
	else
		outputSize = WebPEncodeLosslessRGB(lpData, (int)uiWidth, (int)uiHeight, (int)(uiWidth * 3), &output);

	if(outputSize == 0 || output == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "WebP encoding failed.");
		WebPFree(output);
		return vlFalse;
	}

	FILE *f = fopen(lpPath, "wb");
	if(f == NULL)
	{
		WebPFree(output);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Failed to open '%s' for writing.", lpPath);
		return vlFalse;
	}

	size_t written = fwrite(output, 1, outputSize, f);
	fclose(f);
	WebPFree(output);

	if(written != outputSize)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Failed to write WebP data.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

static vlBool vtfcmdLoadWebP(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	FILE *f = fopen(lpPath, "rb");
	if(f == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Failed to open '%s' for reading.", lpPath);
		return vlFalse;
	}

	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if(fileSize <= 0)
	{
		fclose(f);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Empty or invalid WebP file.");
		return vlFalse;
	}

	uint8_t *fileData = (uint8_t *)malloc((size_t)fileSize);
	if(fileData == NULL)
	{
		fclose(f);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	fread(fileData, 1, (size_t)fileSize, f);
	fclose(f);

	int w = 0, h = 0;
	uint8_t *pixels = WebPDecodeRGBA(fileData, (size_t)fileSize, &w, &h);
	free(fileData);

	if(pixels == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "WebP decoding failed.");
		return vlFalse;
	}

	// Copy to our own buffer since WebP uses its own allocator.
	vlByte *lpRGBA = (vlByte *)malloc((size_t)w * (size_t)h * 4);
	if(lpRGBA == NULL)
	{
		WebPFree(pixels);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	memcpy(lpRGBA, pixels, (size_t)w * (size_t)h * 4);
	WebPFree(pixels);

	pOut->uiWidth = (vlUInt)w;
	pOut->uiHeight = (vlUInt)h;
	pOut->uiChannelsInFile = 4;
	pOut->lpRGBA = lpRGBA;

	if(lpError) *lpError = '\0';
	return vlTrue;
}
#endif

#if VTFCMD_USE_DEVIL

static ILuint g_DevILImage = 0;

static void flip_vertical_copy(vlByte *lpDest, const vlByte *lpSrc, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels)
{
	const vlUInt uiRowSize = uiWidth * uiChannels;
	for(vlUInt y = 0; y < uiHeight; y++)
	{
		memcpy(lpDest + y * uiRowSize, lpSrc + (uiHeight - 1 - y) * uiRowSize, uiRowSize);
	}
}

vlBool vtfcmdImageIOInit(vlChar *lpError, vlUInt uiErrorSize)
{
	(void)uiErrorSize;

	ilInit();

	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

	ilGenImages(1, &g_DevILImage);
	ilBindImage(g_DevILImage);

	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdImageIOShutdown()
{
	if(g_DevILImage != 0)
	{
		ilDeleteImages(1, &g_DevILImage);
		g_DevILImage = 0;
	}

	ilShutDown();
}

vlBool vtfcmdLoadImageRGBA(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	if(pOut == NULL)
		return vlFalse;

	memset(pOut, 0, sizeof(*pOut));

	// Handle formats not supported by DevIL natively.
	const char *ext = find_extension_shared(lpPath);
	if(ext_equals_shared(ext, "qoi"))
		return vtfcmdLoadQOI(lpPath, pOut, lpError, uiErrorSize);
	if(ext_equals_shared(ext, "exr"))
		return vtfcmdvtfcmd_LoadEXR(lpPath, pOut, lpError, uiErrorSize);
#if VTFCMD_HAS_WEBP
	if(ext_equals_shared(ext, "webp"))
		return vtfcmdLoadWebP(lpPath, pOut, lpError, uiErrorSize);
#endif

	if(!ilLoadImage(lpPath))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to load image.");
		return vlFalse;
	}

	pOut->uiWidth = (vlUInt)ilGetInteger(IL_IMAGE_WIDTH);
	pOut->uiHeight = (vlUInt)ilGetInteger(IL_IMAGE_HEIGHT);
	pOut->uiChannelsInFile = (vlUInt)ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	if(pOut->uiChannelsInFile < 1) pOut->uiChannelsInFile = 1;
	if(pOut->uiChannelsInFile > 4) pOut->uiChannelsInFile = 4;

	if(!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to convert image to RGBA8.");
		return vlFalse;
	}

	const vlUInt uiSize = pOut->uiWidth * pOut->uiHeight * 4;
	pOut->lpRGBA = (vlByte *)malloc(uiSize);
	if(pOut->lpRGBA == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	memcpy(pOut->lpRGBA, ilGetData(), uiSize);
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdFreeLoadedImage(VTFCmdLoadedImage *pImage)
{
	if(pImage == NULL)
		return;

	free(pImage->lpRGBA);
	memset(pImage, 0, sizeof(*pImage));
}

vlBool vtfcmdWriteImage(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	if(uiChannels != 3 && uiChannels != 4)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported channel count %u.", uiChannels);
		return vlFalse;
	}

	// Handle formats not supported by DevIL natively.
	const char *ext = find_extension_shared(lpPath);
	if(ext_equals_shared(ext, "qoi"))
		return vtfcmdWriteQOI(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
	if(ext_equals_shared(ext, "exr"))
		return vtfcmdWriteEXR(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
#if VTFCMD_HAS_WEBP
	if(ext_equals_shared(ext, "webp"))
		return vtfcmdWriteWebP(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
#endif

	const vlUInt uiSize = uiWidth * uiHeight * uiChannels;
	vlByte *lpFlipped = (vlByte *)malloc(uiSize);
	if(lpFlipped == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	flip_vertical_copy(lpFlipped, lpData, uiWidth, uiHeight, uiChannels);

	if(!ilTexImage((ILuint)uiWidth, (ILuint)uiHeight, 1, (ILubyte)uiChannels,
	               uiChannels == 4 ? IL_RGBA : IL_RGB, IL_UNSIGNED_BYTE, lpFlipped))
	{
		free(lpFlipped);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to create image for output.");
		return vlFalse;
	}

	free(lpFlipped);

	if(!ilSaveImage(lpPath))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to save image.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

#else

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

// Reuse the shared helpers.
#define ext_equals ext_equals_shared
#define find_extension find_extension_shared

vlBool vtfcmdImageIOInit(vlChar *lpError, vlUInt uiErrorSize)
{
	(void)uiErrorSize;
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdImageIOShutdown()
{
}

vlBool vtfcmdLoadImageRGBA(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	if(pOut == NULL)
		return vlFalse;

	memset(pOut, 0, sizeof(*pOut));

	// Handle formats not covered by stb_image.
	const char *ext = find_extension(lpPath);
	if(ext_equals(ext, "qoi"))
		return vtfcmdLoadQOI(lpPath, pOut, lpError, uiErrorSize);
	if(ext_equals(ext, "exr"))
		return vtfcmdvtfcmd_LoadEXR(lpPath, pOut, lpError, uiErrorSize);
#if VTFCMD_HAS_WEBP
	if(ext_equals(ext, "webp"))
		return vtfcmdLoadWebP(lpPath, pOut, lpError, uiErrorSize);
#endif

	int w = 0, h = 0, c = 0;
	unsigned char *pixels8 = NULL;

	if(stbi_is_hdr(lpPath))
	{
		float *pixelsf = stbi_loadf(lpPath, &w, &h, &c, 4);
		if(pixelsf == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image failed to load HDR: %s", stbi_failure_reason());
			return vlFalse;
		}

		pixels8 = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
		if(pixels8 == NULL)
		{
			stbi_image_free(pixelsf);
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
			return vlFalse;
		}

		for(size_t i = 0; i < (size_t)w * (size_t)h * 4; i++)
		{
			float v = pixelsf[i];
			if(v < 0.0f) v = 0.0f;
			if(v > 1.0f) v = 1.0f;
			pixels8[i] = (unsigned char)(v * 255.0f + 0.5f);
		}

		stbi_image_free(pixelsf);
	}
	else
	{
		pixels8 = stbi_load(lpPath, &w, &h, &c, 4);
		if(pixels8 == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image failed to load image: %s", stbi_failure_reason());
			return vlFalse;
		}
	}

	pOut->uiWidth = (vlUInt)w;
	pOut->uiHeight = (vlUInt)h;
	pOut->uiChannelsInFile = (vlUInt)c;
	if(pOut->uiChannelsInFile < 1) pOut->uiChannelsInFile = 1;
	if(pOut->uiChannelsInFile > 4) pOut->uiChannelsInFile = 4;

	pOut->lpRGBA = (vlByte *)pixels8;
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdFreeLoadedImage(VTFCmdLoadedImage *pImage)
{
	if(pImage == NULL)
		return;

	if(pImage->lpRGBA != NULL)
		stbi_image_free(pImage->lpRGBA);

	memset(pImage, 0, sizeof(*pImage));
}

vlBool vtfcmdWriteImage(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	if(uiChannels != 3 && uiChannels != 4)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported channel count %u.", uiChannels);
		return vlFalse;
	}

	const char *ext = find_extension(lpPath);
	int ok = 0;

	if(ext_equals(ext, "png"))
		ok = stbi_write_png(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData, (int)(uiWidth * uiChannels));
	else if(ext_equals(ext, "tga"))
		ok = stbi_write_tga(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData);
	else if(ext_equals(ext, "bmp"))
		ok = stbi_write_bmp(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData);
	else if(ext_equals(ext, "jpg") || ext_equals(ext, "jpeg"))
		ok = stbi_write_jpg(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData, 90);
	else if(ext_equals(ext, "hdr"))
	{
		if(uiChannels < 3)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, ".hdr export requires at least 3 channels.");
			return vlFalse;
		}

		float *pixelsf = (float *)malloc((size_t)uiWidth * (size_t)uiHeight * 3 * sizeof(float));
		if(pixelsf == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
			return vlFalse;
		}

		for(size_t i = 0; i < (size_t)uiWidth * (size_t)uiHeight; i++)
		{
			pixelsf[i * 3 + 0] = lpData[i * uiChannels + 0] / 255.0f;
			pixelsf[i * 3 + 1] = lpData[i * uiChannels + 1] / 255.0f;
			pixelsf[i * 3 + 2] = lpData[i * uiChannels + 2] / 255.0f;
		}

		ok = stbi_write_hdr(lpPath, (int)uiWidth, (int)uiHeight, 3, pixelsf);
		free(pixelsf);
	}
	else if(ext_equals(ext, "qoi"))
	{
		return vtfcmdWriteQOI(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
	}
	else if(ext_equals(ext, "exr"))
	{
		return vtfcmdWriteEXR(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
	}
#if VTFCMD_HAS_WEBP
	else if(ext_equals(ext, "webp"))
	{
		return vtfcmdWriteWebP(lpPath, lpData, uiWidth, uiHeight, uiChannels, lpError, uiErrorSize);
	}
#endif
	else
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported export format '%s' (use png, tga, bmp, jpg, hdr, qoi, exr"
#if VTFCMD_HAS_WEBP
			", webp"
#endif
			").", ext);
		return vlFalse;
	}

	if(!ok)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image_write failed to save image.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

#endif
