FROM gcr.io/oss-fuzz-base/base-builder
RUN apt-get update && apt-get install -y make autoconf automake libtool
COPY . $SRC/ClusterFuzzer
WORKDIR $SRC/ClusterFuzzer
COPY .clusterfuzzlite/build.sh $SRC/