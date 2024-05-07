__What__

Executable uwc counts the number of distinct unique words in a file whose name is passed as an argument to a program.

For example, given the file content "a horse and a dog" the program must output "4" (the word 'a' appears twice but only accounts for one distinct unique occurrence).
The input text is guaranteed to contain only 'a'..'z' and space characters in ASCII encoding.
The program should be able to handle large inputs (e.g. 32 GiB)
You can assume that all unique words fit into memory when using the data structure of your choice.
The solution must utilize all available CPU resources.

__How-to__

Run prepare-ubuntu20.04.sh to build and run docker image based on Ubuntu 20.04 LTS with clang 10
or prepare-ubuntu20.04.sh to build and run docker image based on Ubuntu 22.04 LTS with clang 14

Run ./build.sh inside docker to build excecutalbles

In ./tests.sh update value of dir according to used compiler
Run ./test.sh to generate test inputs and run tests.

