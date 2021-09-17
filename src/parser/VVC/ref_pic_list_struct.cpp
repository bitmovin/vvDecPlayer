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

#include "ref_pic_list_struct.h"

#include "parser/common/functions.h"
#include "seq_parameter_set_rbsp.h"

namespace parser::vvc
{

using namespace parser::reader;

void ref_pic_list_struct::parse(SubByteReaderLogging &                  reader,
                                unsigned                                listIdx,
                                unsigned                                rplsIdx,
                                std::shared_ptr<seq_parameter_set_rbsp> sps)
{
  assert(sps != nullptr);
  SubByteReaderLoggingSubLevel subLevel(reader,
                                        formatArray("ref_pic_list_struct", listIdx, rplsIdx));

  this->num_ref_entries = reader.readUEV("num_ref_entries");
  if (sps->sps_long_term_ref_pics_flag && rplsIdx < sps->sps_num_ref_pic_lists[listIdx] &&
      this->num_ref_entries > 0)
  {
    this->ltrp_in_header_flag = reader.readFlag("ltrp_in_header_flag");
  }
  unsigned j = 0;
  for (unsigned i = 0; i < this->num_ref_entries; i++)
  {
    if (sps->sps_inter_layer_prediction_enabled_flag)
    {
      this->inter_layer_ref_pic_flag[i] =
          reader.readFlag(formatArray("inter_layer_ref_pic_flag", i));
    }
    if (!this->inter_layer_ref_pic_flag[i])
    {
      if (sps->sps_long_term_ref_pics_flag)
      {
        this->st_ref_pic_flag[i] = reader.readFlag(formatArray("st_ref_pic_flag", i));
      }
      if (this->getStRefPicFlag(i))
      {
        this->abs_delta_poc_st[i] = reader.readUEV(formatArray("abs_delta_poc_st", i));

        // (149)
        if ((sps->sps_weighted_pred_flag || sps->sps_weighted_bipred_flag) && i != 0)
          this->AbsDeltaPocSt[i] = abs_delta_poc_st[i];
        else
          this->AbsDeltaPocSt[i] = abs_delta_poc_st[i] + 1;

        if (this->AbsDeltaPocSt[i])
        {
          this->strp_entry_sign_flag[i] = reader.readFlag(formatArray("strp_entry_sign_flag", i));
        }
      }
      else if (!this->ltrp_in_header_flag)
      {
        auto numBits               = sps->sps_log2_max_pic_order_cnt_lsb_minus4 + 4;
        this->rpls_poc_lsb_lt[j++] = reader.readBits(formatArray("rpls_poc_lsb_lt", i), numBits);
      }
    }
    else
    {
      this->ilrp_idx[i] = reader.readUEV(formatArray("ilrp_idx", i));
    }
  }

  // (148)
  this->NumLtrpEntries = 0;
  for (unsigned i = 0; i < this->num_ref_entries; i++)
    if (!this->inter_layer_ref_pic_flag[i] && !this->getStRefPicFlag(i))
      this->NumLtrpEntries++;

  // (150)
  for (unsigned i = 0; i < this->num_ref_entries; i++)
    if (!this->inter_layer_ref_pic_flag[i] && this->getStRefPicFlag(i))
      this->DeltaPocValSt[i] =
          (1 - 2 * int(this->strp_entry_sign_flag[i])) * this->AbsDeltaPocSt[i];
}

bool ref_pic_list_struct::getStRefPicFlag(unsigned i)
{
  // The default value of a non existent st_ref_pic_flag is true
  if (!this->inter_layer_ref_pic_flag[i] && this->st_ref_pic_flag.count(i) == 0)
    this->st_ref_pic_flag[i] = true;

  return this->st_ref_pic_flag[i];
}

} // namespace parser::vvc
