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
 * \file   label-image.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Feb 15, 2018
 *
 * \brief
 *
 ******************************************************************************/

#include <ch-cpp-utils/utils.hpp>
#include "label-image.hpp"
#include "tensorflow/cc/ops/standard_ops.h"
#include <glog/logging.h>

using ChCppUtils::getEpochNano;



namespace SS {

LabelImage::LabelImage(Config *config) {
  mConfig = config;
  graph = mConfig->getMDTfGraph();
  labels = mConfig->getMDTfLabels();
  input_width = mConfig->getMDTfInputWidth();
  input_height = mConfig->getMDTfInputHeight();
  input_mean = mConfig->getMDTfInputMean();
  input_std = mConfig->getMDTfInputStd();
  input_layer = mConfig->getMDTfInputLayer();
  output_layer = mConfig->getMDTfOutputLayer();
}

LabelImage::~LabelImage() {
}

// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status LabelImage::LoadGraph(const string& graph_file_name,
                 std::unique_ptr<Session>* session) {
  tensorflow::GraphDef graph_def;
  Status load_graph_status =
      ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok()) {
    return tensorflow::errors::NotFound("Failed to load compute graph at '",
                                        graph_file_name, "'");
  }
  session->reset(tensorflow::NewSession(SessionOptions()));
  Status session_create_status = (*session)->Create(graph_def);
  if (!session_create_status.ok()) {
    return session_create_status;
  }
  return Status::OK();
}

int LabelImage::init() {
  // First we load and initialize the model.
  Status load_graph_status = LoadGraph(graph, &session);
  if (!load_graph_status.ok()) {
    LOG(ERROR) << load_graph_status;
    return -1;
  }
  LOG(INFO) << "LoadGraph success";
  return 0;
}

// Takes a file name, and loads a list of labels from it, one per line, and
// returns a vector of the strings. It pads with empty strings so the length
// of the result is a multiple of 16, because our model expects that.
Status LabelImage::ReadLabelsFile(const string& file_name, std::vector<string>* result,
                      size_t* found_label_count) {
  std::ifstream file(file_name);
  if (!file) {
    return tensorflow::errors::NotFound("Labels file ", file_name,
                                        " not found.");
  }
  result->clear();
  string line;
  while (std::getline(file, line)) {
    result->push_back(line);
  }
  *found_label_count = result->size();
  const int padding = 16;
  while (result->size() % padding) {
    result->emplace_back();
  }
  return Status::OK();
}

// Analyzes the output of the Inception graph to retrieve the highest scores and
// their positions in the tensor, which correspond to categories.
Status LabelImage::GetTopLabels(const std::vector<Tensor>& outputs,
				int how_many_labels, Tensor* indices, Tensor* scores) {
  auto root = tensorflow::Scope::NewRootScope();
  using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

  string output_name = "top_k";
  TopK(root.WithOpName(output_name), outputs[0], how_many_labels);
  // This runs the GraphDef network definition that we've just constructed, and
  // returns the results in the output tensors.
  tensorflow::GraphDef graph;
  TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

  std::unique_ptr<Session> session(tensorflow::NewSession(SessionOptions()));
  TF_RETURN_IF_ERROR(session->Create(graph));
  // The TopK node returns two outputs, the scores and their original indices,
  // so we have to append :0 and :1 to specify them both.
  std::vector<Tensor> out_tensors;
  TF_RETURN_IF_ERROR(session->Run({}, {output_name + ":0", output_name + ":1"},
                                  {}, &out_tensors));
  *scores = out_tensors[0];
  *indices = out_tensors[1];
  return Status::OK();
}

// Given the output of a model run, and the name of a file containing the labels
// this prints out the top five highest-scoring values.
Status LabelImage::PrintTopLabels(
                      const std::vector<Tensor>& outputs,
                      const string& labels_file_name,
                      unordered_map<string,float> &labelMap) {
  std::vector<string> labels;
  size_t label_count;
  Status read_labels_status =
      ReadLabelsFile(labels_file_name, &labels, &label_count);
  if (!read_labels_status.ok()) {
    LOG(ERROR) << read_labels_status;
    return read_labels_status;
  }
  const int how_many_labels = std::min(5, static_cast<int>(label_count));
  Tensor indices;
  Tensor scores;
  TF_RETURN_IF_ERROR(GetTopLabels(outputs, how_many_labels, &indices, &scores));
  tensorflow::TTypes<float>::Flat scores_flat = scores.flat<float>();
  tensorflow::TTypes<int32>::Flat indices_flat = indices.flat<int32>();
  for (int pos = 0; pos < how_many_labels; ++pos) {
    const int label_index = indices_flat(pos);
    const float score = scores_flat(pos);
    // LOG(INFO) << labels[label_index] << " (" << label_index << "): " << score;

    labelMap.insert(std::make_pair(labels[label_index], score));
  }
  return Status::OK();
}

// Given an image file name, read in the data, try to decode it as an image,
// resize it to the requested size, and then scale the values as desired.
Status LabelImage::runSession(Tensor &input,
      std::vector<Tensor>* out_tensors) {
  std::vector<std::pair<string, Tensor>> inputs = {
      {input_layer, input},
  };

  TF_RETURN_IF_ERROR(session->Run({inputs}, {output_layer}, {}, out_tensors));
  return Status::OK();
}

unordered_map<string,float> LabelImage::processFrame(Mat frame) {
   Size s(input_height, input_width);
   Mat resizedImage;
   resize(frame, resizedImage, s, 0, 0, INTER_CUBIC);

   int depth = resizedImage.channels();

   // LOG(INFO) << "height=" << input_height << " / width=" << input_width <<
   //    " / depth=" << depth;

   // creating a Tensor for storing the data
   tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT,
         tensorflow::TensorShape({1, input_height, input_width, depth}));
   auto input_tensor_mapped = input_tensor.tensor<float, 4>();

   cv::Mat resizedImage2;
   resizedImage.convertTo(resizedImage2, CV_32FC1);
   resizedImage = resizedImage2;
   resizedImage = resizedImage - input_mean;
   resizedImage = resizedImage / input_std;
   const float * source_data = (float*) resizedImage.data;

   // copying the data into the corresponding tensor
   for (int y = 0; y < input_height; ++y) {
      const float* source_row = source_data + (y * input_width * depth);
      for (int x = 0; x < input_width; ++x) {
         const float* source_pixel = source_row + (x * depth);
         for (int c = 0; c < depth; ++c) {
            const float* source_value = source_pixel + c;
            input_tensor_mapped(0, y, x, c) = *source_value;
         }
      }
   }

   uint64_t start = getEpochNano();
   // running the loaded graph
   std::vector<Tensor> finalOutput;
   runSession(input_tensor, &finalOutput);
   uint64_t end = getEpochNano();
   uint64_t elapsed = end - start;
   elapsed = elapsed / (1000 * 1000);
   LOG(INFO) << "[" << elapsed  << "ms] Final output size=" <<
      finalOutput.size();

   unordered_map<string, float> labelsMap;
   PrintTopLabels(finalOutput, labels, labelsMap);
   return labelsMap;
}

} // End namespace SS.

