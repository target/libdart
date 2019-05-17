# Pull in our big-endian cross-compiler
FROM cfretz244/libdart_ci:bebuilder

# Set ourselves to use a login shell
# so we use our big-endian toolchain
SHELL ["/bin/bash", "--login", "-c"]

# We need to install rapidjson to build our
# parsing tests, and unfortunately the
# version on APT is 3 years out of date.
RUN apt-get install -y curl
RUN BUILD_DIR=$(mktemp -d)                                                                \
  && pushd $BUILD_DIR                                                                     \
  && curl -sL "https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz" | tar xzf -    \
  && cd rapidjson-1.1.0                                                                   \
  && cp -r include/rapidjson /usr/local/include                                           \
  && popd                                                                                 \
  && rm -rf $BUILD_DIR

# Copy the code in.
COPY . /root/libdart

# Compile it.
CMD cd /root/libdart                                                                      \
  && mkdir build                                                                          \
  && cd build                                                                             \
  && cmake .. -D32bit=ON -Dstatic_test=ON -Dextended_test=ON                              \
  && make VERBOSE=1
