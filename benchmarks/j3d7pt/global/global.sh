touch global;
awk '/unroll k=1,j=1,i=1/{print; nr[NR+1]; next}; NR in nr' 64.log | grep -v blockdim | grep -v Error | grep -v "^$" | grep -v "\<0\.0\>" | sort -n >> global
