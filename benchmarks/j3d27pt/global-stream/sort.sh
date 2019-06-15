grep -v blockdim 128.log | grep -v Error | grep -v "^$" | grep -v "\<0\.0\>" | sort -n > out-128 
