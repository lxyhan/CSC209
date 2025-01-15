./echo_arg csc209 > echo_out.txt
./echo_stdin < echo_stdin.c
seq 0 209 | tr -d '\n' | wc -c
ls -S | ./echo_stdin
