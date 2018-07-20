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

#include <sstream>
#include <ch-cpp-utils/third-party/json/json.hpp>

#include "motion-detector.hpp"

#include <glog/logging.h>

using std::ostringstream;
using nlohmann::json;

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

void FileProcessingCtxt::initBuffers() {
	// openCV pixel format
	AVPixelFormat pFormat = AV_PIX_FMT_BGR24;
	// Data size
	int numBytes = avpicture_get_size(pFormat, avctx->width, avctx->height);
	// allocate buffer
	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	// fill frame structure
	avpicture_fill((AVPicture *) pFrameRGB, buffer, pFormat, avctx->width, avctx->height);
}

FileProcessingCtxt::FileProcessingCtxt(string &filename) {
   LOG(INFO) << "FileProcessingCtxt | filename: " << filename;
   this->filename = filename;
   frameCount = 1;
   nextFrameIndex = 1;
   ifmt_ctx = nullptr;
   video_stream_index = -1;
	avctx = nullptr;
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

   initFfmpeg();
   initBuffers();
}

FileProcessingCtxt::~FileProcessingCtxt() {
	av_free(buffer);
	av_free(pFrame);
	av_free(pFrameRGB);

   avformat_close_input(&ifmt_ctx);
}

bool FileProcessingCtxt::initFfmpeg() {
	int ret;

   const char *file = filename.data();

   // Initialize FFMPEG
   av_register_all();
   // Get input file format context
   if ((ret = avformat_open_input(&ifmt_ctx, file, nullptr, nullptr)) < 0)
   {
      LOG(ERROR) << "Could not open input file: " << filename;
      return false;
   }

   // Extract streams description
   if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0)
   {
      LOG(ERROR) << "Failed to retrieve input stream information";
      return false;
   }
   // Print detailed information about the input or output format,
   // such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base.
   av_dump_format(ifmt_ctx, 0, file, 0);

	AVCodec *in_codec = nullptr;
   for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
      if (ifmt_ctx->streams[i]->codec->coder_type == AVMEDIA_TYPE_VIDEO) {
         video_stream_index = (int) i;
         avctx = ifmt_ctx->streams[i]->codec;
         in_codec = avcodec_find_decoder(avctx->codec_id);
         if (!in_codec) {
            LOG(ERROR) << "in codec not found";
            exit(1);
         }
         LOG(INFO)<< "Codec found: " << in_codec->long_name;
         break;
      }
   }

	// Open input codec
	avcodec_open2(avctx, in_codec, NULL);
   return true;
}

void FileProcessingCtxt::processFrame(bool lastFrame) {
	if(frameCount == nextFrameIndex || lastFrame) {
		struct SwsContext *img_convert_ctx = sws_getCachedContext(NULL,
				avctx->width,
				avctx->height, avctx->pix_fmt, avctx->width,
				avctx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

		sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
            avctx->height, pFrameRGB->data, pFrameRGB->linesize);

		sws_freeContext(img_convert_ctx);

		Mat frame(pFrame->height, pFrame->width, CV_8UC3,
				pFrameRGB->data[0], pFrameRGB->linesize[0]);

		 if(lastFrame) frameCount--;

		LOG(INFO)<< "+++++Frame: " << frameCount;
		 bool skip = nextFrame(frame, frameCount, lastFrame, pFrameRGB->linesize[0], this_);
		 LOG(INFO)<< "-----Frame: " << frameCount;

		 nextFrameIndex = skip ? nextFrameIndex * 2 : nextFrameIndex + 1;
	}
   frameCount++;
}

void FileProcessingCtxt::process(NextFrame nextFrame, void *this_) {
   this->nextFrame = nextFrame;
   this->this_ = this_;

	int frameFinished = 0;
	AVPacket pkt;

	// Main loop
	while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index != video_stream_index) {
         av_packet_unref(&pkt);
         continue;
      }

      avcodec_decode_video2(avctx, pFrame, &frameFinished, &pkt);
      if (frameFinished) {
         processFrame();
      }
      // Decrease packet ref counter
      av_packet_unref(&pkt);
	}
	processFrame(true);
}

UploadContext::UploadContext(MotionDetectorCtxt *ctxt, HttpRequest *request) {
	this->context = ctxt;
	this->request = request;
	buffer = nullptr;
	length=0;
}

UploadContext::~UploadContext() {

}

uint8_t* UploadContext::getBuffer() {
   return buffer;
}

void UploadContext::setBuffer(uint8_t* buffer) {
   this->buffer = buffer;
}

uint32_t UploadContext::getLength() {
   return length;
}

void UploadContext::setLength(uint32_t length) {
   this->length = length;
}

MotionDetectorCtxt* UploadContext::getContext() {
   return context;
}

HttpRequest* UploadContext::getRequest() {
   return request;
}

string& UploadContext::getUrl() {
   return url;
}

void UploadContext::setUrl(string url) {
   this->url = url;
}

void UploadContext::wait() {
   LOG(INFO) << "Waiting for upload done: " << url;
   mFinished.wait();
   LOG(INFO) << "Upload done..." << url;
}

void UploadContext::done() {
   mFinished.notify();
   LOG(INFO) << "Notified upload done." << url;
}

MotionDetectorCtxt::MotionDetectorCtxt(Config *config, string &filename) {
   this->mConfig = config;
   this->filename = filename;
   detected = false;

	if(mConfig->getMDRender()) {
		namedWindow(windowName, WINDOW_AUTOSIZE);
		namedWindow(firstFrameWindow, WINDOW_AUTOSIZE);
		namedWindow(thresholdFrameWindow, WINDOW_AUTOSIZE);
		namedWindow(diffFrameWindow, WINDOW_AUTOSIZE);
	}
#ifdef ENABLE_TENSORFLOW
	mLabelImage=nullptr;
#endif

   mFPCtxt = new FileProcessingCtxt(filename);
	mMinArea = mConfig->getMDMinArea();
}

MotionDetectorCtxt::~MotionDetectorCtxt() {
	LOG(INFO) << "*****************~MotionDetectorCtxt";
	freeFinishedUploads();
   delete mFPCtxt;
	if(mConfig->getMDRender()) {
		destroyAllWindows();
	}
}

#ifdef ENABLE_TENSORFLOW
bool MotionDetectorCtxt::detectTf(Mat &frame) {
   unordered_map<string,float> labels = mLabelImage->processFrame(frame);
	auto motion = labels.find("motion");
	auto nmotion = labels.find("nmotion");
	LOG(INFO) << "motion: " << motion->second << ", nmotion: " << nmotion->second;
	bool detected = false;
	if(motion->second > nmotion->second) {
		detected = true;
	}
   return detected;
}
#endif

void MotionDetectorCtxt::render(const String& winname, InputArray mat) {
	if(mConfig->getMDRender()) {
		imshow(winname, mat);
	}
}

bool MotionDetectorCtxt::detectCv(Mat &frame) {
	Mat frameDelta;
	Mat thres;
	Mat capturedImageGrayFrame;
	std::vector<std::vector<cv::Point> > contours;
	Point offset;

	cvtColor (frame, capturedImageGrayFrame, CV_RGB2GRAY);

	GaussianBlur(capturedImageGrayFrame, capturedImageGrayFrame, Size( 21, 21 ), 0, BORDER_DEFAULT);

	if(0 == firstFrame.total()) {
		firstFrame = capturedImageGrayFrame.clone();
		LOG(INFO) << "First fame size: " << firstFrame.total();
		// render(firstFrameWindow, frame);
		return false;
	}

	absdiff(firstFrame, capturedImageGrayFrame, frameDelta);
	// render(diffFrameWindow, frameDelta);

	threshold(frameDelta, thres, 25, 255, THRESH_BINARY);

	// render(thresholdFrameWindow, thres);

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
		rectangle(frame, rect, Scalar(255, 255, 0), 2);
      detected = true;
	}

	render(windowName, frame);
	if(mConfig->getMDRender()) {
		waitKey((int) (0 == mConfig->getMDRenderDelay() ? 1 : mConfig->getMDRenderDelay()));
	}
   return detected;
}

void MotionDetectorCtxt::freeFinishedUploads() {
   lock_guard <mutex> lock (mFinishedUploadsMutex);

   while(mFinishedUploads.size()) {
      UploadContext *context = mFinishedUploads.front();
      context->wait();
      string url = context->getUrl();
      HttpRequest *request = context->getRequest();
      uint8_t *buffer = context->getBuffer();
      LOG(INFO) << "Delayed freeing of upload context: " << url;
      delete request;
      delete context;
      free(buffer);
      mFinishedUploads.erase(mFinishedUploads.begin());
   }
}

void MotionDetectorCtxt::_onLoad(HttpRequestLoadEvent *event, void *this_) {
	UploadContext *context = (UploadContext *) this_;
	MotionDetectorCtxt *md = context->getContext();
	md->onLoad(event, context);
}

void MotionDetectorCtxt::onLoad(HttpRequestLoadEvent *event, UploadContext *context) {
	string url = context->getUrl();
	LOG(INFO) << "Request Complete. Adding context to finished list: " << url;
	context->done();
}

void MotionDetectorCtxt::saveFrame(Mat &frame, string &filename, uint32_t index, bool lastFrame, uint32_t linesize) {
   ostringstream os;

   string file = filename.substr(filename.find_last_of("/") + 1);
   LOG(INFO) << "File from path: " << file;

   os << file << "." << index << ".jpg";
   LOG(INFO) << "Saving frame " << index << " from file: " << os.str();

   ostringstream os1;
	os1 << "http://127.0.0.1:8889/images/" << os.str() << "?width="
			<< frame.cols << "&height=" << frame.rows << "&linesize="
			<< linesize;

   /*
    * curl -i -XPOST
    * 	--data-binary @Tue-2018-04-03-08-40-06-PDT-1522770006018966474.ts.1.bin
    * 	"http://127.0.0.1:8889/images/i.jpg?width=848&height=480&linesize=2544"
    */
   HttpRequest *request = new HttpRequest();

   UploadContext *upload = new UploadContext(this, request);
   size_t length = frame.cols * frame.rows * frame.channels() * frame.elemSize1();
   void *buffer = malloc(length);
   memmove(buffer, frame.data, length);
   upload->setBuffer((uint8_t *) buffer);
   upload->setLength(length);
   upload->setUrl(os1.str());

   request->onLoad(MotionDetectorCtxt::_onLoad).bind(upload);
   request->open(EVHTTP_REQ_POST, upload->getUrl()).send(
		   (void *) upload->getBuffer(), length);
   mFinishedUploads.push_back(upload);
}

bool MotionDetectorCtxt::_nextFrame(Mat &frame, uint32_t index, bool lastFrame, uint32_t linesize, void *this_) {
   MotionDetectorCtxt *ctxt = (MotionDetectorCtxt *) this_;
   return ctxt->nextFrame(frame, index, lastFrame, linesize);
}


bool MotionDetectorCtxt::nextFrame(Mat &frame, uint32_t index, bool lastFrame, uint32_t linesize) {
	detected = detectCv(frame);
   if (detected) {
      detectedFrames.push_back(frame);
      if(detectedFrames.size() >= 2) {
         detectedFrames.erase(detectedFrames.begin());
      }
   } else {
      LOG(INFO) << "No Motion Detected by OpenCV.";
   }

   if(detectedFrames.size()) {
      LOG(INFO) << "Detected by OpenCV. Running it through TF. Frames: " <<
         detectedFrames.size();
#ifdef ENABLE_TENSORFLOW
    	detected = detectTf(detectedFrames.at(0));
#endif
      if(detected) {
         saveFrame(frame, filename, index, lastFrame, linesize);
      }
   }

	return true;
}

void MotionDetectorCtxt::process() {
   mFPCtxt->process(MotionDetectorCtxt::_nextFrame, this);
}

bool MotionDetectorCtxt::motionDetected() {
   return detected;
}

#ifdef ENABLE_TENSORFLOW
void MotionDetectorCtxt::setLabelImage(LabelImage *labelImage) {
	mLabelImage = labelImage;
}
#endif

MotionDetectorThread::MotionDetectorThread(Config *config) {
	mConfig = config;

#ifdef ENABLE_TENSORFLOW
   mLabelImage = new LabelImage(config);
   mLabelImage->init();
#endif
}

MotionDetectorThread::~MotionDetectorThread() {
	LOG(INFO) << "*****************~MotionDetectorThread";
#ifdef ENABLE_TENSORFLOW
	delete mLabelImage;
#endif
}

void MotionDetectorThread::processJob(MotionDetectorJob *data) {
   MotionDetectorCtxt *mdCtxt = new MotionDetectorCtxt(mConfig,
         data->mFilename);
#ifdef ENABLE_TENSORFLOW
	mdCtxt->setLabelImage(mLabelImage);
#endif

   mdCtxt->process();
   bool detected = mdCtxt->motionDetected();
   if(detected) {
		LOG(INFO) << pthread_self() << " Motion Detected: " << data->mFilename;
      data->notify();
   } else {
		LOG(INFO) << pthread_self() << " No Motion Detected: " << data->mFilename;
   }
   delete mdCtxt;
}

MotionDetector::MotionDetector(Config *config) {
	LOG(INFO) << "*****************MotionDetector";
	mConfig = config;
	mMailClient = new MailClient(config);
	mKafkaClient = new KafkaClient(config);
	mKafkaClient->init();
	mPool = new ThreadPool(mConfig->getMDThreadCount(), false);
	LOG(INFO) << "Motion detector thread pool ready to process!";
}

MotionDetector::MotionDetector(Config *config, KafkaClient *kafkaClient) {
	LOG(INFO) << "*****************MotionDetector";
	mConfig = config;
	mMailClient = new MailClient(config);
	mKafkaClient = kafkaClient;
	mPool = new ThreadPool(mConfig->getMDThreadCount(), false);
	LOG(INFO) << "Motion detector thread pool ready to process!";
}

MotionDetector::~MotionDetector() {
	LOG(INFO) << "*****************~MotionDetector";
	LOG(INFO) << "*****************~MotionDetector deleting pool";
	for(auto &entry : mPoolCtxt) {
		MotionDetectorThread *ctxt = entry.second;
		delete ctxt;
	}
	delete mPool;
	delete mKafkaClient;
	delete mMailClient;
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

	// MotionDetectorThread *ctxt = new MotionDetectorThread(mConfig);

	Mat frame;
	while (true) {
		cameraCapture >> frame;
		// ctxt->detect(frame);
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

	delete data;
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

	json j;
	j["filename"] = filename;
	mKafkaClient->send(j);
}

void MotionDetector::initiateCameraCapture()
{
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
        goto CLEAN_RETURN;
	}

	/*
	 * Set the desired FPS.
	 */
	bStatus = cameraCapture.set(CV_CAP_PROP_FPS, mConfig->getCameraFps());
	if (false == bStatus)
	{
		goto CLEAN_RETURN;
	}

	bStatus = cameraCapture.set (CV_CAP_PROP_FRAME_WIDTH,
			mConfig->getCameraWidth());
	if (false == bStatus)
	{
		goto CLEAN_RETURN;
	}

    bStatus = cameraCapture.set (CV_CAP_PROP_FRAME_HEIGHT,
		mConfig->getCameraHeight());
	if (bStatus)
	{
		LOG(INFO) << "Camera Device Initialized!";
	}
CLEAN_RETURN:
	return;
}


} // End namespace SS.

