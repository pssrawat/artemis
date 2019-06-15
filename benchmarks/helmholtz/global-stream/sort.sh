grep -v Error 255.log | awk -F " " '{print $NF}' |grep -v "\<0\.0\>" | grep -v "^$" | sort -n > out-255
