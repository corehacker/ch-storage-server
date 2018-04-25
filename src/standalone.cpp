
#include <string>
#include <glog/logging.h>

#include <opencv2/opencv.hpp>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/tensor.h"

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

using namespace cv;
using std::string;

const string windowName = "Motion Detector";
// initializing the graph
tensorflow::GraphDef graph_def;

// Name of the folder in which inception graph is present
string graphFile = "tensorflow/examples/ch-storage-server/config/tf_files/retrained_graph.pb";
string labelfile = "tensorflow/examples/ch-storage-server/config/tf_files/retrained_labels.txt";

std::unique_ptr<tensorflow::Session> session_inception;

std::vector<std::string> gLabels;

void render(const String& winname, InputArray mat) {
   imshow(winname, mat);
}

bool init() {
   // Loading the graph to the given variable
   tensorflow::Status graphLoadedStatus = ReadBinaryProto(tensorflow::Env::Default(),graphFile,&graph_def);
   if (!graphLoadedStatus.ok()){
      LOG(ERROR) << graphLoadedStatus.ToString();
      return false;
   }

   // creating a session with the grap
   session_inception.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
   tensorflow::Status session_create_status = session_inception->Create(graph_def);
   if (!session_create_status.ok()){
      LOG(ERROR) << session_create_status.ToString();
      return false;
   }

   // Label File Name
   std::ifstream label(labelfile);
   std::string line;

   // sorting the file to find the top labels
   std::vector<std::pair<float,std::string>> sorted;

   for (unsigned int i =0; i<2 ;++i){
      std::getline(label,line);
      gLabels.emplace_back(line);
   }
   return true;
}

bool detect(Mat frame) {
	Mat frameDelta;
	Mat thres;
	std::vector<std::vector<cv::Point> > contours;
	Point offset;
   bool detected = false;

   int height = 299;
   int width = 299;
   int mean = 0;
   int std = 255;

   Size s(height,width);
   Mat Image;
   resize(frame, Image, s, 0, 0, INTER_CUBIC);

   int depth = Image.channels();

   LOG(INFO) << "height=" << height << " / width=" << width << " / depth=" << depth;

   // creating a Tensor for storing the data
   tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({1,height,width,depth}));
   auto input_tensor_mapped = input_tensor.tensor<float, 4>();

   cv::Mat Image2;
   Image.convertTo(Image2, CV_32FC1);
   Image = Image2;
   Image = Image-mean;
   Image = Image/std;
   const float * source_data = (float*) Image.data;

   // copying the data into the corresponding tensor
   for (int y = 0; y < height; ++y) {
      const float* source_row = source_data + (y * width * depth);
      for (int x = 0; x < width; ++x) {
         const float* source_pixel = source_row + (x * depth);
         for (int c = 0; c < depth; ++c) {
            const float* source_value = source_pixel + c;
            input_tensor_mapped(0, y, x, c) = *source_value;
         }
      }
   }

   LOG(INFO) << "[Start]";

   // running the loaded graph
   std::vector<tensorflow::Tensor> finalOutput;

   std::string InputName = "Mul";
   std::string OutputName = "final_result";
   tensorflow::Status run_status  = session_inception->Run({{InputName,input_tensor}},{OutputName},{},&finalOutput);

   // finding the labels for prediction
   LOG(INFO) << "[End] Final output size=" << finalOutput.size();
   tensorflow::Tensor output = std::move(finalOutput.at(0));
   auto scores = output.flat<float>();

   // sorting the file to find the top labels
   std::vector<std::pair<float,std::string>> sorted;
   for (unsigned int i =0; i<scores.size() ;++i){
      sorted.emplace_back(scores(i), gLabels[i]);
   }

   std::sort(sorted.begin(),sorted.end());
   std::reverse(sorted.begin(),sorted.end());
   for(unsigned int i =0 ; i< sorted.size(); ++i){
      LOG(INFO) << "Category " << sorted[i].second << " with probability "<< sorted[i].first;
   }
   return detected;
}

void processJob(string filename) {
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket pkt;
	AVFrame *pFrame = NULL;
	AVFrame *pFrameRGB = NULL;
	int frameFinished = 0;
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

	const char *in_filename;
	if(filename.length() > 0) {
		in_filename = filename.data();
	} else {
		return;
	}

	int ret;

	// Initialize FFMPEG
	av_register_all();
	// Get input file format context
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
	{
		fprintf(stderr, "Could not open input file '%s'", in_filename);
		return;
	}
	// Extract streams description
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
	{
		fprintf(stderr, "Failed to retrieve input stream information");
		return;
	}
	// Print detailed information about the input or output format,
	// such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base.
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	LOG(INFO) << "Processing input file: " << in_filename;

	// Search for input video codec info
	AVCodec *in_codec = nullptr;
	AVCodecContext* avctx = nullptr;

	int video_stream_index = -1;
	for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
		if (ifmt_ctx->streams[i]->codec->coder_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = (int) i;
			avctx = ifmt_ctx->streams[i]->codec;
			in_codec = avcodec_find_decoder(avctx->codec_id);
			if (!in_codec) {
				fprintf(stderr, "in codec not found\n");
				exit(1);
			}
			LOG(INFO)<< "Codec found: " << in_codec->long_name;
			break;
		}
	}

	// openCV pixel format
	AVPixelFormat pFormat = AV_PIX_FMT_RGB24;
	// Data size
	int numBytes = avpicture_get_size(pFormat, avctx->width, avctx->height);
	// allocate buffer
	uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	// fill frame structure
	avpicture_fill((AVPicture *)pFrameRGB, buffer, pFormat, avctx->width, avctx->height);
	// Open input codec
	avcodec_open2(avctx, in_codec, NULL);

   if(!init()) {
      return;
   }

   // namedWindow(windowName, WINDOW_AUTOSIZE);
   bool detected = false;
   uint32_t frame = 0;
	// Main loop
	while (1) {
		AVStream *in_stream;
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0) {
			break;
		}
		in_stream = ifmt_ctx->streams[pkt.stream_index];

		if (pkt.stream_index == video_stream_index) {
			avcodec_decode_video2(avctx, pFrame, &frameFinished, &pkt);
			if (frameFinished) {
				struct SwsContext *img_convert_ctx;
				img_convert_ctx = sws_getCachedContext(NULL, avctx->width,
						avctx->height, avctx->pix_fmt, avctx->width,
						avctx->height, AV_PIX_FMT_BGR24,
						SWS_BICUBIC,
						NULL,
						NULL,
						NULL);
				sws_scale(img_convert_ctx, ((AVPicture*) pFrame)->data,
						((AVPicture*) pFrame)->linesize, 0, avctx->height,
						((AVPicture *) pFrameRGB)->data,
						((AVPicture *) pFrameRGB)->linesize);

				sws_freeContext(img_convert_ctx);
				Mat img(pFrame->height, pFrame->width, CV_8UC3,
						pFrameRGB->data[0], false);
            // render(windowName, img);
            // waitKey(1);

				LOG(INFO)<< "+++++Frame: " << frame;
				bool motion = detect(img);
				if (motion) {
					detected = true;
				}
				LOG(INFO)<< "-----Frame: " << frame;
            LOG(INFO) << "";
				frame++;
			}
		}
		// Decrease packet ref counter
		av_packet_unref(&pkt);
	}
   destroyAllWindows();

   if(detected) {
		LOG(INFO) << pthread_self() << " Motion Detected: " << filename;
   } else {
		LOG(INFO) << pthread_self() << " No Motion Detected: " << filename;
   }

	avformat_close_input(&ifmt_ctx);
//	avcodec_close(avctx);
	av_free(pFrame);
	av_free(pFrameRGB);
}

int main(int argc, char **argv) {
   if(argc == 2) {
      string filename = argv[1];
      processJob(filename);
      return 0;
   } else {
      LOG(ERROR) << "Usage: a.out <TS Filename>";
      return -1;
   }
}
