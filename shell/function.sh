#!/bin/sh
acanoe=$(echo "acanoe")
function_1()
{
	echo "this is acanoe function 1"
	echo $1
}
function_2()
{
	function_1 $1
	echo $1
	echo "this is acanoe function 2"
}

function_2  $acanoe
