FROM quay.io/jlospinoso/cppbuild:v1.2.0
RUN apt update && apt upgrade -y

RUN mkdir abrade
WORKDIR abrade

COPY *.h *.hpp *.cpp CMakeLists.txt ./
COPY beast ./beast
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
ENTRYPOINT ["/abrade/build/abrade"]
