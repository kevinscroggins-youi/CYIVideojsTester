#!/bin/bash

./generate.rb -p tizen-nacl
./build.rb -b build/tizen-nacl/Debug
tizen install -n /Users/kevinscroggins/youidev/Projects/CYIVideojsTestApp/cpp/build/tizen-nacl/Debug/VideojsTester-Debug.wgt
tizen run -p gWjgX0Xyna.videojstester
