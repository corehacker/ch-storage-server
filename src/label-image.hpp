/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2018, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2018, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   label-image.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Feb 15, 2018
 *
 * \brief
 *
 ******************************************************************************/

#include <fstream>
#include <utility>
#include <vector>
#include <unordered_map>

#include <opencv2/opencv.hpp>

// #include "tensorflow/cc/ops/const_op.h"
// #include "tensorflow/cc/ops/image_ops.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

#include "config.hpp"

using namespace cv;
using std::string;
using std::unordered_map;

// These are all common classes it's handy to reference with no namespace.
using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::Session;
using tensorflow::SessionOptions;
using tensorflow::string;
using tensorflow::int32;
using tensorflow::GraphDef;

#ifndef SRC_LABEL_IMAGE_HPP_
#define SRC_LABEL_IMAGE_HPP_

namespace SS {

class LabelImage {
private:
  Config *mConfig;
  std::unique_ptr<tensorflow::Session> session;
  string root;
  string graph;
  int32 input_width;
  int32 input_height;
  float input_mean;
  float input_std;
  string input_layer;
  string output_layer;
  string labels;

  Status LoadGraph(const string& graph_file_name,
        std::unique_ptr<tensorflow::Session>* session);
  Status ReadLabelsFile(const string& file_name,
        std::vector<string>* result,
        size_t* found_label_count);
  Status GetTopLabels(const std::vector<Tensor>& outputs,
        int how_many_labels,
        Tensor* indices, Tensor* scores);
  Status PrintTopLabels(
        const std::vector<Tensor>& outputs,
        const string& labels_file_name,
        unordered_map<string,float> &labelMap);
  Status runSession(Tensor &input,
        std::vector<Tensor>* out_tensors);
public:
  LabelImage(Config *config);
  ~LabelImage();
  int init();
  unordered_map<string,float> processFrame(Mat frame);
};

} // End namespace SS.


#endif /* SRC_LABEL_IMAGE_HPP_ */

