touch global;
awk '/unroll k=1,j=1,i=1/{print}' 128.log | grep -v Error | awk -F " " '{print $NF}' |grep -v "\<0\.0\>" | grep -v "^$" | sort -n >> global
awk '/unroll k=1,j=1,i=1/{print}' 255.log | grep -v Error | awk -F " " '{print $NF}' |grep -v "\<0\.0\>" | grep -v "^$" | sort -n >> global
awk '/unroll k=1,j=1,i=1/{print}' 32.log | grep -v Error  | awk -F " " '{print $NF}' |grep -v "\<0\.0\>" | grep -v "^$" | sort -n >> global
awk '/unroll k=1,j=1,i=1/{print}' 64.log | grep -v Error  | awk -F " " '{print $NF}' |grep -v "\<0\.0\>" | grep -v "^$" | sort -n >> global
