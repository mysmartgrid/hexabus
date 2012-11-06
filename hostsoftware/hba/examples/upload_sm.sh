#!/bin/bash

if [ $# -ne "2" ]
then
    echo "Usage: upload_sm.sh statemachine_name ipv6_address"
    exit 1
fi


#Check if all files are there
if [ ! -f "$1.cond" ]
then
    echo "$1.cond not found!"
    exit 1
else
    cond_table="$1.cond"
    #cat $cond_table
fi

if [ ! -f "$1.trans" ]
then
    echo "$1.trans not found!"
    exit 1
else
    trans_table="$1.trans"
    #cat $trans_table
fi

if [ ! -f "$1.dttrans" ]
then
    echo "$1.dttrans not found!"
    exit 1
else
    dttrans_table="$1.dttrans"
    #cat $dttrans_table
fi

#Build curl commands
post_cond="curl -6 -X POST --data-binary @$cond_table http://$2/sm_post.shtml"
post_trans="curl -6 -X POST --data-binary @$trans_table http://$2/sm_post.shtml"
post_dttrans="curl -6 -X POST --data-binary @$dttrans_table http://$2/sm_post.shtml"

post_start="curl -6 -X POST -d start& http://$2/sm_control.shtml"
post_stop="curl -6 -X POST -d stop& http://$2/sm_control.shtml"

#Post to device
set -e
echo "Stopping state machine using $post_stop."
#$post_stop
echo "Uploading conditions using $post_cond ..."
$post_cond
echo "Uploading transitions..."
$post_trans
echo "Uploading datetime transitions..."
$post_dttrans
echo "Starting state machine"
#$post_start
set +e

echo "Upload complete"
exit 0
