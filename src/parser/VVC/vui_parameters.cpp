/* MIT License

Copyright (c) 2021 Christian Feldmann <christian.feldmann@gmx.de>
                                      <christian.feldmann@bitmovin.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "vui_parameters.h"

namespace parser::vvc
{

using namespace reader;

void vui_parameters::parse(reader::SubByteReaderLogging &reader, unsigned)
{
  this->vui_progressive_source_flag        = reader.readFlag("vui_progressive_source_flag");
  this->vui_interlaced_source_flag         = reader.readFlag("vui_interlaced_source_flag");
  this->vui_non_packed_constraint_flag     = reader.readFlag("vui_non_packed_constraint_flag");
  this->vui_non_projected_constraint_flag  = reader.readFlag("vui_non_projected_constraint_flag");
  this->vui_aspect_ratio_info_present_flag = reader.readFlag("vui_aspect_ratio_info_present_flag");

  if (this->vui_aspect_ratio_info_present_flag)
  {
    this->vui_aspect_ratio_constant_flag = reader.readFlag("vui_aspect_ratio_constant_flag");
    this->vui_aspect_ratio_idc =
        reader.readBits("vui_aspect_ratio_idc",
                        8,
                        Options().withMeaningMap(sampleAspectRatioCoding.getMeaningMap()));
    if (this->vui_aspect_ratio_idc == 255)
    {
      this->vui_sar_width  = reader.readBits("vui_sar_width", 16);
      this->vui_sar_height = reader.readBits("vui_sar_height", 16);
    }
  }
  this->vui_overscan_info_present_flag = reader.readFlag("vui_overscan_info_present_flag");
  if (this->vui_overscan_info_present_flag)
  {
    this->vui_overscan_appropriate_flag = reader.readFlag("vui_overscan_appropriate_flag");
  }
  this->vui_colour_description_present_flag =
      reader.readFlag("vui_colour_description_present_flag");
  if (this->vui_colour_description_present_flag)
  {
    this->vui_colour_primaries = reader.readBits(
        "vui_colour_primaries",
        8,
        Options()
            .withMeaningVector({"For future use by ITU-T | ISO/IEC",
                                "Rec. ITU-R BT.709-6 / BT.1361 / IEC 61966-2-1 (sRGB or sYCC)",
                                "Unspecified",
                                "Reserved For future use by ITU - T | ISO / IEC",
                                "Rec. ITU-R BT.470-6 System M (historical) (NTSC)",
                                "Rec. ITU-R BT.470-6 System B, G (historical) / BT.601 / BT.1358"
                                "Rec. ITU-R BT.601-7 525 / BT.1358-1 525 or 625 (historical) / "
                                "BT.1700-0 NTSC / SMPTE 170M (2004)",
                                "SMPTE 240M (1999) (historical)",
                                "Generic film (colour filters using Illuminant C)",
                                "Rec. ITU-R BT.2020-2 / BT.2100-0",
                                "SMPTE ST 428-1 (CIE 1931 XYZ as in ISO 11664-1)",
                                "SMPTE RP 431-2 (2011)",
                                "SMPTE EG 432-1 (2010)",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "For future use by ITU-T | ISO/IEC",
                                "EBU Tech. 3213-E (1975)"})
            .withMeaning("For future use by ITU-T | ISO/IEC"));
    this->vui_transfer_characteristics = reader.readBits(
        "vui_transfer_characteristics",
        8,
        Options()
            .withMeaningVector(
                {"For future use by ITU-T | ISO/IEC",
                 "Rec. ITU-R BT.709-6 / conventional colour gamut system (historical)",
                 "Image characteristics are unknown or are determined by the application",
                 "For future use by ITU-T | ISO/IEC",
                 "Rec. ITU-R BT.470-6 System M (historical) / United States National Television "
                 "System Committee 1953 / Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM",
                 "Rec. ITU-R BT.470-6 System B, G (historical)",
                 "Rec. ITU-R BT.601-7 525 or 625 / BT.1358-1 525 or 625 (historical) / BT.1700-0 "
                 "NTSC / SMPTE 170M (2004)",
                 "SMPTE 240M (1999) (historical)",
                 "Linear transfer characteristics",
                 "Logarithmic transfer characteristic (100:1 range)",
                 "Logarithmic transfer characteristic (100 * Sqrt( 10 ) : 1 range)",
                 "IEC 61966-2-4",
                 "Rec. ITU-R BT.1361-0 extended colour gamut system (historical)",
                 "IEC 61966-2-1 sRGB or sYCC",
                 "Rec. ITU-R BT.2020-2 (10-bit system)",
                 "Rec. ITU-R BT.2020-2 (12-bit system)",
                 "SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems / Rec. ITU-R BT.2100-0 "
                 "perceptual quantization (PQ) system",
                 "SMPTE ST 428-1",
                 "ARIB STD-B67 Rec. ITU-R BT.2100-0 hybrid loggamma (HLG) system"})
            .withMeaning("For future use by ITU-T | ISO/IEC"));
    this->vui_matrix_coeffs = reader.readBits(
        "vui_matrix_coeffs",
        8,
        Options()
            .withMeaningVector(
                {"The identity matrix. IEC 61966-2-1 sRGB SMPTE ST 428-1",
                 "Rec. ITU-R BT.709-6 / ITU-R BT.1361-0 (historical) / IEC 61966-2-1 sYCC / IEC "
                 "61966-2-4 xvYCC709 / SMPTE RP 177 (1993) Annex B",
                 "Image characteristics are unknown or are determined by the application",
                 "For future use by ITU-T | ISO/IEC",
                 "United States Federal Communications Commission (2003) Title 47 Code of Federal "
                 "Regulations 73.682",
                 "Rec. ITU-R BT.470-6 System B, G (historical) / Rec. ITU-R BT.601-7 625 / Rec. "
                 "ITU-R BT.1358-0 625 (historical) / Rec. ITU-R BT.1700-0 625 PAL and 625 SECAM / "
                 "IEC 61966-2-4 xvYCC601",
                 "Rec. ITU-R BT.601-7 525 / Rec. ITU-R BT.1358-1 525 or 625 (historical) / Rec. "
                 "ITU-R BT.1700-0 NTSC / SMPTE 170M (2004)",
                 "SMPTE 240M (1999) (historical)",
                 "YCgCo",
                 "Rec. ITU-R BT.2020-2 (non-constant luminance) / Rec. ITU-R BT.2100-0 Y′CbCr",
                 "Rec. ITU-R BT.2020-2 (constant luminance)",
                 "SMPTE ST 2085 (2015)",
                 "Chromaticity-derived non-constant luminance system",
                 "Chromaticity-derived constant luminance system",
                 "Rec. ITU-R BT.2100-0 ICTCP"})
            .withMeaning("For future use by ITU-T | ISO/IEC"));
    this->vui_full_range_flag = reader.readFlag("vui_full_range_flag");
  }
  this->vui_chroma_loc_info_present_flag = reader.readFlag("vui_chroma_loc_info_present_flag");
  if (this->vui_chroma_loc_info_present_flag)
  {
    auto chromaLocMeaningVector = std::vector<std::string>(
        {"Middle Left", "Center", "Top Left", "Top Center", "Bottom Left", "Bottom Center"});

    if (this->vui_progressive_source_flag && !this->vui_interlaced_source_flag)
    {
      this->vui_chroma_sample_loc_type_frame = reader.readUEV(
          "vui_chroma_sample_loc_type_frame",
          Options().withCheckRange({0, 5}).withMeaningVector(chromaLocMeaningVector));
    }
    else
    {
      this->vui_chroma_sample_loc_type_top_field = reader.readUEV(
          "vui_chroma_sample_loc_type_top_field",
          Options().withCheckRange({0, 5}).withMeaningVector(chromaLocMeaningVector));
      this->vui_chroma_sample_loc_type_bottom_field = reader.readUEV(
          "vui_chroma_sample_loc_type_bottom_field",
          Options().withCheckRange({0, 5}).withMeaningVector(chromaLocMeaningVector));
    }
  }
}

} // namespace parser::vvc
