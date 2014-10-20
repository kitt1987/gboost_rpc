#!/bin/bash

for protocol in `ls *.proto`
do
	protoc --cpp_out=. ${protocol}
done