#!/bin/bash

# Get the IMP KEY
IMP_KEY="`cat ./IMP_BUILD_KEY | openssl enc -base64`"

# Get the MODEL name
IMP_MODEL="`cat ./IMP_MODEL`"

# URL to the electric imp workflow
URL="https://build.electricimp.com/v4/"

# List Devices
DEVICE_ARGS="devices/"

# Agent src path
agent_code="/tmp/agent_code.nut"
device_code="/tmp/device_code.nut"

# For some reason, I can't get the following line in a variable to be eval'd
# @TODO: Figure out why I can't do resp=`curl ...`
curl -X GET -H "Authorization: Basic $IMP_KEY" $URL$DEVICE_ARGS

# Copyright
BUILD_DATE="//    Built `date` on `hostname`\n//    git hash:`git rev-parse HEAD` \n"

# Custom 3rd party modules are a pain to work with in the stock IMP IDE
# Make sure the list is in order of dependencies!
DEVICE_SRCS=(bmp180.c device.c)
AGENT_SRCS=(agent.c)

# Create a tmp file
cat /dev/null > $device_code
cat /dev/null > $agent_code

for index in ${!DEVICE_SRCS[*]}
do
    echo "Concatenating ${DEVICE_SRCS[$index]} to $device_code"
    echo $index
    if [ "$index" -gt "0" ] ; then
        echo "Adding header"
        echo "//==================================" >> $device_code
        echo "//${DEVICE_SRCS[$index]}            " >> $device_code
        echo "//==================================" >> $device_code
    fi
    cat ${DEVICE_SRCS[$index]} >> $device_code
done
      
for index  in ${!AGENT_SRCS[*]}
do
    echo "Concatenating ${AGENT_SRCS[$index]} to $agent_code"
    echo $index
    if [ "$index" -gt "0" ] ; then
        echo "Adding header"
        echo "//==================================" >> $agent_code
        echo "//${AGENT_SRCS[$index]}             " >> $agent_code
        echo "//==================================" >> $agent_code
    fi
    cat ${AGENT_SRCS[$index]} >> $agent_code
done

# Put the date into the file
#echo -e $BUILD_DATE >> $device_code
#echo -e $BUILD_DATE >> $agent_code

#agent_raw=`cat $agent_code | openssl enc -base64`
#device_raw=`cat $device_code | openssl enc -base64`

agent_raw=`cat $agent_code | python -c 'import json,sys; print json.dumps(sys.stdin.read())'`
device_raw=`cat $device_code | python -c 'import json,sys; print json.dumps(sys.stdin.read())'`

//echo $agent_raw

echo "Attempting to commit source to IMP cloud IDE..."
# To POST code to the server
resp=`curl -X POST -H "Authorization: Basic $IMP_KEY" \
-H "Content-Type: application/json" -d \
"{\"agent_code\":$agent_raw,\"device_code\":$device_raw}" \
https://build.electricimp.com/v4/models/$IMP_MODEL/revisions`

#echo -e $resp

echo "$resp" | python -m json.tool

