#!/bin/sh
#check PR_NAME
#Author: caibin
#Date: 2014.12.05 

PR_NAME=$1
SUPPORT_LIST=$( ls apps/httpd/webs/product_spec/ )

show_usage()
{
	echo "==========================="
	echo "Now supported PR_NAME:"
	echo ${SUPPORT_LIST}
	echo "==========================="
}

IS_SUPPORTED="$(echo "$SUPPORT_LIST" | sed -ne "/$PR_NAME/p")"

if [ "$IS_SUPPORTED" = "$PR_NAME" ]; then
	echo "--try to build $PR_NAME--";
else
	echo "Product Name not supported!";
	show_usage;
	exit -1;
fi




