/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
 */

#include "YUVConversion.h"

using namespace YUV_Internals;

// Restrict is basically a promise to the compiler that for the scope of the pointer, the target of
// the pointer will only be accessed through that pointer (and pointers copied from it).
#if __STDC__ != 1
#define restrict __restrict /* use implementation __ format */
#else
#ifndef __STDC_VERSION__
#define restrict __restrict /* use implementation __ format */
#else
#if __STDC_VERSION__ < 199901L
#define restrict __restrict /* use implementation __ format */
#else
#/* all ok */
#endif
#endif
#endif

inline int getValueFromSource(const unsigned char *restrict src,
                              const int                     idx,
                              const int                     bps,
                              const bool                    bigEndian)
{
  if (bps > 8)
    // Read two bytes in the right order
    return (bigEndian) ? src[idx * 2] << 8 | src[idx * 2 + 1]
                       : src[idx * 2] | src[idx * 2 + 1] << 8;
  else
    // Just read one byte
    return src[idx];
}

inline void convertYUVToRGB8Bit(const unsigned int valY,
                                const unsigned int valU,
                                const unsigned int valV,
                                int &              valR,
                                int &              valG,
                                int &              valB,
                                const int          RGBConv[5],
                                const bool         fullRange,
                                const int          bps)
{
  if (bps > 14)
  {
    // The bit depth of an int (32) is not enough to perform a YUV -> RGB conversion for a bit depth
    // > 14 bits. We could use 64 bit values but for what? We are clipping the result to 8 bit
    // anyways so let's just get rid of 2 of the bits for the YUV values.
    const int yOffset = (fullRange ? 0 : 16 << (bps - 10));
    const int cZero   = 128 << (bps - 10);

    const int Y_tmp = ((valY >> 2) - yOffset) * RGBConv[0];
    const int U_tmp = (valU >> 2) - cZero;
    const int V_tmp = (valV >> 2) - cZero;

    const int R_tmp = (Y_tmp + V_tmp * RGBConv[1]) >>
                      (16 + bps - 10); // 32 to 16 bit conversion by right shifting
    const int G_tmp = (Y_tmp + U_tmp * RGBConv[2] + V_tmp * RGBConv[3]) >> (16 + bps - 10);
    const int B_tmp = (Y_tmp + U_tmp * RGBConv[4]) >> (16 + bps - 10);

    valR = (R_tmp < 0) ? 0 : (R_tmp > 255) ? 255 : R_tmp;
    valG = (G_tmp < 0) ? 0 : (G_tmp > 255) ? 255 : G_tmp;
    valB = (B_tmp < 0) ? 0 : (B_tmp > 255) ? 255 : B_tmp;
  }
  else
  {
    const int yOffset = (fullRange ? 0 : 16 << (bps - 8));
    const int cZero   = 128 << (bps - 8);

    const int Y_tmp = (valY - yOffset) * RGBConv[0];
    const int U_tmp = valU - cZero;
    const int V_tmp = valV - cZero;

    const int R_tmp =
        (Y_tmp + V_tmp * RGBConv[1]) >> (16 + bps - 8); // 32 to 16 bit conversion by right shifting
    const int G_tmp = (Y_tmp + U_tmp * RGBConv[2] + V_tmp * RGBConv[3]) >> (16 + bps - 8);
    const int B_tmp = (Y_tmp + U_tmp * RGBConv[4]) >> (16 + bps - 8);

    valR = (R_tmp < 0) ? 0 : (R_tmp > 255) ? 255 : R_tmp;
    valG = (G_tmp < 0) ? 0 : (G_tmp > 255) ? 255 : G_tmp;
    valB = (B_tmp < 0) ? 0 : (B_tmp > 255) ? 255 : B_tmp;
  }
}

inline void YUVPlaneToRGB_444(const int                     componentSize,
                              const unsigned char *restrict srcY,
                              const unsigned char *restrict srcU,
                              const unsigned char *restrict srcV,
                              unsigned char *restrict       dst,
                              const int                     RGBConv[5],
                              const bool                    fullRange,
                              const int                     bps,
                              const bool                    bigEndian,
                              const int                     inValSkip)
{
  for (int i = 0; i < componentSize; ++i)
  {
    unsigned int valY = getValueFromSource(srcY, i, bps, bigEndian);
    unsigned int valU = getValueFromSource(srcU, i * inValSkip, bps, bigEndian);
    unsigned int valV = getValueFromSource(srcV, i * inValSkip, bps, bigEndian);

    // Get the RGB values for this sample
    int valR, valG, valB;
    convertYUVToRGB8Bit(valY, valU, valV, valR, valG, valB, RGBConv, fullRange, bps);

    // Save the RGB values
    dst[i * 4]     = valB;
    dst[i * 4 + 1] = valG;
    dst[i * 4 + 2] = valR;
    dst[i * 4 + 3] = 255;
  }
}

inline int interpolateUVSample(const ChromaInterpolation mode, const int sample1, const int sample2)
{
  if (mode == ChromaInterpolation::Bilinear)
    // Interpolate linearly between sample1 and sample2
    return ((sample1 + sample2) + 1) >> 1;
  return sample1; // Sample and hold
}

// TODO: Consider sample position
inline int interpolateUVSample2D(const ChromaInterpolation mode,
                                 const int                 sample1,
                                 const int                 sample2,
                                 const int                 sample3,
                                 const int                 sample4)
{
  if (mode == ChromaInterpolation::Bilinear)
    // Interpolate linearly between sample1 - sample 4
    return ((sample1 + sample2 + sample3 + sample4) + 2) >> 2;
  return sample1; // Sample and hold
}

inline void YUVPlaneToRGB_422(const int                     w,
                              const int                     h,
                              const unsigned char *restrict srcY,
                              const unsigned char *restrict srcU,
                              const unsigned char *restrict srcV,
                              unsigned char *restrict       dst,
                              const int                     RGBConv[5],
                              const bool                    fullRange,
                              const ChromaInterpolation     interpolation,
                              const int                     bps,
                              const bool                    bigEndian,
                              const int                     inValSkip)
{
  // Horizontal up-sampling is required. Process two Y values at a time
  for (int y = 0; y < h; y++)
  {
    const int srcIdxUV   = y * w / 2;
    int       curUSample = getValueFromSource(srcU, srcIdxUV * inValSkip, bps, bigEndian);
    int       curVSample = getValueFromSource(srcV, srcIdxUV * inValSkip, bps, bigEndian);

    for (int x = 0; x < (w / 2) - 1; x++)
    {
      // Get the next U/V sample
      const int srcPosLineUV = srcIdxUV + x + 1;
      int       nextUSample  = getValueFromSource(srcU, srcPosLineUV * inValSkip, bps, bigEndian);
      int       nextVSample  = getValueFromSource(srcV, srcPosLineUV * inValSkip, bps, bigEndian);

      // From the current and the next U/V sample, interpolate the UV sample in between
      int interpolatedU = interpolateUVSample(interpolation, curUSample, nextUSample);
      int interpolatedV = interpolateUVSample(interpolation, curVSample, nextVSample);

      // Get the 2 Y samples
      int valY1 = getValueFromSource(srcY, y * w + x * 2, bps, bigEndian);
      int valY2 = getValueFromSource(srcY, y * w + x * 2 + 1, bps, bigEndian);

      // Convert to 2 RGB values and save them (BGRA)
      int valR1, valR2, valG1, valG2, valB1, valB2;
      convertYUVToRGB8Bit(
          valY1, curUSample, curVSample, valR1, valG1, valB1, RGBConv, fullRange, bps);
      convertYUVToRGB8Bit(
          valY2, interpolatedU, interpolatedV, valR2, valG2, valB2, RGBConv, fullRange, bps);
      const int pos = (y * w + x * 2) * 4;
      dst[pos]      = valB1;
      dst[pos + 1]  = valG1;
      dst[pos + 2]  = valR1;
      dst[pos + 3]  = 255;
      dst[pos + 4]  = valB2;
      dst[pos + 5]  = valG2;
      dst[pos + 6]  = valR2;
      dst[pos + 7]  = 255;

      // The next one is now the current one
      curUSample = nextUSample;
      curVSample = nextVSample;
    }

    // For the last row, there is no next sample. Just reuse the current one again. No interpolation
    // required either.

    // Get the 2 Y samples
    int valY1 = getValueFromSource(srcY, (y + 1) * w - 2, bps, bigEndian);
    int valY2 = getValueFromSource(srcY, (y + 1) * w - 1, bps, bigEndian);

    // Convert to 2 RGB values and save them
    int valR1, valR2, valG1, valG2, valB1, valB2;
    convertYUVToRGB8Bit(
        valY1, curUSample, curVSample, valR1, valG1, valB1, RGBConv, fullRange, bps);
    convertYUVToRGB8Bit(
        valY2, curUSample, curVSample, valR2, valG2, valB2, RGBConv, fullRange, bps);
    const int pos = ((y + 1) * w) * 4;
    dst[pos - 8]  = valB1;
    dst[pos - 7]  = valG1;
    dst[pos - 6]  = valR1;
    dst[pos - 5]  = 255;
    dst[pos - 4]  = valB2;
    dst[pos - 3]  = valG2;
    dst[pos - 2]  = valR2;
    dst[pos - 1]  = 255;
  }
}

inline void YUVPlaneToRGB_420(const int                     w,
                              const int                     h,
                              const unsigned char *restrict srcY,
                              const unsigned char *restrict srcU,
                              const unsigned char *restrict srcV,
                              unsigned char *restrict       dst,
                              const int                     RGBConv[5],
                              const bool                    fullRange,
                              const ChromaInterpolation     interpolation,
                              const int                     bps,
                              const bool                    bigEndian,
                              const int                     inValSkip)
{
  // Format is YUV 4:2:0. Horizontal and vertical up-sampling is required. Process 4 Y positions at
  // a time
  const int hh = h / 2; // The half values
  const int wh = w / 2;
  for (int y = 0; y < hh - 1; y++)
  {
    // Get the current U/V samples for this y line and the next one (_NL)
    const int srcIdxUV0 = y * wh;
    const int srcIdxUV1 = (y + 1) * wh;
    int       curU      = getValueFromSource(srcU, srcIdxUV0 * inValSkip, bps, bigEndian);
    int       curV      = getValueFromSource(srcV, srcIdxUV0 * inValSkip, bps, bigEndian);
    int       curU_NL   = getValueFromSource(srcU, srcIdxUV1 * inValSkip, bps, bigEndian);
    int       curV_NL   = getValueFromSource(srcV, srcIdxUV1 * inValSkip, bps, bigEndian);

    for (int x = 0; x < wh - 1; x++)
    {
      // Get the next U/V sample for this line and the next one
      const int srcIdxUVLine0 = srcIdxUV0 + x + 1;
      const int srcIdxUVLine1 = srcIdxUV1 + x + 1;
      int       nextU         = getValueFromSource(srcU, srcIdxUVLine0 * inValSkip, bps, bigEndian);
      int       nextV         = getValueFromSource(srcV, srcIdxUVLine0 * inValSkip, bps, bigEndian);
      int       nextU_NL      = getValueFromSource(srcU, srcIdxUVLine1 * inValSkip, bps, bigEndian);
      int       nextV_NL      = getValueFromSource(srcV, srcIdxUVLine1 * inValSkip, bps, bigEndian);

      // From the current and the next U/V sample, interpolate the 3 UV samples in between
      int interpolatedU_Hor =
          interpolateUVSample(interpolation, curU, nextU); // Horizontal interpolation
      int interpolatedV_Hor = interpolateUVSample(interpolation, curV, nextV);
      int interpolatedU_Ver =
          interpolateUVSample(interpolation, curU, curU_NL); // Vertical interpolation
      int interpolatedV_Ver = interpolateUVSample(interpolation, curV, curV_NL);
      int interpolatedU_Bi =
          interpolateUVSample2D(interpolation, curU, nextU, curU_NL, nextU_NL); // 2D interpolation
      int interpolatedV_Bi =
          interpolateUVSample2D(interpolation, curV, nextV, curV_NL, nextV_NL); // 2D interpolation

      // Get the 4 Y samples
      int valY1 = getValueFromSource(srcY, (y * w + x) * 2, bps, bigEndian);
      int valY2 = getValueFromSource(srcY, (y * w + x) * 2 + 1, bps, bigEndian);
      int valY3 = getValueFromSource(srcY, (y * 2 + 1) * w + x * 2, bps, bigEndian);
      int valY4 = getValueFromSource(srcY, (y * 2 + 1) * w + x * 2 + 1, bps, bigEndian);

      // Convert to 4 RGB values and save them
      int valR1, valR2, valG1, valG2, valB1, valB2;
      convertYUVToRGB8Bit(valY1, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps);
      convertYUVToRGB8Bit(valY2,
                          interpolatedU_Hor,
                          interpolatedV_Hor,
                          valR2,
                          valG2,
                          valB2,
                          RGBConv,
                          fullRange,
                          bps);
      const int pos1 = (y * 2 * w + x * 2) * 4;
      dst[pos1]      = valB1;
      dst[pos1 + 1]  = valG1;
      dst[pos1 + 2]  = valR1;
      dst[pos1 + 3]  = 255;
      dst[pos1 + 4]  = valB2;
      dst[pos1 + 5]  = valG2;
      dst[pos1 + 6]  = valR2;
      dst[pos1 + 7]  = 255;
      convertYUVToRGB8Bit(valY3,
                          interpolatedU_Ver,
                          interpolatedV_Ver,
                          valR1,
                          valG1,
                          valB1,
                          RGBConv,
                          fullRange,
                          bps); // Second line
      convertYUVToRGB8Bit(
          valY4, interpolatedU_Bi, interpolatedV_Bi, valR2, valG2, valB2, RGBConv, fullRange, bps);
      const int pos2 = pos1 + w * 4; // Next line
      dst[pos2]      = valB1;
      dst[pos2 + 1]  = valG1;
      dst[pos2 + 2]  = valR1;
      dst[pos2 + 3]  = 255;
      dst[pos2 + 4]  = valB2;
      dst[pos2 + 5]  = valG2;
      dst[pos2 + 6]  = valR2;
      dst[pos2 + 7]  = 255;

      // The next one is now the current one
      curU    = nextU;
      curV    = nextV;
      curU_NL = nextU_NL;
      curV_NL = nextV_NL;
    }

    // For the last x value (the right border), there is no next value. Just sample and hold. Only
    // vertical interpolation is required.
    int interpolatedU_Ver =
        interpolateUVSample(interpolation, curU, curU_NL); // Vertical interpolation
    int interpolatedV_Ver = interpolateUVSample(interpolation, curV, curV_NL);

    // Get the 4 Y samples
    int valY1 = getValueFromSource(srcY, (y * 2 + 1) * w - 2, bps, bigEndian);
    int valY2 = getValueFromSource(srcY, (y * 2 + 1) * w - 1, bps, bigEndian);
    int valY3 = getValueFromSource(srcY, (y * 2 + 2) * w - 2, bps, bigEndian);
    int valY4 = getValueFromSource(srcY, (y * 2 + 2) * w - 1, bps, bigEndian);

    // Convert to 4 RGB values and save them
    int valR1, valR2, valG1, valG2, valB1, valB2;
    convertYUVToRGB8Bit(valY1, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps);
    convertYUVToRGB8Bit(valY2, curU, curV, valR2, valG2, valB2, RGBConv, fullRange, bps);
    const int pos1 = ((y * 2 + 1) * w) * 4;
    dst[pos1 - 8]  = valB1;
    dst[pos1 - 7]  = valG1;
    dst[pos1 - 6]  = valR1;
    dst[pos1 - 5]  = 255;
    dst[pos1 - 4]  = valB2;
    dst[pos1 - 3]  = valG2;
    dst[pos1 - 2]  = valR2;
    dst[pos1 - 1]  = 255;
    convertYUVToRGB8Bit(valY3,
                        interpolatedU_Ver,
                        interpolatedV_Ver,
                        valR1,
                        valG1,
                        valB1,
                        RGBConv,
                        fullRange,
                        bps); // Second line
    convertYUVToRGB8Bit(
        valY4, interpolatedU_Ver, interpolatedV_Ver, valR2, valG2, valB2, RGBConv, fullRange, bps);
    const int pos2 = pos1 + w * 4; // Next line
    dst[pos2 - 8]  = valB1;
    dst[pos2 - 7]  = valG1;
    dst[pos2 - 6]  = valR1;
    dst[pos2 - 5]  = 255;
    dst[pos2 - 4]  = valB2;
    dst[pos2 - 3]  = valG2;
    dst[pos2 - 2]  = valR2;
    dst[pos2 - 1]  = 255;
  }

  // At the last Y line (the bottom line) a similar scenario occurs. There is no next Y line. Just
  // sample and hold. Only horizontal interpolation is required.

  // Get the current U/V samples for this y line
  const int y  = hh - 1; // Just process the last y line
  const int y2 = (hh - 1) * 2;

  // Get 2 chroma samples from this line
  const int srcIdxUV = y * wh;
  int       curU     = getValueFromSource(srcU, srcIdxUV * inValSkip, bps, bigEndian);
  int       curV     = getValueFromSource(srcV, srcIdxUV * inValSkip, bps, bigEndian);

  for (int x = 0; x < (w / 2) - 1; x++)
  {
    // Get the next U/V sample for this line and the next one
    const int srcIdxLineUV = srcIdxUV + x + 1;
    int       nextU        = getValueFromSource(srcU, srcIdxLineUV * inValSkip, bps, bigEndian);
    int       nextV        = getValueFromSource(srcV, srcIdxLineUV * inValSkip, bps, bigEndian);

    // From the current and the next U/V sample, interpolate the 3 UV samples in between
    int interpolatedU_Hor =
        interpolateUVSample(interpolation, curU, nextU); // Horizontal interpolation
    int interpolatedV_Hor = interpolateUVSample(interpolation, curV, nextV);

    // Get the 4 Y samples
    int valY1 = getValueFromSource(srcY, (y * w + x) * 2, bps, bigEndian);
    int valY2 = getValueFromSource(srcY, (y * w + x) * 2 + 1, bps, bigEndian);
    int valY3 = getValueFromSource(srcY, (y2 + 1) * w + x * 2, bps, bigEndian);
    int valY4 = getValueFromSource(srcY, (y2 + 1) * w + x * 2 + 1, bps, bigEndian);

    // Convert to 4 RGB values and save them
    int valR1, valR2, valG1, valG2, valB1, valB2;
    convertYUVToRGB8Bit(valY1, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps);
    convertYUVToRGB8Bit(
        valY2, interpolatedU_Hor, interpolatedV_Hor, valR2, valG2, valB2, RGBConv, fullRange, bps);
    const int pos1 = (y2 * w + x * 2) * 4;
    dst[pos1]      = valB1;
    dst[pos1 + 1]  = valG1;
    dst[pos1 + 2]  = valR1;
    dst[pos1 + 3]  = 255;
    dst[pos1 + 4]  = valB2;
    dst[pos1 + 5]  = valG2;
    dst[pos1 + 6]  = valR2;
    dst[pos1 + 7]  = 255;
    convertYUVToRGB8Bit(
        valY3, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps); // Second line
    convertYUVToRGB8Bit(
        valY4, interpolatedU_Hor, interpolatedV_Hor, valR2, valG2, valB2, RGBConv, fullRange, bps);
    const int pos2 = pos1 + w * 4; // Next line
    dst[pos2]      = valB1;
    dst[pos2 + 1]  = valG1;
    dst[pos2 + 2]  = valR1;
    dst[pos2 + 3]  = 255;
    dst[pos2 + 4]  = valB2;
    dst[pos2 + 5]  = valG2;
    dst[pos2 + 6]  = valR2;
    dst[pos2 + 7]  = 255;

    // The next one is now the current one
    curU = nextU;
    curV = nextV;
  }

  // For the last x value in the last y row (the right bottom), there is no next value in neither
  // direction. Just sample and hold. No interpolation is required.

  // Get the 4 Y samples
  int valY1 = getValueFromSource(srcY, (y2 + 1) * w - 2, bps, bigEndian);
  int valY2 = getValueFromSource(srcY, (y2 + 1) * w - 1, bps, bigEndian);
  int valY3 = getValueFromSource(srcY, (y2 + 2) * w - 2, bps, bigEndian);
  int valY4 = getValueFromSource(srcY, (y2 + 2) * w - 1, bps, bigEndian);

  // Convert to 4 RGB values and save them
  int valR1, valR2, valG1, valG2, valB1, valB2;
  convertYUVToRGB8Bit(valY1, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps);
  convertYUVToRGB8Bit(valY2, curU, curV, valR2, valG2, valB2, RGBConv, fullRange, bps);
  const int pos1 = (y2 + 1) * w * 4;
  dst[pos1 - 8]  = valB1;
  dst[pos1 - 7]  = valG1;
  dst[pos1 - 6]  = valR1;
  dst[pos1 - 5]  = 255;
  dst[pos1 - 4]  = valB2;
  dst[pos1 - 3]  = valG2;
  dst[pos1 - 2]  = valR2;
  dst[pos1 - 1]  = 255;
  convertYUVToRGB8Bit(
      valY3, curU, curV, valR1, valG1, valB1, RGBConv, fullRange, bps); // Second line
  convertYUVToRGB8Bit(valY4, curU, curV, valR2, valG2, valB2, RGBConv, fullRange, bps);
  const int pos2 = pos1 + w * 4; // Next line
  dst[pos2 - 8]  = valB1;
  dst[pos2 - 7]  = valG1;
  dst[pos2 - 6]  = valR1;
  dst[pos2 - 5]  = 255;
  dst[pos2 - 4]  = valB2;
  dst[pos2 - 3]  = valG2;
  dst[pos2 - 2]  = valR2;
  dst[pos2 - 1]  = 255;
}

// This is a specialized function that can convert 8-bit YUV 4:2:0 to RGB888 using
// NearestNeighborInterpolation. The chroma must be 0 in x direction and 1 in y direction. No
// yuvMath is supported.
// TODO: Correct the chroma subsampling offset.
bool convertYUV420ToRGB(const QByteArray &   sourceBuffer,
                        unsigned char *      targetBuffer,
                        const Size           size,
                        const YUVPixelFormat format)
{
  const auto frameWidth  = size.width;
  const auto frameHeight = size.height;

  // For 4:2:0, w and h must be dividible by 2
  assert(frameWidth % 2 == 0 && frameHeight % 2 == 0);

  int componentLenghtY  = frameWidth * frameHeight;
  int componentLengthUV = componentLenghtY >> 2;
  Q_ASSERT(sourceBuffer.size() >= componentLenghtY + componentLengthUV +
                                      componentLengthUV); // YUV 420 must be (at least) 1.5*Y-area

  // Perform software based 420 to RGB conversion
  static unsigned char  clp_buf[384 + 256 + 384];
  static unsigned char *clip_buf            = clp_buf + 384;
  static bool           clp_buf_initialized = false;

  if (!clp_buf_initialized)
  {
    // Initialize clipping table. Because of the static bool, this will only be called once.
    memset(clp_buf, 0, 384);
    int i;
    for (i = 0; i < 256; i++)
      clp_buf[384 + i] = i;
    memset(clp_buf + 384 + 256, 255, 384);
    clp_buf_initialized = true;
  }

  unsigned char *restrict dst = targetBuffer;

  // Get/set the parameters used for YUV -> RGB conversion
  auto       yuvColorConversionType = ColorConversion::BT709_LimitedRange;
  const bool fullRange              = (yuvColorConversionType == ColorConversion::BT709_FullRange ||
                          yuvColorConversionType == ColorConversion::BT601_FullRange ||
                          yuvColorConversionType == ColorConversion::BT2020_FullRange);
  const int  yOffset                = (fullRange ? 0 : 16);
  const int  cZero                  = 128;
  int        RGBConv[5];
  getColorConversionCoefficients(yuvColorConversionType, RGBConv);

  // Get pointers to the source and the output array
  const bool uPplaneFirst =
      (format.getPlaneOrder() == PlaneOrder::YUV ||
       format.getPlaneOrder() == PlaneOrder::YUVA); // Is the U plane the first or the second?
  const unsigned char *restrict srcY = (unsigned char *)sourceBuffer.data();
  const unsigned char *restrict srcU =
      uPplaneFirst ? srcY + componentLenghtY : srcY + componentLenghtY + componentLengthUV;
  const unsigned char *restrict srcV =
      uPplaneFirst ? srcY + componentLenghtY + componentLengthUV : srcY + componentLenghtY;

  for (unsigned yh = 0; yh < frameHeight / 2; yh++)
  {
    // Process two lines at once, always 4 RGB values at a time (they have the same U/V components)

    int dstAddr1  = yh * 2 * frameWidth * 4;       // The RGB output address of line yh*2
    int dstAddr2  = (yh * 2 + 1) * frameWidth * 4; // The RGB output address of line yh*2+1
    int srcAddrY1 = yh * 2 * frameWidth;           // The Y source address of line yh*2
    int srcAddrY2 = (yh * 2 + 1) * frameWidth;     // The Y source address of line yh*2+1
    int srcAddrUV = yh * frameWidth / 2; // The UV source address of both lines (UV are identical)

    for (unsigned xh = 0, x = 0; xh < frameWidth / 2; xh++, x += 2)
    {
      // Process four pixels (the ones for which U/V are valid

      // Load UV and pre-multiply
      const int U_tmp_G = ((int)srcU[srcAddrUV + xh] - cZero) * RGBConv[2];
      const int U_tmp_B = ((int)srcU[srcAddrUV + xh] - cZero) * RGBConv[4];
      const int V_tmp_R = ((int)srcV[srcAddrUV + xh] - cZero) * RGBConv[1];
      const int V_tmp_G = ((int)srcV[srcAddrUV + xh] - cZero) * RGBConv[3];

      // Pixel top left
      {
        const int Y_tmp = ((int)srcY[srcAddrY1 + x] - yOffset) * RGBConv[0];

        const int R_tmp = (Y_tmp + V_tmp_R) >> 16;
        const int G_tmp = (Y_tmp + U_tmp_G + V_tmp_G) >> 16;
        const int B_tmp = (Y_tmp + U_tmp_B) >> 16;

        dst[dstAddr1]     = clip_buf[B_tmp];
        dst[dstAddr1 + 1] = clip_buf[G_tmp];
        dst[dstAddr1 + 2] = clip_buf[R_tmp];
        dst[dstAddr1 + 3] = 255;
        dstAddr1 += 4;
      }
      // Pixel top right
      {
        const int Y_tmp = ((int)srcY[srcAddrY1 + x + 1] - yOffset) * RGBConv[0];

        const int R_tmp = (Y_tmp + V_tmp_R) >> 16;
        const int G_tmp = (Y_tmp + U_tmp_G + V_tmp_G) >> 16;
        const int B_tmp = (Y_tmp + U_tmp_B) >> 16;

        dst[dstAddr1]     = clip_buf[B_tmp];
        dst[dstAddr1 + 1] = clip_buf[G_tmp];
        dst[dstAddr1 + 2] = clip_buf[R_tmp];
        dst[dstAddr1 + 3] = 255;
        dstAddr1 += 4;
      }
      // Pixel bottom left
      {
        const int Y_tmp = ((int)srcY[srcAddrY2 + x] - yOffset) * RGBConv[0];

        const int R_tmp = (Y_tmp + V_tmp_R) >> 16;
        const int G_tmp = (Y_tmp + U_tmp_G + V_tmp_G) >> 16;
        const int B_tmp = (Y_tmp + U_tmp_B) >> 16;

        dst[dstAddr2]     = clip_buf[B_tmp];
        dst[dstAddr2 + 1] = clip_buf[G_tmp];
        dst[dstAddr2 + 2] = clip_buf[R_tmp];
        dst[dstAddr2 + 3] = 255;
        dstAddr2 += 4;
      }
      // Pixel bottom right
      {
        const int Y_tmp = ((int)srcY[srcAddrY2 + x + 1] - yOffset) * RGBConv[0];

        const int R_tmp = (Y_tmp + V_tmp_R) >> 16;
        const int G_tmp = (Y_tmp + U_tmp_G + V_tmp_G) >> 16;
        const int B_tmp = (Y_tmp + U_tmp_B) >> 16;

        dst[dstAddr2]     = clip_buf[B_tmp];
        dst[dstAddr2 + 1] = clip_buf[G_tmp];
        dst[dstAddr2 + 2] = clip_buf[R_tmp];
        dst[dstAddr2 + 3] = 255;
        dstAddr2 += 4;
      }
    }
  }

  return true;
}

bool convertYUVPlanarToRGB(const QByteArray &    sourceBuffer,
                           uchar *               targetBuffer,
                           const Size            curFrameSize,
                           const YUVPixelFormat &sourceBufferFormat)
{
  // These are constant for the runtime of this function. This way, the compiler can optimize the
  // hell out of this function.
  const auto format        = sourceBufferFormat;
  const auto interpolation = ChromaInterpolation::NearestNeighbor;
  const auto conversion    = ColorConversion::BT709_LimitedRange;
  const auto w             = curFrameSize.width;
  const auto h             = curFrameSize.height;

  const auto bps       = format.getBitsPerSample();
  const bool fullRange = (conversion == ColorConversion::BT709_FullRange ||
                          conversion == ColorConversion::BT601_FullRange ||
                          conversion == ColorConversion::BT2020_FullRange);
  // const auto yOffset = 16<<(bps-8);
  // const auto cZero = 128<<(bps-8);

  // The luma component has full resolution. The size of each chroma components depends on the
  // subsampling.
  const auto componentSizeLuma = (w * h);
  const auto componentSizeChroma =
      (w / format.getSubsamplingHor()) * (h / format.getSubsamplingVer());

  // How many bytes are in each component?
  const auto nrBytesLumaPlane   = (bps > 8) ? componentSizeLuma * 2 : componentSizeLuma;
  const auto nrBytesChromaPlane = (bps > 8) ? componentSizeChroma * 2 : componentSizeChroma;

  // If the U and V (and A if present) components are interlevaed, we have to skip every nth value
  // in the input when reading U and V
  const auto inputValSkip = format.isUVInterleaved()
                                ? ((format.getPlaneOrder() == PlaneOrder::YUV ||
                                    format.getPlaneOrder() == PlaneOrder::YVU)
                                       ? 2
                                       : 3)
                                : 1;

  // A pointer to the output
  unsigned char *restrict dst = targetBuffer;

  // Is the U plane the first or the second?
  const bool uPlaneFirst =
      (format.getPlaneOrder() == PlaneOrder::YUV || format.getPlaneOrder() == PlaneOrder::YUVA);

  // In case the U and V (and A if present) components are interleaved, the skip to the next plane
  // is just 1 (or 2) bytes
  int nrBytesToNextChromaPlane = nrBytesChromaPlane;
  if (format.isUVInterleaved())
    nrBytesToNextChromaPlane = (bps > 8) ? 2 : 1;

  // Get/set the parameters used for YUV -> RGB conversion
  int RGBConv[5];
  getColorConversionCoefficients(conversion, RGBConv);

  // We are displaying all components, so we have to perform conversion to RGB (possibly including
  // interpolation and YUV math)

  // Get the pointers to the source planes (8 bit per sample)
  const unsigned char *restrict srcY = (unsigned char *)sourceBuffer.data();
  const unsigned char *restrict srcU =
      uPlaneFirst ? srcY + nrBytesLumaPlane : srcY + nrBytesLumaPlane + nrBytesToNextChromaPlane;
  const unsigned char *restrict srcV =
      uPlaneFirst ? srcY + nrBytesLumaPlane + nrBytesToNextChromaPlane : srcY + nrBytesLumaPlane;

  if (format.getSubsampling() == Subsampling::YUV_444)
    YUVPlaneToRGB_444(componentSizeLuma,
                      srcY,
                      srcU,
                      srcV,
                      dst,
                      RGBConv,
                      fullRange,
                      bps,
                      format.isBigEndian(),
                      inputValSkip);
  else if (format.getSubsampling() == Subsampling::YUV_422)
    YUVPlaneToRGB_422(w,
                      h,
                      srcY,
                      srcU,
                      srcV,
                      dst,
                      RGBConv,
                      fullRange,
                      interpolation,
                      bps,
                      format.isBigEndian(),
                      inputValSkip);
  else if (format.getSubsampling() == Subsampling::YUV_420)
    YUVPlaneToRGB_420(w,
                      h,
                      srcY,
                      srcU,
                      srcV,
                      dst,
                      RGBConv,
                      fullRange,
                      interpolation,
                      bps,
                      format.isBigEndian(),
                      inputValSkip);
  else
    Q_ASSERT_X(false, "convertYUVPlanarToRGB", "Subsampling not supported");

  return true;
}

// Convert the given raw YUV data in sourceBuffer (using srcPixelFormat) to image (RGB-888), using
// the buffer tmpRGBBuffer for intermediate RGB values.
void convertYUVToImage(const QByteArray &    sourceBuffer,
                       QImage &              outputImage,
                       const YUVPixelFormat &yuvFormat,
                       const Size &          curFrameSize)
{
  if (!yuvFormat.canConvertToRGB(curFrameSize) || sourceBuffer.isEmpty())
  {
    outputImage = QImage();
    return;
  }

  // Create the output image in the right format.
  // In both cases, we will set the alpha channel to 255. The format of the raw buffer is: BGRA
  // (each 8 bit). Internally, this is how QImage allocates the number of bytes per line (with depth
  // = 32): const int bytes_per_line = ((width * depth + 31) >> 5) << 2; // bytes per scanline (must
  // be multiple of 4)
  auto qFrameSize = QSize(int(curFrameSize.width), int(curFrameSize.height));
  if (is_Q_OS_WIN || is_Q_OS_MAC)
    outputImage = QImage(qFrameSize, platformImageFormat());
  else if (is_Q_OS_LINUX)
  {
    QImage::Format f = platformImageFormat();
    if (f == QImage::Format_ARGB32_Premultiplied || f == QImage::Format_ARGB32)
      outputImage = QImage(qFrameSize, f);
    else
      outputImage = QImage(qFrameSize, QImage::Format_RGB32);
  }

  // Check the image buffer size before we write to it
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
  assert(clipToUnsigned(outputImage.byteCount()) >= curFrameSize.width * curFrameSize.height * 4);
#else
  assert(clipToUnsigned(outputImage.sizeInBytes()) >= curFrameSize.width * curFrameSize.height * 4);
#endif

  bool convOK;

  // Convert the source to RGB
  const auto chromaInterpolation = ChromaInterpolation::NearestNeighbor;
  if (yuvFormat.isPlanar())
  {
    if (yuvFormat.getBitsPerSample() == 8 && yuvFormat.getSubsampling() == Subsampling::YUV_420 &&
        chromaInterpolation == ChromaInterpolation::NearestNeighbor &&
        yuvFormat.getChromaOffset().x == 0 && yuvFormat.getChromaOffset().y == 1 &&
        !yuvFormat.isUVInterleaved())
      // 8 bit 4:2:0, nearest neighbor, chroma offset (0,1) (the default for 4:2:0), all components
      // displayed and no yuv math. We can use a specialized function for this.
      convOK = convertYUV420ToRGB(sourceBuffer, outputImage.bits(), curFrameSize, yuvFormat);
    else
      convOK = convertYUVPlanarToRGB(sourceBuffer, outputImage.bits(), curFrameSize, yuvFormat);
  }
  else
  {
    Q_ASSERT_X(false, "convertYUVToImage", "Only planar formats supported");
  }

  assert(convOK);

  if (is_Q_OS_LINUX)
  {
    // On linux, we may have to convert the image to the platform image format if it is not one of
    // the RGBA formats.
    QImage::Format f = platformImageFormat();
    if (f != QImage::Format_ARGB32_Premultiplied && f != QImage::Format_ARGB32 &&
        f != QImage::Format_RGB32)
      outputImage = outputImage.convertToFormat(f);
  }
}