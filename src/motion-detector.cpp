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
 * \file   motion-detector.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Feb 26, 2018
 *
 * \brief
 *
 ******************************************************************************/
#include <glog/logging.h>

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "motion-detector.hpp"

namespace SS {

const string windowName = "Motion Detector";
const string firstFrameWindow = "First Frame";
const string thresholdFrameWindow = "Threshold Frame";
const string diffFrameWindow = "Difference Frame";

MotionDetectorJob::MotionDetectorJob (MotionDetector *md, string &filename) {
	mMD = md;
	mFilename = filename;
}

MotionDetectorJob::~MotionDetectorJob () {
}

void MotionDetectorJob::notify() {
	mMD->notify(mFilename);
}

MotionDetectorThread::MotionDetectorThread(Config *config) {
	mConfig = config;

	mMinArea = mConfig->getMDMinArea();
}

MotionDetectorThread::~MotionDetectorThread() {

}

void MotionDetectorThread::render(const String& winname, InputArray mat) {
	if(mConfig->getMDRender()) {
		imshow(winname, mat);
	}
}

bool MotionDetectorThread::detect(const string &filename) {
	Mat frameDelta;
	Mat thres;
	std::vector<std::vector<cv::Point> > contours;
	Point offset;

	cvtColor (capturedImageRgbFrame, capturedImageGrayFrame, CV_RGB2GRAY);

	GaussianBlur(capturedImageGrayFrame, capturedImageGrayFrame, Size( 21, 21 ), 0, BORDER_DEFAULT);

//		LOG(INFO) << "First fame size: " << firstFrame.total();
	if(0 == firstFrame.total()) {
		firstFrame = capturedImageGrayFrame.clone();
		LOG(INFO) << "First fame size: " << firstFrame.total();
		render(firstFrameWindow, capturedImageRgbFrame);
		return false;
	}

	absdiff(firstFrame, capturedImageGrayFrame, frameDelta);
	render(diffFrameWindow, frameDelta);

	threshold(frameDelta, thres, 25, 255, THRESH_BINARY);

	render(thresholdFrameWindow, thres);

	dilate(thres, thres, noArray(), Point(-1,-1), 2);
	findContours(thres.clone(), contours, RETR_EXTERNAL,
			CHAIN_APPROX_SIMPLE, offset);

//	LOG(INFO) << "Contours size: " << contours.size();
   bool detected = false;
	for (size_t idx = 0; idx < contours.size(); idx++) {
		double area = contourArea(contours[idx]);
		if(area < mMinArea) {
			// LOG(INFO) << pthread_self() << " Motion Not Detected; Area: " << area;
			continue;
		}
		// LOG(INFO) << pthread_self() << " Motion Detected; Area: " << area;
		Rect rect = boundingRect(contours[idx]);
		rectangle(capturedImageRgbFrame, rect, Scalar(255, 255, 0), 2);
      detected = true;
	}

	render(windowName, capturedImageRgbFrame);
	if(mConfig->getMDRender()) {
		waitKey((int) (0 == mConfig->getMDRenderDelay() ? 1 : mConfig->getMDRenderDelay()));
	}

   return detected;
}

void MotionDetectorThread::detect(Mat &frame) {
	capturedImageRgbFrame = frame;
   const string filename = "";
	detect(filename);
}

void MotionDetectorThread::processJob(MotionDetectorJob *data) {
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket pkt;
	AVFrame *pFrame = NULL;
	AVFrame *pFrameRGB = NULL;
	int frameFinished = 0;
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

	const string filename = data->mFilename;

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

	if(mConfig->getMDRender()) {
		namedWindow(windowName, WINDOW_AUTOSIZE);
		namedWindow(firstFrameWindow, WINDOW_AUTOSIZE);
		namedWindow(thresholdFrameWindow, WINDOW_AUTOSIZE);
		namedWindow(diffFrameWindow, WINDOW_AUTOSIZE);
	}
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
				capturedImageRgbFrame = img;

				// LOG(INFO)<< "+++++Frame: " << frame;
				bool motion = detect(filename);
				if (motion) {
					detected = true;
				}
				// LOG(INFO)<< "-----Frame: " << frame;
				frame++;
			}
		}
		// Decrease packet ref counter
		av_packet_unref(&pkt);
	}
	if(mConfig->getMDRender()) {
		destroyAllWindows();
	}

   if(detected) {
		LOG(INFO) << pthread_self() << " Motion Detected: " << filename;
      data->notify();
   } else {
		LOG(INFO) << pthread_self() << " No Motion Detected: " << filename;
   }

	avformat_close_input(&ifmt_ctx);
//	avcodec_close(avctx);
	av_free(pFrame);
	av_free(pFrameRGB);
}

MotionDetector::MotionDetector(Config *config) {
	mConfig = config;
	mMailClient = new MailClient(config);
	mPool = new ThreadPool(mConfig->getMDThreadCount(), false);
	LOG(INFO) << "Motion detector thread pool ready to process!";
}

MotionDetector::~MotionDetector() {
	LOG(INFO) << "*****************~MotionDetector";
	LOG(INFO) << "*****************~MotionDetector deleting pool";
	delete mPool;
}

void MotionDetector::init() {
	namedWindow(windowName, WINDOW_AUTOSIZE);
	initiateCameraCapture();
}

void MotionDetector::start() {
	Mat frameDelta;
	Mat thres;
	std::vector<std::vector<cv::Point> > contours;
	Point offset;

	MotionDetectorThread *ctxt = new MotionDetectorThread(mConfig);

	Mat frame;
	while (true) {
		cameraCapture >> frame;
		ctxt->detect(frame);
	}
}

void MotionDetector::deinit() {
	if(cameraCapture.isOpened()) {
		cameraCapture.release();
		LOG(INFO) << "Camera Device De-Initialized!";
	}

	destroyWindow(windowName);
}

void *MotionDetector::_routine(void *arg, struct event_base *base) {
	MotionDetectorJob *data = (MotionDetectorJob *) arg;
	MotionDetector *md = data->mMD;
	return md->routine(data);
}

void *MotionDetector::routine(MotionDetectorJob *data) {
	LOG(INFO) << "Motion detector job running";

	MotionDetectorThread *ctxt = NULL;
	pthread_t id = pthread_self();
	auto search = mPoolCtxt.find(id);
	if(search == mPoolCtxt.end() || NULL == search->second) {
		LOG(INFO) << "Creating thread context for: " << id;
		ctxt = new MotionDetectorThread(mConfig);
		mPoolCtxt.insert(make_pair(id, ctxt));
	} else {
		LOG(INFO) << "Using existing thread context for: " << id;
		ctxt = search->second;
	}

	ctxt->processJob(data);
	return NULL;
}

void MotionDetector::process(string &filename) {
	MotionDetectorJob *data = new MotionDetectorJob(this, filename);
	ThreadJob *job = new ThreadJob(MotionDetector::_routine, data);
	LOG(INFO) << "New motion detector job created";
	mPool->addJob(job);
}

void MotionDetector::notify(string &filename) {
	mMailClient->notifyMotionDetection(filename);
}

void MotionDetector::initiateCameraCapture()
{
	int i_ret_val = -1;

	bool bStatus = false;

	/**************************************************************************
	 * Start Video Capture
	 *************************************************************************/
	/*
	 * Open the camera device.
	 */
    cameraCapture = VideoCapture(mConfig->getCameraDevice());
	if(false == cameraCapture.isOpened())
	{
		LOG(ERROR) << "Camera Device Open Failed!";
		i_ret_val = -1;
        goto CLEAN_RETURN;
	}

	/*
	 * Set the desired FPS.
	 */
	bStatus = cameraCapture.set(CV_CAP_PROP_FPS, mConfig->getCameraFps());
	if (false == bStatus)
	{
		i_ret_val = -1;
		goto CLEAN_RETURN;
	}

	bStatus = cameraCapture.set (CV_CAP_PROP_FRAME_WIDTH,
			mConfig->getCameraWidth());
	if (false == bStatus)
	{
		i_ret_val = -1;
		goto CLEAN_RETURN;
	}

    bStatus = cameraCapture.set (CV_CAP_PROP_FRAME_HEIGHT,
		mConfig->getCameraHeight());
	if (false == bStatus)
	{
		i_ret_val = -1;
	}
	else
	{
		LOG(INFO) << "Camera Device Initialized!";
		i_ret_val = 0;
	}
CLEAN_RETURN:
	return;
}


} // End namespace SS.

