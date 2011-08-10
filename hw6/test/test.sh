#!/bin/csh
set files = `ls mmoutput`
foreach file ($files)
set args = `echo $file | sed 's/gcc-1K//g'`
set args = `echo $args | sed 's/[^0-9]/ /g'`
set output = "myoutput/$file"
../simulator ../gcc-1K.trace $args $output
diff -w ./mmoutput/$file $output > diff_$file
end
