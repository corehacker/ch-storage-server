AM_CPPFLAGS = -I..
bin_PROGRAMS = ch-storage-server


if ENABLE_TF
ch_storage_server_SOURCES =  \
						main.cpp \
						storage-server.cpp \
						config.cpp \
						motion-detector.cpp \
						label-image.cpp \
						kafka-client.cpp \
						mail-client.cpp \
						curl-smtp.cpp \
						firebase-client.cpp
else
ch_storage_server_SOURCES =  \
						main.cpp \
						storage-server.cpp \
						config.cpp \
						motion-detector.cpp \
						kafka-client.cpp \
						mail-client.cpp \
						curl-smtp.cpp \
						firebase-client.cpp
endif



