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

#include "alf_data.h"

#include "adaptation_parameter_set_rbsp.h"
#include "parser/common/functions.h"

#include <cmath>

namespace parser::vvc
{

using namespace parser::reader;

void alf_data::parse(SubByteReaderLogging &reader, adaptation_parameter_set_rbsp *aps)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "alf_data");

  this->alf_luma_filter_signal_flag = reader.readFlag("alf_luma_filter_signal_flag");
  if (aps->aps_chroma_present_flag)
  {
    this->alf_chroma_filter_signal_flag = reader.readFlag("alf_chroma_filter_signal_flag");
    this->alf_cc_cb_filter_signal_flag  = reader.readFlag("alf_cc_cb_filter_signal_flag");
    this->alf_cc_cr_filter_signal_flag  = reader.readFlag("alf_cc_cr_filter_signal_flag");
  }
  if (this->alf_luma_filter_signal_flag)
  {
    this->alf_luma_clip_flag = reader.readFlag("alf_luma_clip_flag");
    this->alf_luma_num_filters_signalled_minus1 =
        reader.readUEV("alf_luma_num_filters_signalled_minus1");
    if (this->alf_luma_num_filters_signalled_minus1 > 0)
    {
      auto NumAlfFilters = 25u;
      for (unsigned filtIdx = 0; filtIdx < NumAlfFilters; filtIdx++)
      {
        auto nrBits = std::ceil(std::log2(alf_luma_num_filters_signalled_minus1 + 1));
        this->alf_luma_coeff_delta_idx = reader.readBits(
            formatArray("alf_luma_coeff_delta_idx", filtIdx),
            nrBits,
            Options().withCheckRange({0, this->alf_luma_num_filters_signalled_minus1}));
      }
    }
    for (unsigned sfIdx = 0; sfIdx <= alf_luma_num_filters_signalled_minus1; sfIdx++)
    {
      this->alf_luma_coeff_abs.push_back({});
      this->alf_luma_coeff_sign.push_back({});
      for (unsigned j = 0; j < 12; j++)
      {
        this->alf_luma_coeff_abs[sfIdx].push_back(
            reader.readUEV(formatArray("alf_luma_coeff_abs", sfIdx, j)));
        if (this->alf_luma_coeff_abs[sfIdx][j])
        {
          this->alf_luma_coeff_sign[sfIdx].push_back(
              reader.readFlag(formatArray("alf_luma_coeff_sign", sfIdx, j)));
        }
        else
        {
          this->alf_luma_coeff_sign[sfIdx].push_back(0);
        }
      }
    }
    if (this->alf_luma_clip_flag)
    {
      for (unsigned sfIdx = 0; sfIdx <= alf_luma_num_filters_signalled_minus1; sfIdx++)
      {
        this->alf_luma_clip_idx.push_back({});
        for (unsigned j = 0; j < 12; j++)
        {
          this->alf_luma_clip_idx[sfIdx].push_back(
              reader.readBits(formatArray("alf_luma_clip_idx", sfIdx, j), 2));
        }
      }
    }
  }
  if (this->alf_chroma_filter_signal_flag)
  {
    this->alf_chroma_clip_flag              = reader.readFlag("alf_chroma_clip_flag");
    this->alf_chroma_num_alt_filters_minus1 = reader.readUEV("alf_chroma_num_alt_filters_minus1");
    for (unsigned altIdx = 0; altIdx <= alf_chroma_num_alt_filters_minus1; altIdx++)
    {
      this->alf_chroma_coeff_abs.push_back({});
      this->alf_chroma_coeff_sign.push_back({});
      this->alf_chroma_clip_idx.push_back({});
      for (unsigned j = 0; j < 6; j++)
      {
        this->alf_chroma_coeff_abs[altIdx].push_back(
            reader.readUEV(formatArray("alf_chroma_coeff_abs", altIdx, j)));
        if (this->alf_chroma_coeff_abs[altIdx][j] > 0)
        {
          this->alf_chroma_coeff_sign[altIdx].push_back(
              reader.readFlag(formatArray("alf_chroma_coeff_sign", altIdx, j)));
        }
        else
        {
          this->alf_chroma_coeff_sign[altIdx].push_back(0);
        }
      }
      if (this->alf_chroma_clip_flag)
      {
        for (unsigned j = 0; j < 6; j++)
        {
          this->alf_chroma_clip_idx[altIdx].push_back(
              reader.readBits(formatArray("alf_chroma_clip_idx", altIdx, j), 2));
        }
      }
    }
  }
  if (this->alf_cc_cb_filter_signal_flag)
  {
    this->alf_cc_cb_filters_signalled_minus1 =
        reader.readUEV("alf_cc_cb_filters_signalled_minus1", Options().withCheckRange({0, 3}));
    for (unsigned k = 0; k < alf_cc_cb_filters_signalled_minus1 + 1; k++)
    {
      this->alf_cc_cb_mapped_coeff_abs.push_back({});
      this->alf_cc_cb_coeff_sign.push_back({});
      for (unsigned j = 0; j < 7; j++)
      {
        this->alf_cc_cb_mapped_coeff_abs[k].push_back(
            reader.readBits(formatArray("alf_cc_cb_mapped_coeff_abs", k, j), 3));
        if (this->alf_cc_cb_mapped_coeff_abs[k][j])
        {
          this->alf_cc_cb_coeff_sign[k].push_back(
              reader.readFlag(formatArray("alf_cc_cb_coeff_sign", k, j)));
        }
        else
        {
          this->alf_cc_cb_coeff_sign[k].push_back(0);
        }
      }
    }
  }
  if (this->alf_cc_cr_filter_signal_flag)
  {
    this->alf_cc_cr_filters_signalled_minus1 =
        reader.readUEV("alf_cc_cr_filters_signalled_minus1", Options().withCheckRange({0, 3}));
    for (unsigned k = 0; k < alf_cc_cr_filters_signalled_minus1 + 1; k++)
    {
      this->alf_cc_cr_mapped_coeff_abs.push_back({});
      this->alf_cc_cr_coeff_sign.push_back({});
      for (unsigned j = 0; j < 7; j++)
      {
        this->alf_cc_cr_mapped_coeff_abs[k].push_back(
            reader.readBits(formatArray("alf_cc_cr_mapped_coeff_abs", k, j), 3));
        if (this->alf_cc_cr_mapped_coeff_abs[k][j])
        {
          this->alf_cc_cr_coeff_sign[k].push_back(
              reader.readFlag(formatArray("alf_cc_cr_coeff_sign", k, j)));
        }
        else
        {
          this->alf_cc_cr_coeff_sign[k].push_back(0);
        }
      }
    }
  }
}

} // namespace parser::vvc
