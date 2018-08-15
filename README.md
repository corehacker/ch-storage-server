# ch-storage-server

[![Build Status](https://travis-ci.org/corehacker/ch-storage-server.png?branch=master)](https://travis-ci.org/corehacker/ch-storage-server)

* Server receives HTTP POSTs for inidividual segments.
* Save in directory structure based on date and hour.
* Purge content after TTL expiry.
* Sever runs on a desktop linux based machine.
* Motion Detection
** v1: Used OpenCV background subtraction concept to detect motion.
** v2: Used Tensorflow to detect if a specific motion has motion or not. Trained a custom graph atop inception v3 and used the same for motion detection.
* Setup a postfix mail server which would trigger notifications via email.
* Simple JSON configuration framework for customization (feature flags/notification config/tensorflow parameters).
