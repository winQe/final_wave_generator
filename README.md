# MA4830 Major Project - Real-Time Wave Generator
## Compilation and Running

### ` make all `
 This command removes the executable file if it exists, compiles all the necessary .c files, then runs the newly created executable file. The program will run in “Keyboard Input” mode.


### `make compile` 
This command compiles all the necessary .c files which creates a new executable file, but doesn’t execute the file


### `make run`
This command runs the executable file. The program will run in “Keyboard Input” mode.


### `make clean`  
This command removes the executable file if it exists.

### `make load`
This command runs the executable file and loads previously saved parameters from config.txt as the initial input.

### `make daq`
This command runs the executable file and enables input from potentiometer and toggle switches (DAQ Input Mode).