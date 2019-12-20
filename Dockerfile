FROM tensorflow/tensorflow:1.15.0

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cmake \
    libtiff5-dev libavcodec-dev libavformat-dev libswscale-dev libxine2-dev libv4l-dev \
    libgtk2.0-dev \
    libtbb-dev libatlas-base-dev libfaac-dev libmp3lame-dev libtheora-dev libvorbis-dev \
    libxvidcore-dev libopencore-amrnb-dev libopencore-amrwb-dev x264 v4l-utils \
    libcurl4-gnutls-dev ffmpeg libavresample-dev libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-base1.0-dev gphoto2 libgphoto2* jasper doxygen qtbase5-dev \
    pylint flake8  vtk7 autoconf automake libtool git wget curl && mkdir -p /deps && cd /deps

RUN mkdir -p opencv && \ 
    cd opencv && \
    git clone https://github.com/opencv/opencv.git && \
    cd opencv && \
    git checkout 4.1.2 && \
    cd .. && \
    git clone https://github.com/opencv/opencv_contrib.git && \
    cd opencv_contrib && \
    git checkout 4.1.2 && \
    cd .. && \
    cd opencv && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D INSTALL_C_EXAMPLES=ON -D INSTALL_PYTHON_EXAMPLES=ON -D WITH_TBB=ON -D WITH_V4L=ON -D WITH_QT=ON -D WITH_OPENGL=ON -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules -D BUILD_EXAMPLES=ON .. && \
    make -j8 && \
    make -j8 install

RUN cd /deps && \
    curl -L https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz -o libevent-2.1.8-stable.tar.gz && \
    tar xvf libevent-2.1.8-stable.tar.gz && \
    cd libevent-2.1.8-stable && \
    ./configure && \
    make -j8 && \
    make -j8 install

RUN cd /deps && \
    wget https://github.com/google/glog/archive/v0.3.5.tar.gz && \
    tar xvf v0.3.5.tar.gz && cd glog-0.3.5 && \
    ./configure && \
    make -j8 && \
    make -j8 install

RUN cd /deps && \
    git clone https://github.com/gperftools/gperftools.git && \
    cd gperftools && \
    git checkout gperftools-2.7 && \
    (./autogen.sh || ./autogen.sh) && \
    ./configure && \
    make -j8 && \
    make -j8 install

RUN cd /deps && \
    git clone https://github.com/edenhill/librdkafka.git && \
    cd librdkafka && \
    git checkout v0.11.4 && \
    ./configure && \
    make -j8 && \
    make -j8 install

RUN cd /deps && \
    git clone https://github.com/corehacker/ch-cpp-utils.git && \
    cd ch-cpp-utils && \
    ./autogen.sh && \
    ./configure && \
    make -j8 && \
    make -j8 install

# TODO Remove this link once we fix the code.
RUN ln -s /usr/local/include/opencv4/opencv2 /usr/local/include/opencv2

RUN cd /deps && \
    git clone https://github.com/corehacker/ch-storage-server.git && \
    cd ch-storage-server && \
    ./autogen.sh && \
    ./configure && \
    make -j8 && \
    make -j8 install

ENV LD_LIBRARY_PATH=/usr/local/lib

RUN mkdir -p /etc/ch-storage-server

COPY ./ch-configs/ch-storage-server /etc/ch-storage-server


# ln -s /usr/local/lib/python2.7/dist-packages/tensorflow_core/libtensorflow_framework.so.1 /usr/local/lib/libtensorflow_framework.so
# ln -s /usr/local/lib/python2.7/dist-packages/tensorflow_core/include/tensorflow /usr/local/include/tensorflow