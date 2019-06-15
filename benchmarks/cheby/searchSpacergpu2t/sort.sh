grep -v blockdim 64.log | grep -v Error | grep -v "^$" | grep -v "\<0\.0\>" | sort -n > out-64 
