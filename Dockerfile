FROM quay.io/jlospinoso/cppbuild:v1.5.0
RUN apt update && apt upgrade -y && apt install gcc-9 python3 python3-pip python3-setuptools
RUN python3 -m pip install --upgrade pip && python3 -m pip install --upgrade conan conan-package-tools

RUN mkdir abrade
WORKDIR abrade

COPY *.h *.hpp *.cpp CMakeLists.txt  ./
RUN mkdir build
WORKDIR build
RUN CC=gcc-9 CXX=g++-9 conan install .. --build missing -g cmake
RUN CC=gcc-9 CXX=g++-9 cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make
ENTRYPOINT ["/abrade/build/abrade"]
