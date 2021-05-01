/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "nvdsinfer_custom_impl.h"

/* Assumes only one input layer "im_info" needs to be initialized */
bool NvDsInferInitializeInputLayers (std::vector<NvDsInferLayerInfo> const &inputLayersInfo,
        NvDsInferNetworkInfo const &networkInfo,
        unsigned int maxBatchSize)
{
  printf("NvDsInferLayerInfo length %lu\n", inputLayersInfo.size());
  printf("NvDsInferLayerInfo.layerName %s\n", inputLayersInfo[0].layerName);
  printf("NvDsInferLayerInfo.isInput %d\n", inputLayersInfo[0].isInput);
  printf("NvDsInferLayerInfo.layerName %s\n", inputLayersInfo[1].layerName);
  printf("NvDsInferLayerInfo.isInput %d\n", inputLayersInfo[1].isInput);
  printf("inputLayers[0].inferDims.numElements %d\n", inputLayersInfo[0].inferDims.numElements);
  printf("inputLayers[1].inferDims.numElements %d\n", inputLayersInfo[1].inferDims.numElements);
  
  float *imInfo = (float *) inputLayersInfo[0].buffer;
  for (unsigned int i = 0; i < maxBatchSize; i++) {
    printf("networkInfo.height %d\n", networkInfo.height);
    printf("networkInfo.width %d\n", networkInfo.width);
    for(unsigned int c = 0; c < 3; c++) {
      for(unsigned int w = 0; w < 448; w++) {
	for(unsigned int h = 0; h < 448; h++) {
	  imInfo[c*448*448+w*448+h] = 0.0;
	}
      }
    }
  }
  imInfo = (float *) inputLayersInfo[1].buffer;
  for (unsigned int i = 0; i < maxBatchSize; i++) {

    for(unsigned int c = 0; c < 3; c++) {
      for(unsigned int w = 0; w < 448; w++) {
        for(unsigned int h = 0; h < 448; h++) {
          imInfo[c*448*448+w*448+h] = 0.0;
        }
      }
    }
  }
  return true;
}

