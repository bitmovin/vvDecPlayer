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

#pragma once

#include "parser/common/SubByteReaderLogging.h"

namespace parser::vvc
{

enum class NalType
{
  TRAIL_NUT,
  STSA_NUT,
  RADL_NUT,
  RASL_NUT,
  RSV_VCL_4,
  RSV_VCL_5,
  RSV_VCL_6,
  IDR_W_RADL,
  IDR_N_LP,
  CRA_NUT,
  GDR_NUT,
  RSV_IRAP_11,
  OPI_NUT,
  DCI_NUT,
  VPS_NUT,
  SPS_NUT,
  PPS_NUT,
  PREFIX_APS_NUT,
  SUFFIX_APS_NUT,
  PH_NUT,
  AUD_NUT,
  EOS_NUT,
  EOB_NUT,
  PREFIX_SEI_NUT,
  SUFFIX_SEI_NUT,
  FD_NUT,
  RSV_NVCL_26,
  RSV_NVCL_27,
  UNSPEC_28,
  UNSPEC_29,
  UNSPEC_30,
  UNSPEC_31,
  UNSPECIFIED
};

class nal_unit_header
{
public:
  nal_unit_header()  = default;
  ~nal_unit_header() = default;
  void parse(reader::SubByteReaderLogging &reader);

  bool isSlice() const
  {
    return this->nal_unit_type == NalType::TRAIL_NUT || this->nal_unit_type == NalType::STSA_NUT ||
           this->nal_unit_type == NalType::RADL_NUT || this->nal_unit_type == NalType::RASL_NUT ||
           this->nal_unit_type == NalType::IDR_W_RADL || this->nal_unit_type == NalType::IDR_N_LP ||
           this->nal_unit_type == NalType::CRA_NUT || this->nal_unit_type == NalType::GDR_NUT;
  }

  unsigned nuh_layer_id;
  unsigned nuh_temporal_id_plus1;

  NalType  nal_unit_type;
  unsigned nalUnitTypeID;
};

} // namespace parser::vvc
