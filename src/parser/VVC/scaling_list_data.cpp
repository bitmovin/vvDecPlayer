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

#include "scaling_list_data.h"

#include "adaptation_parameter_set_rbsp.h"
#include "common/Typedef.h"

namespace parser::vvc
{

namespace
{

vector4d<int> calcDiagScanOrder()
{
  vector4d<int> DiagScanOrder;
  for (unsigned log2BlockWidth = 0; log2BlockWidth < 4; log2BlockWidth++)
  {
    DiagScanOrder.push_back({});
    for (unsigned log2BlockHeight = 0; log2BlockHeight < 4; log2BlockHeight++)
    {
      DiagScanOrder[log2BlockWidth].push_back({});
      auto blkWidth  = 1u << log2BlockWidth;
      auto blkHeight = 1u << log2BlockHeight;
      // 6.5.2 (24)
      unsigned i        = 0;
      int      x        = 0;
      int      y        = 0;
      bool     stopLoop = false;
      while (!stopLoop)
      {
        while (y >= 0)
        {
          if (x < int(blkWidth) && y < int(blkHeight))
          {
            DiagScanOrder[log2BlockWidth][log2BlockHeight].push_back({x, y});
            i++;
          }
          y--;
          x++;
        }
        y = x;
        x = 0;
        if (i >= blkWidth * blkHeight)
          stopLoop = true;
      }
    }
  }
  return DiagScanOrder;
}

} // namespace

using namespace parser::reader;

void scaling_list_data::parse(SubByteReaderLogging &reader, adaptation_parameter_set_rbsp *aps)
{
  SubByteReaderLoggingSubLevel subLevel(reader, "scaling_list_data");

  auto DiagScanOrder = calcDiagScanOrder();

  for (unsigned id = 0; id < 28; id++)
  {
    auto matrixSize = id < 2 ? 2u : (id < 8 ? 4u : 8u);
    this->ScalingList.push_back({});
    if (aps->aps_chroma_present_flag || id % 3 == 2 || id == 27)
    {
      this->scaling_list_copy_mode_flag[id] = reader.readFlag("scaling_list_copy_mode_flag");
      if (!this->scaling_list_copy_mode_flag[id])
      {
        this->scaling_list_pred_mode_flag[id] = reader.readFlag("scaling_list_pred_mode_flag");
      }
      if ((this->scaling_list_copy_mode_flag[id] || this->scaling_list_pred_mode_flag[id]) &&
          id != 0 && id != 2 && id != 8)
      {
        this->scaling_list_pred_id_delta[id] = reader.readUEV("scaling_list_pred_id_delta");
      }
      if (!this->scaling_list_copy_mode_flag[id])
      {
        auto nextCoef = 0;
        if (id > 13)
        {
          this->scaling_list_dc_coef[id] = reader.readSEV("scaling_list_dc_coef");
          nextCoef += this->scaling_list_dc_coef[id - 14];
        }

        for (unsigned i = 0; i < matrixSize * matrixSize; i++)
        {
          auto x = DiagScanOrder[3][3][i][0];
          auto y = DiagScanOrder[3][3][i][1];
          if (!(id > 25 && x >= 4 && y >= 4))
          {
            this->scaling_list_delta_coef[id][i] = reader.readSEV("scaling_list_delta_coef");
            nextCoef += this->scaling_list_delta_coef[id][i];
          }
          this->ScalingList[id].push_back(nextCoef);
        }
      }
    }
    else
    {
      this->scaling_list_copy_mode_flag[id] = true;
    }
  }
}

} // namespace parser::vvc
