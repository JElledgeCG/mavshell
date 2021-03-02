# mavshell
A portable, bash-like UNIX shell.
- Written in C
- Accepts all supported shell commands
- Includes history command, which shows the last 15 attempted commands
- Includes a listpids command, which will list the process IDs of the last 15 processes spwaned by the shell
- Includes a !n command, allowing the user to rerun the n most recent command (same n as history outputs)

To Run:
- On a linux system with gcc installed, in the terminal use 
  > `gcc -Wall msh.c -o msh -std=c99`
- Type `./msh`
