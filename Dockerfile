FROM quay.io/jlospinoso/cppbuild:v1.1.0
RUN apt update && apt upgrade -y

RUN mkdir abrade
WORKDIR abrade

COPY *.h *.hpp *.cpp CMakeLists.txt ./
RUN mkdir build
RUN git clone https://github.com/boostorg/beast.git
WORKDIR build
RUN cmake ..
RUN make
ENTRYPOINT ["/abrade/build/abrade"]
