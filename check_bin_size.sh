#!/bin/bash

function checkBinarySize {

    filename=$1
    maxSize=72 # On-chip ram size for imx50 in kB

    # get file size in bytes
    binSize=$(stat --printf="%s" $filename)

    ((extra=$maxSize*1024-$binSize))

    if [ "$extra" -gt "0" ]; then
        echo "$filename fits into the $maxSize kB on-chip ram with $extra bytes to spare"

    elif [ "$extra" -lt "0" ]; then
        ((missing=-$extra))
        echo "ERROR: $filename is $missing bytes too big for the 72 kB on-chip RAM!" >&2
        exit 1
    else
        echo "$filename fits _exactly into the $maxSize kB on-chip RAM"
    fi

}
