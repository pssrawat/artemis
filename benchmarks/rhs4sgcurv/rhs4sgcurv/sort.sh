tac 255.log | sed '/Run Error/I,+2 d' | tac | grep -v blockdim | grep -v Error | grep -v "^$" | grep -v "\<0\.0\>" | sort -n > out-255 
