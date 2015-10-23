This program is a shell program can run on Debian OS. It is written in C/C++, most part is written in C, while it borrows features from C++ including string, vector<>, pair<>, ifstream and ofstream. To compile this program, you can use the makefile provided. Name of the program will be OS_hw3. 


It has implemented all features mentioned on the book and additional features in the PDF file. Here is a list of features: 

* Fork a child process to invoke execvp.

* If input command includes a separate ”&” at the end, the parent process will wait for child process. 
Please note that an “&” at other position of command, or not separated by space at the end, will not be recognized.

* History feature: by typing "history", shell displays up to 10 latest commands in history. 

* History commands execution: by typing "!N", shell will execute the Nth command in history. If you type "!!", shell will execute the last command in history. 
NOTICE: shell only stores 10 commands, if you have typed 20 commands and enter "!5", this feature will not work. Also, such commands in the history mode will be stored as actual commands rather than "!!" or "!N".

* Alias feature: works as described, if alias is in conflict with existing system program, shell will ask user whether to continue or stop. If user chooses to continue, it will translate such word as alias every time. Otherwise the alias will not be set, and user will be able to call this system program in the future.

* Script feature: shell will create a log file and redirect output of script to this log file. 
NOTICE: Please note that this program will read input one line at a time, please do not copy all script at one time. Please copy or type line by line. Please write the script like any other bash script. The script will be executed after typing "% endscript". 

* Path setting: the same as described. If there are more than one executable files found under preset path, shell will inform the user and let user choose which file to execute. 
Please note that “% set path = “ should have space between each two. And relative paths will be converted to absolute ones, e.g., under path of /home/os in the virtual box, type “% set path = (.)” will make shell search /home/os/ each time, rather than searching “./“.
 
* Initialization file: same as described. The file should be located under user's home directory(e.g., in /home/os in this virtualbox) and the file must named as ".cs543rc".

* Verbose: turn on the verbose mode can provide a lot of details.

* Pipe communication (extra credit): works as expected, must start with "%", rest part can be the same as linux commands.


Note that the configurations, i.e., additional features except pipe communication, are not supposed to include "&" at the last. And if you would like to invoke wait() in parent process, please always include a separate ”&” at the end, and only the end, of the command. You can also use “&” for pipe, you can type regular commands for pipe, and add a “&” at the end. 

Please also not that alias, script, path, verbose, pipe, and also the content in initialization file, should include a separate “%” in the front of all input commands.
