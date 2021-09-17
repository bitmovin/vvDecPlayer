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

#include "ref_pic_lists.h"

#include "parser/common/functions.h"
#include "pic_parameter_set_rbsp.h"
#include "seq_parameter_set_rbsp.h"

#include <cmath>

namespace parser::vvc
{

using namespace parser::reader;

void ref_pic_lists::parse(SubByteReaderLogging &                  reader,
                          std::shared_ptr<seq_parameter_set_rbsp> sps,
                          std::shared_ptr<pic_parameter_set_rbsp> pps)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "ref_pic_lists");

  for (unsigned i = 0; i < 2; i++)
  {
    this->delta_poc_msb_cycle_present_flag.push_back({});
    if (sps->sps_num_ref_pic_lists[i] > 0 && (i == 0 || (i == 1 && pps->pps_rpl1_idx_present_flag)))
    {
      this->rpl_sps_flag[i] = reader.readFlag(formatArray("rpl_sps_flag", i));
    }
    else
    {
      // 7.40.10 If rpl_sps_flag is not present
      if (sps->sps_num_ref_pic_lists[i] == 0)
      {
        this->rpl_sps_flag[i] = 0;
      }
      else if (!pps->pps_rpl1_idx_present_flag && i == 1)
      {
        this->rpl_sps_flag[1] = rpl_sps_flag[0];
      }
    }
    if (this->rpl_sps_flag[i])
    {
      if (sps->sps_num_ref_pic_lists[i] > 1 &&
          (i == 0 || (i == 1 && pps->pps_rpl1_idx_present_flag)))
      {
        auto nrBits      = std::ceil(std::log2(sps->sps_num_ref_pic_lists[i]));
        this->rpl_idx[i] = reader.readBits(formatArray("rpl_idx", i), nrBits);
      }
      else if (i == 1 && !pps->pps_rpl1_idx_present_flag)
      {
        this->rpl_idx[i] = this->rpl_idx[0];
      }
    }
    else
    {
      this->ref_pic_list_structs[i].parse(reader, i, sps->sps_num_ref_pic_lists[i], sps);
    }

    this->RplsIdx[i] = this->rpl_sps_flag[i] ? this->rpl_idx[i] : sps->sps_num_ref_pic_lists[i];
    reader.logCalculatedValue(formatArray("RplsIdx", i), this->RplsIdx[i]);

    auto rpl = this->getActiveRefPixList(sps, i);

    for (unsigned j = 0; j < rpl.NumLtrpEntries; j++)
    {
      if (rpl.ltrp_in_header_flag)
      {
        auto nrBits      = sps->sps_log2_max_pic_order_cnt_lsb_minus4 + 4;
        this->poc_lsb_lt = reader.readBits(formatArray("poc_lsb_lt", i), nrBits);
      }
      this->delta_poc_msb_cycle_present_flag[i].push_back(
          reader.readFlag(formatArray("delta_poc_msb_cycle_present_flag", i)));
      if (this->delta_poc_msb_cycle_present_flag[i][j])
      {
        this->delta_poc_msb_cycle_lt[i][j] =
            reader.readUEV(formatArray("delta_poc_msb_cycle_lt", i, j));
      }
    }
  }
}

ref_pic_list_struct ref_pic_lists::getActiveRefPixList(std::shared_ptr<seq_parameter_set_rbsp> sps,
                                                       unsigned listIndex) const
{
  return (this->rpl_sps_flag.at(listIndex)
              ? sps->ref_pic_list_structs[listIndex][this->rpl_idx.at(listIndex)]
              : this->ref_pic_list_structs[listIndex]);
}

} // namespace parser::vvc
