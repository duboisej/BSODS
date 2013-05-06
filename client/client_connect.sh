#!/bin/bash

for i in $(eval echo {1..$1});
do
	./one_client.sh $2;
done