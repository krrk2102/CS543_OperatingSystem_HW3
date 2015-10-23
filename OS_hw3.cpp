#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>
#include <dirent.h>
#include <utility>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

/**
 * This is the homework 3 for Operating System, a simple shell.
 * Most part of this shell is written in pure C.
 * C++ features used are: string, pair<>, vector<>, ifstream and ofstream.
 * It implements all features described on the book and addtional features, also the extra credit.
 * It is not sensitive to redundant spaces or newline characters for command-line inputs.
 * The script you type should strictly the same as regular bash script.
 */

/* The maximum length command */
#define MAX_LINE 80

/**
 * int ReadInput(char **s,int cstar_size, FILE *stream, bool &_verbose, bool txt_mode = false)
 * char **s is arguments to store input commands, cstar_size is for maximum number of commands, *stream is the input stream you want and generally should be stdin, _verbose decides whether to display addtional information, txt_mode chooses its working mode: command-line inputs or regular inputs.
 * If inputs are stored successfully, it returns number of arguments, otherwise it returns a negative value, usually the value of input arguments number.
 * This function is modified from function fgets. It can be used for command-line input (defult: txt_mode=false) and also regular input.
 * For command line inputs, it can devide inputs by ' ', and also recognize quotation mark (e.g., read "ls -al" as a single input). When you just want to use '"' for input, you can input '\"'.
 * For regular input, it can devide inputs by ' ', quotation mark will not work now. Every input but space will be passed.
 * This function is not sensitive to redundant ' ' and empty lines.(e.g. read " ls   -al" as 2 arguments ls and -al)
 */
int ReadInput(char **s,int cstar_size, FILE *stream, bool &_verbose, bool txt_mode = false) {
    int n = MAX_LINE;
    register int c;
    register char *cs;
    int i = -1;
    int end = 0;
    while(++i < cstar_size && end == 0) {
        s[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
        cs = s[i];
        int syntax = -1;
        bool quote = false;
        while(--n >= 0 && (c = getc(stream)) != EOF) {
            /* Ends input reading when entering '\n' */
            if (c == '\n') {
                if (cs == s[i]) {
                    i--;
                }
                if (quote == true) {
                    fprintf(stderr, "A \" at the end is expected.\n");
                }
                end = 1;
                break;
            } else if ((c == ' ' || c == '\t') && quote == false) {
                /* If input is within a set of '"', function will not devide them up. */
                if (n != MAX_LINE-1) {
                    break;
                } else n = MAX_LINE;
            } else if (c == '"') {
                /* Decide whether to recognize it as quatation by previous input and current mode. */
                if (txt_mode == false) {
                    if (n != syntax-1) {
                        quote = !quote;
                    } else {
                        *--cs = c;
                        cs++;
                    }
                } else *cs++ = c;
            } else if (c == '\\') {
                /* As a reference for next input '"', if applicable. */
                *cs++ = c;
                if (txt_mode == false) {
                    syntax = n;
                }
            } else {
                /* Not a special character, just copy it. */
                *cs++ = c;
            }
            /* Checking length if a single command. */
            if (n == 0) {
                printf("You have exceeds maximum length of a single command.\n");
                return -i-1;
            }
        }
        *cs = '\0';
        /* Checking total number of commands. */
        if (i == cstar_size-1 && end == 0) {
            printf("You have reached maximum number of arguments, no more arugments acceptable.\n");
            return -i-1;
        }
        n = MAX_LINE;
    }
    if (_verbose == true) {
        printf("Input commands and parameters are:\n");
        for (int j = 0; j < i; j++) {
            printf("%s| ", s[j]);
        }
        printf("\n");
    }
    return i;
}

/**
 * void add_history_cmd(char * _args[MAX_LINE/2+1], int &_argument_size, char * _cmd_history[10][MAX_LINE/2+1], int &_history_counter)
 * *_args[MAX_LINE/2+1] is the command just collected from user input, _argument_size is number of input arguments in _args, *_cmd_history[10][MAX_LINE/2+1] is stored commands in history. _history_counter shows how many commands are in history.
 * This function is for adding most recent input commands into commands input history list. The list contains up to 10 most recent inputs.
 * This function will add new input into the front of list, moving rest ones backwards.
 * If there are more than 10 commands in history, remove the last one.
 */
void add_history_cmd(char * _args[MAX_LINE/2+1], int &_argument_size, char * _cmd_history[10][MAX_LINE/2+1], int &_history_counter) {
    _history_counter++;
    /* Moving history commands backwards, making the front of the list vacant. The last one in the list will be overwritten, if applicable. */
    for (int i = 9; i > 0; --i) {
        if (i < _history_counter) {
            for (int j = 0; j < (MAX_LINE/2+1); j++) {
                if (_cmd_history[i-1][j] != (char*)0) {
                    if (_cmd_history[i][j] != (char*)0) {
                        free(_cmd_history[i][j]);
                    }
                    _cmd_history[i][j] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                    strcpy(_cmd_history[i][j], _cmd_history[i-1][j]);
                } else {
                    if (_cmd_history[i][j] != (char*)0) {
                        free(_cmd_history[i][j]);
                        _cmd_history[i][j] = (char*)0;
                    }
                    break;
                }
            }
        } else continue;
    }
    /* Inserting latest commands into the first place. */
    for (int i = 0; i < (MAX_LINE/2+1); i++) {
        if (_args[i] != (char*)0) {
            if (_cmd_history[0][i] != (char*)0) {
                free(_cmd_history[0][i]);
            }
            _cmd_history[0][i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
            strcpy(_cmd_history[0][i], _args[i]);
        } else {
            if (_cmd_history[0][i] != (char*)0) {
                free(_cmd_history[0][i]);
                _cmd_history[0][i] = (char*)0;
            }
            break;
        }
    }
}

/**
 * void display_history(char * _cmd_history[10][MAX_LINE/2+1], int &_history_number)
 * *_cmd_history[10][MAX_LINE/2+1] is list of commands history, _history_number shows how many commands has been input since the shell runs.
 * This function will display all commands in the history list.
 * It will display up to 10 recent commands.
 */
void display_history(char * _cmd_history[10][MAX_LINE/2+1], int &_history_number) {
    for (int i = 0; i < 10; i++) {
        /* If there are less than 10 commands in history, the function checks for list boundary. */
        if (_history_number <= 10) {
            if (i < _history_number) {
                printf("%d ", _history_number-i);
                for (int j = 0; j < (MAX_LINE/2+1); j++) {
                    if (_cmd_history[i][j]  != (char*)0) {
                        printf("%s ", _cmd_history[i][j]);
                    } else break;
                }
                printf("\n");
            } else break;
        } else {
            printf("%d ", _history_number-i);
            for (int j = 0; j < (MAX_LINE/2+1); j++) {
                if (_cmd_history[i][j] !=  (char*)0) {
                    printf("%s ", _cmd_history[i][j]);
                } else break;
            }
            printf("\n");
        }
    }
}

/**
 * void history_cmd_exec(char * _args[MAX_LINE/2+1], char * _cmd_history[10][MAX_LINE/2+1], int &_argument_size, int &_history_counter, bool &_verbose)
 * *_args[MAX_LINE/2+1] stands for user input commands, *_cmd_history[10][MAX_LINE/2+1] is used to store the list of history commands, _argument_size of number of input commands, in this case it should be 1, _history_counter is number of commands in history, _verbose decides whethere work in verbose mode.
 * This function will find the history commands that user wants, and replace the current one with the one wanted exactly, then pass it back to shell for execution.
 * This function only work with latest 10 history commands.
 */
void history_cmd_exec(char * _args[MAX_LINE/2+1], char * _cmd_history[10][MAX_LINE/2+1], int &_argument_size, int &_history_counter, bool &_verbose) {
    char * number = _args[0];
    number++;
    int history_position = atoi(number);
    number = (char*)0;
    free(_args[0]);
    _args[0] = (char*)0;
    /* Check if the commands wanted is within the 10 latest ones, if so, copy the right one to args. */
    if (history_position > 0 && history_position >= _history_counter - 9 && history_position <= _history_counter) {
        history_position  = _history_counter - history_position;
        if (_verbose == true) {
            printf("The %d commands in history is: ", history_position);
        }
        for (int i = 0; i < (MAX_LINE/2+1); i++) {
            if (_cmd_history[history_position][i] != (char*)0) {
                if (_args[i] != (char*)0) {
                    free(_args[i]);
                }
                _args[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                strcpy(_args[i], _cmd_history[history_position][i]);
                if (_verbose == true) {
                    printf("%s ", _args[i]);
                }
                _argument_size++;
            } else break;
        }
        if (_verbose == true) {
            printf("\n");
        }
    /* If the number wanted is out of bound, output relative information to user. */
    } else if (history_position < _history_counter - 9 && history_position > 0) {
        printf("You can only access up to 10 most recent history commands, please use history command to check the commands you want.\n");
        free(_args[0]);
        _args[0] = (char*)0;
        _argument_size = 0;
    } else {
        printf("Please enter a valid number for history command, use history command to check the commands you want.\n");
        free(_args[0]);
        _args[0] = (char*)0;
        _argument_size = 0;
    }
}

/**
 * void latest_cmd_exec(char * _args[MAX_LINE/2+1], char * _cmd_history[10][MAX_LINE/2+1], int &_argument_size, int &_history_counter, bool &_verbose)
 * *_args[MAX_LINE/2+1] is for input commands, *_cmd_history[10][MAX_LINE/2+1] is commands in history, _argument_size is number of input commands, _history_counter records number of commands input by user, _verbose is to control function whether to work in verbose mode. 
 * This function replace the input "!!" with the most recent commands in history, then it is handled back to shell.
 * If there is no commands in history, it will do nothing but print "No commands in history." to user.
 */
void latest_cmd_exec(char * _args[MAX_LINE/2+1], char * _cmd_history[10][MAX_LINE/2+1], int &_argument_size, int &_history_counter, bool &_verbose) {
    free(_args[0]);
    _args[0] = (char*)malloc((MAX_LINE+1)*sizeof(char));
    if (_history_counter > 1) {
        /* If there are commands in history, the function will find most recent one and replace input with it. */
        if (_verbose == true) {
            printf("Last command is: ");
        }
        for (int i = 0; i < (MAX_LINE/2+1); i++) {
            if (_cmd_history[0][i] != (char*)0) {
                if (_args[i] == (char*)0) {
                    _args[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                }
                strcpy(_args[i], _cmd_history[0][i]);
                if (_verbose == true) {
                    printf("%s ", _args[i]);
                }
                _argument_size++;
            } else break;
        }
        if (_verbose == true) {
            printf("\n");
        }
    } else {
        /* If there is no commands in history, the function prints the error information to user. */
        fprintf(stderr, "No commands in history.\n");
        free(_args[0]);
        _args[0] = (char*)0;
        _argument_size = 0;
    }
}

/**
 * void set_path(char * _args[MAX_LINE/2+1], int &_argument_size, vector<string> &_envir_path, bool &_verbose)
 * *_args[MAX_LINE/2+1] and _argument_size are input commands and its size, _envir_path is the list for paths set before, _verbose for verbose mode.
 * This function will add input path into the list. It will devide "% set path = ($PATH)" up and add path into the list.
 */
void set_path(char * _args[MAX_LINE/2+1], int &_argument_size, vector<string> &_envir_path, bool &_verbose) {
    for (int i = 4; i < _argument_size; i++) {
        string path;
        /* This is to remove first '(' from path list. */
        if (i == 4) {
            if (_args[i][1] != '\0') {
                path.append(_args[i]+1);
            } else continue;
        } else {
            if (_args[i][0] != '\0') {
                path.append(_args[i]);
            } else continue;
        }
        /* Supplement for real paths. */
        if (path == ".") {
            path.insert(1, "/");
        } else if (path == "..") {
        	  path.insert(2, "/");
        }
        if (path.size() >= 2) {
            if (path.at(0) != '/' && path.substr(0, 2) != "./" && path.substr(0, 3) != "../") {
                path.insert(0, "./");
            }
        } else {
            if (path.at(0) != '/') {
                path.insert(0, "./");
            }
        }
        /* This is to process relative path. */
        if (path.substr(0, 2) ==  "./") {
            path = path.substr(2);
            char curr_path[100];
            getcwd(curr_path, 100);
            path.insert(0, curr_path);
        } else if (path.size() >= 3 && path.substr(0, 3) == "../") {
        	  path = path.substr(3);
        	  char curr_path[100];
        	  getcwd(curr_path, 100);
        	  string tmp_path(curr_path);
        	  int slash = tmp_path.size();
        	  while (tmp_path[--slash] != '/') {
        	  	  tmp_path.erase(tmp_path.end()-1);
        	  }
        	  path.insert(0, tmp_path.c_str());
        }
        if (path.at(path.size()-1) != '/') {
            path.append("/");
        }
        /* If the input path has been added before, it will exit adding procedure, otherwise it adds new commands into the path. */
        bool exist = false;
        for (int j = 0; j < _envir_path.size(); j++) {
            if (_envir_path[j] == path) {
                exist = true;
                if (_verbose == true) {
                    printf("The path already exists.\n");
                }
                break;
            }
        }
        if (exist == false) {
            _envir_path.push_back(path);
            if (_verbose == true) {
                printf("Path has been set.\n");
            }
        }
    }
}

/**
 * bool search_path(char * _args[MAX_LINE/2+1], vector<string> &_envir_path, bool &_verbose, bool check_only = false)
 * *_args[MAX_LINE/2+1] is user input, _envir_path stores paths to search executable files, _verbose is to choose work mode, check_only is only to find if there is executable file exists rather than to change the input path.
 * This function is to search the paths set by the user and to find if there are avaible executable files under the paths. 
 * If it finds a file, it returns true otherwise false.
 * If it works in chech_only mode, it will return true immediately finding the file. While it will change user input by adding absolute filename, when working under normal mode.
 */
bool search_path(char * _args[MAX_LINE/2+1], vector<string> &_envir_path, bool &_verbose, bool check_only = false) {
    bool result = false;
    string full_path;
    vector<string> full_paths;
    /* Checking preset path for intended executable file. */
    for (int i = 0; i < _envir_path.size(); i++) {
        if (_verbose == true) {
            printf("Shell is searching the directory of %s\n", _envir_path[i].c_str());
        }
        DIR * file_path = NULL;
        struct dirent * file_ptr;
        if ((file_path = opendir(_envir_path[i].c_str())) != NULL) {
            while (file_ptr = readdir(file_path)) {
                full_path = _envir_path[i];
                full_path.append(file_ptr->d_name);
                /* If one result is found, add it to found list. */
                if (strcmp(full_path.c_str(), _args[0]) == 0 || strcmp(file_ptr->d_name, _args[0]) == 0) {
                    if (check_only == true) {
                        return true;
                    } else result = true;
                    full_paths.push_back(full_path);
                    if (_verbose == true) {
                        printf("Directory found: %s\n", full_path.c_str());
                    }
                }
                full_path.clear();
            }
        } else printf("Cannot open the dir: %s, please check the path or your preset input.\n", _envir_path[i].c_str());
        if (_verbose == true) {
            printf("Shell is leaving the directory of %s\n", _envir_path[i].c_str());
        }
        closedir(file_path);
    }
    /* If only one file is found, replace old relative filename with the absolute filename. */
    if (full_paths.size() == 1) {
        free(_args[0]);
        _args[0] = (char*)malloc((MAX_LINE+1)*sizeof(char));
        strcpy(_args[0], full_paths[0].c_str());
    /* If there is no files found, print information to user. */
    }else if (full_paths.size() == 0) {
        if (_verbose == true && check_only == false) {
            fprintf(stderr, "There is no file found in the preset path. Commands will be handed to execvp function directly.\n");
        }
    /* If there are more than 1 files found, print information to user and let user choose one of them. */
    }else {
        printf("There are more than one file found from preset path. Please enter the file you want to execute:\n");
        for (int i = 0; i < full_paths.size(); i++) {
            printf("File %d: %s\n", i+1, full_paths[i].c_str());
        }
        bool right_path = false;
        while (right_path == false) {
            char ** tmp = (char**)malloc(sizeof(char*));
            * tmp = (char*)malloc((MAX_LINE+1)*sizeof(char));
            ReadInput(tmp, 1, stdin, _verbose, true);
            for (int i = 0; i < full_paths.size(); i++) {
                if (strcmp(*tmp, full_paths[i].c_str()) == 0) {
                    free(_args[0]);
                    _args[0] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                    strcpy(_args[0], *tmp);
                    printf("You have chosen %s\n", _args[0]);
                    right_path = true;
                }
            }
            /* By typing "exit", the shell will be allowed to pass relative filename to system. */
            if (strcmp(*tmp, "exit") == 0) {
                right_path = true;
            }
            if (right_path == false) {
                printf("Please enter your path exactly the same as paths displayed above. Enter exit to skip and pass it to system direclty to handle.\n");
            }
            free(*tmp);
            free(tmp);
        }
    }
    return result;
}

/**
 * void set_alias(char * _args[MAX_LINE/2+1], vector<pair<string, vector<string> > > &_alias_list, vector<string> &_envir_path, bool &_verbose)
 * *_args is user input, _alias_list stores all alias costumized by user, _envir_path is user-set path, _verbose for working mode.
 * This function adds alias and their real commands into the list. They are stored as pair<string, vector<string> >, the first stands for the alias, second for real commands.
 * This function will check if the alias is conflict with names of existing system programs, and let user choose whether continue the setting.
 */
void set_alias(char * _args[MAX_LINE/2+1], vector<pair<string, vector<string> > > &_alias_list, vector<string> &_envir_path, bool &_verbose) {
    string alias_name(_args[0]);
    vector<string> cmd;
    if (_verbose == true) {
        printf("Please make sure your alias is not conflict with parameters for system programs\n");
    }
    /* Check if alias is conflict with existing system program, if so, let user choose whether using it by blocking system program call. */
    bool stop_for_conflict = search_path(_args, _envir_path, _verbose, true);
    if (stop_for_conflict == true) {
        printf("Your alias is conflict with existing system program, type yes to continue and the system program will not be called in the future, or type no to stop set this alias.\n");
        while (strcmp(_args[0], "yes")!=0 && strcmp(_args[0], "no")!=0) {
            printf("Please type exactly yes or no:\n");
            free(_args[0]);
            _args[0] = (char*)0;
            ReadInput(_args, 1, stdin, _verbose);
        }
        if (strcmp(_args[0], "no") == 0) {
            if (_verbose == true) {
                printf("You chose to continue.\n");
            }
            stop_for_conflict = true;
        } else {
            if (_verbose == true) {
                printf("You chose to stop.\n");
            }
            stop_for_conflict = false;
        }
    }
    /* If the alias conflicts with old ones, it will overwrite old one. */
    if (stop_for_conflict == false) {
        if (_verbose == true) {
            printf("Checking previous alias.\n");
        }
        for (vector<pair<string, vector<string> > >::iterator it = _alias_list.begin(); it != _alias_list.end(); ++it) {
            if (it->first == alias_name) {
                it = _alias_list.erase(it);
                if (_verbose == true) {
                    printf("Alias already exists. Old one will be overwriten.\n");
                }
                break;
            }
        }
        if (_verbose == true) {
            printf("Setting your alias.\n");
        }
        string alias(_args[1]);
        int pos = -1;
        while (++pos < alias.size()) {
            string single_cmd;
            bool quote = false;
            int counter = 0;
            int syntax = -2;
            while (pos < alias.size()) {
                if (alias.at(pos) == ' ' && quote == false) {
                    break;
                } else if (alias.at(pos) == '\\') {
                    syntax = counter;
                    single_cmd.append(&alias.at(pos), 1);
                } else if (alias.at(pos) == '"') {
                    if (counter == syntax+1) {
                        single_cmd.erase(single_cmd.end());
                        single_cmd.append(&alias.at(pos), 1);
                    } else {
                        quote = !quote;
                    }
                } else {
                    single_cmd.append(&alias.at(pos), 1);
                }
                pos++;
                counter++;
            }
            cmd.push_back(single_cmd);
        }
        _alias_list.push_back(make_pair(alias_name, cmd));
        if (_verbose == true) {
            printf("Your alias has been set successfully.\n");
        }
    }
}

/**
 * void check_alias(char * _args[MAX_LINE/2+1], int &_argument_size, vector<pair<string, vector<string> > > &_alias_list, bool &_verbose)
 * *_args and _argument_size are user input and its size, _alias_list is preset alias by users, _verbose for verbose mode.
 * This function will check if a user input contains alias and replace alias with real commands.
 */
void check_alias(char * _args[MAX_LINE/2+1], int &_argument_size, vector<pair<string, vector<string> > > &_alias_list, bool &_verbose) {
    /* This function will check every single input for alias and replace every one. */
    for (int i = 0; i < _argument_size; i++) {
        string input_cmd(_args[i]);
        for (int j = 0; j < _alias_list.size(); j++) {
            if (input_cmd == _alias_list[j].first) {
                if (_verbose == true) {
                    printf("Alias found: %s\n", input_cmd.c_str());
                }
                if (_alias_list[j].second.size() > 1 && _argument_size > 1) {
                    for (int k = _argument_size - 1; k > i; k--) {
                        _args[k+_alias_list[j].second.size()-1] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(_args[k+_alias_list[j].second.size()-1], _args[k]);
                        free(_args[k]);
                        _args[k] = (char*)0;
                    }
                }
                if (_verbose == true) {
                    printf("Replacing alias with commands: ");
                }
                for (int k = i; k < i + _alias_list[j].second.size(); k++) {
                    if (_args[k] != (char*)0) {
                        free(_args[k]);
                    }
                    _args[k] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                    strcpy(_args[k], _alias_list[j].second[k-i].c_str());
                    if (_verbose == true) {
                        printf("%s ", _args[k]);
                    }
                }
                if (_verbose == true) {
                    printf("\n");
                }
                _argument_size = _argument_size - 1 + _alias_list[j].second.size();
                break;
            }
        }
    }
}

/**
 * void verbose_property(char * _args[MAX_LINE/2+1], int &_argument_size, bool &_verbose)
 * *_args and _argument_size are user input and size, _verbose is for mode switching.
 */
void verbose_property(char * _args[MAX_LINE/2+1], int &_argument_size, bool &_verbose) {
    if (_verbose == false) {
        if (strcmp(_args[3], "on") == 0) {
            _verbose = true;
            printf("You have turned on verbose mode.\n");
        } else printf("Verbose mode is already off.\n");
    } else {
        if (strcmp(_args[3], "off") == 0) {
            _verbose = false;
            printf("You have turned off verbose mode.\n");
        } else printf("Verbose mode is already on.\n");
    }
}

/**
 * void script_exec(char * _args[MAX_LINE/2+1], int &_argument_size, ofstream &_logfile, bool &_verbose)
 * _args and _argument_size are user input and its size, _logfile is the file for script input, _verbose is for verbose mode.
 * This function will record user input for script and save it for execution.
 * Please enter the script with regular script grammar.
 */
void script_exec(char * _args[MAX_LINE/2+1], int &_argument_size, ofstream &_logfile, bool &_verbose) {
    printf("Please type your script, ends with \"%% endscript\":\nAll outputs are redirected to the file.\n");
    char * dest_sh = (char*)malloc((MAX_LINE+1)*sizeof(char));
    bool initial = true;
    ofstream sh_file("./.cs543_tmp_script");
    while (true) {
        for (int i = 0; i < _argument_size; i++) {
            if (_args[i] != (char*)0) {
                free(_args[i]);
                _args[i] = (char*)0;
            }
        }
        _argument_size = ReadInput(_args, MAX_LINE/2, stdin, _verbose, true);
        /* If user input "% endscript", exit the procedure and save file. */
        if (_argument_size == 2 && strcmp(_args[0], "%") == 0 && strcmp(_args[1], "endscript") == 0) {
            break;
        }
        for (int i = 0; i < _argument_size; i++) {
            _logfile<<_args[i]<<" ";
            sh_file<<_args[i]<<" ";
        }
        _logfile<<endl;
        sh_file<<endl;
        if (initial == true) {
            if (_args[0][0] == '#' && _args[0][1] == '!') {
                strcpy(dest_sh, _args[0]+2);
            } else {
                strcpy(dest_sh, "sh");
            }
        }
        initial = false;
    }
    _logfile<<endl<<"Script response is:"<<endl;
    _logfile.close();
    sh_file.close();
    free(_args[0]);
    _args[0] = (char*)malloc((MAX_LINE+1)*sizeof(char));
    strcpy(_args[0], dest_sh);
    free(dest_sh);
}

/**
 * void initialization_file(vector<pair<string, vector<string> > > &_alias_list, vector<string> &_envir_path, bool &_verbose)
 * _alias_list stores alias, _envir_path stores user preset paths, _verbose for verbose settings.
 * This function will change directory to default directory and read preset configuration file.
 * This function supports settings for alias, paths, verbose mode.
 * It will execute configurations line by line.
 * You can use '#' at the first of each line to make comments.
 */
void initialization_file(vector<pair<string, vector<string> > > &_alias_list, vector<string> &_envir_path, bool &_verbose) {
    char current_path[MAX_LINE+1];
    struct passwd *user = getpwuid(getuid());
    chdir(user->pw_dir);
    string filename(".cs543rc");
    ifstream init(filename.c_str());
    string buff;
    vector<vector<string> > file_cmd;
    vector<string> single_line;
    bool quote = false;
    /* If there is no file or the file is empty, this process will not be executed. */
    if (_verbose == true) {
        if (init.is_open() == false) {
            printf("There is no preset configuration file.\n");
        }
    }
    /* Read input file and store all of them. */
    while (init) {
        init>>buff;
        if (buff.length() > MAX_LINE) {
            single_line.clear();
            file_cmd.clear();
            fprintf(stderr, "The commad of %s is out of MAX_LINE bound, please check you file again. This file will not be loaded.\n", buff.c_str());
            buff.clear();
            break;
        }
        if (buff.empty() == false) {
            if ((buff == "%" || buff.at(0) == '#') && single_line.size() > 0) {
                if (single_line.size() <= MAX_LINE/2+1) {
                    file_cmd.push_back(single_line);
                    single_line.clear();
                } else {
                    fprintf(stderr, "The number of command for a single configuration exceeds the bound, please check you file again and initialization file will not be loaded at this time.\n");
                    buff.clear();
                    single_line.clear();
                    file_cmd.clear();
                    break;
                }
            }
            single_line.push_back(buff);
        }
        buff.clear();
    }
    if (single_line.size() > 0) {
        file_cmd.push_back(single_line);
        single_line.clear();
    }
    /* Process them line by line, analyzing their inputs. */
    for (int i = 0; i < file_cmd.size(); i++) {
        char * cmd[MAX_LINE/2+1];
        for (int j = 0; j < file_cmd[i].size(); j++) {
            cmd[j] = (char*)malloc((MAX_LINE+1)*sizeof(char));
            strcpy(cmd[j], file_cmd[i][j].c_str());
        }
        /* Process for alias, path, verbose,respectively. */
        if (file_cmd[i].size()>3 && strcmp(cmd[0], "%")==0 && strcmp(cmd[1], "alias")==0) {
            for (int j = 4; j < file_cmd[i].size(); j++) {
                file_cmd[i][3].append(" "+file_cmd[i][j]);
            }
            file_cmd[i][3] = file_cmd[i][3].substr(1, file_cmd[i][3].size()-2);
            free(cmd[0]);
            cmd[0] = (char*)malloc((MAX_LINE+1)*sizeof(char));
            strcpy(cmd[0], cmd[2]);
            free(cmd[1]);
            cmd[1] = (char*)malloc(file_cmd[i][3].size()*sizeof(char));
            free(cmd[2]);
            cmd[2] = (char*)0;
            strcpy(cmd[1], file_cmd[i][3].c_str());
            set_alias(cmd, _alias_list, _envir_path, _verbose);
        } else if (file_cmd[i].size()>4 && strcmp(cmd[0], "%")==0 && strcmp(cmd[1], "set")==0 && strcmp(cmd[2], "path")==0 && strcmp(cmd[3], "=")==0) {
            if (file_cmd[i].back().at(file_cmd[i].back().size()-1) == ')') {
                cmd[file_cmd[i].size()-1][file_cmd[i].back().size()-1] = '\0';
            } else printf("Expecting a ) at end.\n");
            int size = file_cmd[i].size();
            set_path(cmd, size, _envir_path, _verbose);
        } else if (file_cmd[i].size()==4 && strcmp(cmd[0], "%")==0 && strcmp(cmd[1], "set")==0 && strcmp(cmd[2], "verbose")==0) {
            int size = file_cmd[i].size();
            verbose_property(cmd, size, _verbose);
        }
        for (int j = 0; j < file_cmd[i].size(); j++) {
            free(cmd[j]);
        }
    }
    if (_verbose == true) {
        if (file_cmd.size() > 0) {
            printf("Configurations in the preset input file:\n");
            for (int i = 0; i < file_cmd.size(); i++) {
                for (int j = 0; j < file_cmd[i].size(); j++) {
                    printf("%s ", file_cmd[i][j].c_str());
                }
                printf("\n");
            }
        }
    }
}

/**
 * This is main function of shell. 
 * The shell allows all features mentioned in the homework.
 * It can has configuration options, history, alias, script, paths, preset file, verbose mode. All configuration work will not invoke fork().
 * Normally, the shell will fork a child process to execute programs, if the shell process will wait for child is determined by if a '&' at the end of commands.
 * If there is a need for pipe, the child process will fork another process to execute concurrently.
 */
int main (void) {
    /* Arguments for current commands, commands history. */
    char * args[MAX_LINE/2+1], * cmd_history[10][MAX_LINE/2+1];
    /* should_run determine whether exit while loop, argument_size is number of user input commands, history_counter is number of commands in the history. */
    int should_run = 1, argument_size, history_counter = 0;
    /* Flags to determine working mode. */
    bool invoke_wait , verbose = false, script = false, pipe_exist = false;
    /* Arrays for alias and path list. */
    vector<pair<string,vector<string> > > alias_list;
    vector<string> envir_path;
    /* Initailize char * pointers. */
    pid_t pid;
    for (int i = 0; i < (MAX_LINE/2+1); i++) {
        args[i] = (char*)0;
    }
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < (MAX_LINE/2+1); j++) {
            cmd_history[i][j] = (char*)malloc((MAX_LINE+1)*sizeof(char));
        }
    }
    /* Reading preset configuration file. */
    initialization_file(alias_list, envir_path, verbose);
    while (should_run) {
        printf("osh> ");
        fflush(stdout);
        /* Reading input command from user. */
        argument_size = ReadInput(args, MAX_LINE/2, stdin, verbose);
        /* If user input does not meet required format, argument size is negative, shell will reiterate. */
        if (argument_size < 0) {
            fprintf(stderr, "Please check your input targuments, and type them again.\n");
            argument_size = -argument_size;
        /* If user input is correct in format, the shell begins to process. */
        } else if (argument_size > 0) {
            /* This boolean is a flag for the commands whether is a shell configuration or system program call. True for configuration, false for system program. */
            bool non_exec = false;
            if (argument_size == 1 && args[0][0] == '!') {
                argument_size = 0;
                bool Nth_cmd = false;
                /* If the input is a single one of '!' with numbers, this should be command for history commands execution. */
                for (int i = 0; args[0][i] != '\0'; i++) {
                    if (args[0][i] > 47 && args[0][i] < 58) {
                        Nth_cmd = true;
                        break;
                    }
                }
                /* If the input is '!!', invoke latest command execution. */
                if (Nth_cmd == true) {
                    history_cmd_exec(args, cmd_history, argument_size, history_counter, verbose);
                } else {
                    if (strcmp(args[0], "!!") == 0) {
                        latest_cmd_exec(args, cmd_history, argument_size, history_counter, verbose);
                    }
                }
            }
            /* Additional feature, this is to change directory. */
            if (strcmp(args[0], "cd") == 0) {
                non_exec = true;
                if (argument_size == 2) {
                    if (chdir(args[1]) == -1) {
                        fprintf(stderr, "You path is not correct, directory change failed.\n");
                    }
                } else if (argument_size == 1) {
                    struct passwd *user = getpwuid(getuid());
                    chdir(user->pw_dir);
                } else {
                    printf("Please enter correct cd command.\n");
                }
            }
            /* If the command start with '%', it is a configuration setting. */
            if (args[0][0] == '%' && argument_size > 1) {
                if (strcmp(args[1], "alias") == 0 && argument_size == 4) {
                    /* Remove '%' and 'alias', and then pass user input to set_alias function. */
                    non_exec = true;
                    char ** tmp = args;
                    tmp += 2;
                    free(args[argument_size]);
                    args[argument_size] = (char*)0;
                    set_alias(tmp, alias_list, envir_path, verbose);
                }
                /* This part will handle verbose and path settings. */
                if (strcmp(args[1], "set") == 0 && argument_size > 3) {
                    /* This is for verbose setting. */
                    if (strcmp(args[2], "verbose") == 0) {
                        non_exec = true;
                        verbose_property(args, argument_size, verbose);
                    /* This is for paths setting. */
                    } else if (strcmp(args[2], "path") == 0 && strcmp(args[3], "=") == 0 && argument_size > 4) {
                        non_exec = true;
                        int last_argument_size = 0;
                        while (args[argument_size-1][last_argument_size] != '\0') {
                            last_argument_size++;
                        }
                        /* This setting must in the format of "% set path = ($PATH)",i.e. has '(' at the beginning is args[4]. */
                        if (args[4][0] == '(') {
                            /* Remove the ')' at last, and the function for set path will handle the first '('. */
                            if (args[argument_size-1][last_argument_size-1] == ')') {
                                args[argument_size-1][last_argument_size-1] = '\0';
                            } else printf("Expecting a ) at end.\n");
                            set_path(args, argument_size, envir_path, verbose);
                        }
                    }
                }
                /* The command of "% script filename" should invode script mode. */
                if (argument_size == 3 && strcmp(args[1], "script") == 0) {
                    if (script == false) {
                        script = true;
                        /* This part will create script file and log file. */
                        string logfilename(args[2]);
                        ofstream logfile(logfilename.c_str());
                        while (!logfile.is_open()) {
                            printf("Please enter a valid file path.\n");
                            for (int i = 0; i < argument_size; i++) {
                                free(args[i]);
                                args[i] = (char*)0;
                            }
                            argument_size = ReadInput(args, 1, stdin, verbose);
                        }
                        script_exec(args, argument_size, logfile, verbose);
                        chmod(logfilename.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH|S_IXOTH|S_IXUSR|S_IXGRP);
                        for (int i = 1; i < argument_size; i++) {
                            if (args[i] != (char*)0) {
                                free(args[i]);
                                args[i] = (char*)0;
                            }
                        }
                        args[1] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[1], logfilename.c_str());
                        args[2] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[2], "|");
                        args[3] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[3], "tee");
                        args[4] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[4], "-a");
                        args[5] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[5], logfilename.c_str());
                        args[6] = (char*)0;
                        argument_size = 6;
                    } else printf("Script mode is already on.\n");
                }
                /* If the user enter "% endscript" accidentally, tell user the script mode is not on. */
                if (argument_size == 2 && strcmp(args[1], "endscript") == 0) {
                    if (script == false) {
                        non_exec = true;
                        printf("Script mode is not on.\n");
                    }
                }
            }
            /* Add commands to the history list. */
            if (verbose == true) {
                printf("Recent commands and parameters are added into history.\n");
            }
            add_history_cmd(args, argument_size, cmd_history, history_counter);
            /* When user input "history", invoke history display. */
            if (argument_size == 1 && strcmp(args[0], "history") == 0) {
                non_exec = true;
                if (history_counter > 1) {
                    display_history(cmd_history, history_counter);
                /* If there is no commands in history, print information to screen. */
                } else printf("This is the first command in history.\n");
            }
            /* If user type "exit", change should_run to 0 and terminates the while loop. */
            if (argument_size == 1 && strcmp(args[0], "exit") == 0) {
                should_run = 0;
                non_exec = true;
            }
            /* If the commands is a input needs execvp(), shell forks a child process. */
            if (non_exec == false) {
                int pipe_position = -1;
                /* If the last one is '&', this should invoke wait() for child process. */
                if (strcmp(args[argument_size-1], "&") == 0) {
                    invoke_wait = true;
                    argument_size--;
                    free(args[argument_size]);
                    args[argument_size] = (char*)0;
                }
                /* Check if there is need for pipe communication. */
                if (argument_size > 3 && strcmp(args[0], "%") == 0) {
                    argument_size--;
                    for (int i = 0; i < argument_size; i++) {
                        free(args[i]);
                        args[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                        strcpy(args[i], args[i+1]);
                        if (strcmp(args[i], "|") == 0) {
                            pipe_exist = true;
                            pipe_position = i;
                        }
                    }
                    free(args[argument_size]);
                    args[argument_size] = (char*)0;
                }
                if (script == true) {
                    invoke_wait = true;
                    pipe_exist = true;
                    pipe_position = 2;
                }
                /**
                 * After reading user input, the steps are:
                 * (1) fork a child process using fork()
                 * (2) the child process will invoke execvp()
                 * (3) if command include &, parent will invoke wait()
                 */
                pid = fork();
                if (pid < 0) {
                    fprintf(stderr,"Shell process encountered fork error.\n");
                    return 1;
                } else if (pid == 0) {
                    if (verbose == true) {
                        printf("Shell process has forked another process, pid: %d\n", getpid());
                    }
                    /* If there is command for pipe, establish a pipe and fork another process for concurrently processing. */
                    if (pipe_exist == true) {
                        int pipe_des[2];
                        /* Construct a pipe. */
                        if (pipe_exist == true) {
                            if(pipe(pipe_des) != 0) {
                                fprintf(stderr, "Pipe construction failed.\n");
                                return 2;
                            }
                        }
                        pid_t pid_1 = fork();
                        if (pid_1 < 0) {
                            fprintf(stderr, "Child process encountered fork error.\n");
                            return 1;
                        /* The third process executes the first half of input commands. */
                        } else if (pid_1 == 0) {
                            if (verbose == true) {
                                printf("The third process has been forked, pid: %d \n", getpid());
                            }
                            if (verbose == true) {
                                printf("Establishing pipe connection at the point of pid %d\n", getpid());
                            }
                            close(pipe_des[0]);
                            if (verbose == true) {
                                printf("Splitting input commands for first process.\n");
                            }
                            /* If user did not type firt part of commands(i.e. "% | grep "o" "), shell asks usr to do it. */
                            if (pipe_position != 0) {
                                argument_size = pipe_position;
                                free(args[pipe_position]);
                                args[pipe_position] = (char*)0;
                            } else {
                                for (int i = 0; i < argument_size; i++) {
                                    if (args[i] != (char*)0) {
                                        free(args[i]);
                                        args[i] = (char*)0;
                                    }
                                }
                                printf("Please enter the commands for first process for pipe.\n");
                                while (args[0] == (char*)0) {
                                    argument_size = ReadInput(args, MAX_LINE/2, stdin, verbose);
                                }
                            }
                            if (verbose == true) {
                                printf("The first part of commands are(for pipe communication):\n");
                                for (int i = 0; i < argument_size; i++) {
                                    printf("%s ", args[i]);
                                }
                                printf("\n");
                            }
                            /* After checking alias and paths, shell will execute the commands. */
                            if (verbose == true) {
                                printf("Checking alias for input commands.\n");
                            }
                            check_alias(args, argument_size, alias_list, verbose);
                            if (verbose == true) {
                                printf("Check complete.\nStart path searching.\n");
                            }
                            search_path(args, envir_path, verbose);
                            if (args[argument_size] != (char*)0) {
                                free(args[argument_size]);
                                args[argument_size] = (char*)0;
                            }
                            if (verbose == true) {
                                printf("The process pid: %d is about to finish.\n", getpid());
                            }
                            /* When running under script mode, the output should be copied to tee, in order to get output both on the screen and in the file. */
                            if (script == true) {
                                free(args[1]);
                                args[1] = (char*)malloc(20*sizeof(char));
                                strcpy(args[1], "./.cs543_tmp_script");
                            }
                            /* Redirecting output to second process. */
                            dup2(pipe_des[1], 1);
                            close(pipe_des[1]);
                            if (execvp(args[0], args) == -1) {
                                fprintf(stderr, "Child process pid: %d ends abnormally.\n", getpid());
                            }
                            exit(0);
                        /* The second process executes the second half of input commands. */
                        } else {
                            if (verbose == true) {
                                printf("Process pid: %d prepare to execute.\n", getpid());
                            }
                            if (verbose == true) {
                                printf("Establishing pipe connection at the point of pid %d\n", getpid());
                            }
                            close(pipe_des[1]);
                            if (verbose == true) {
                                printf("Splitting commands for second process.\n");
                            }
                            /* If user did not input commands for second part, shell asks user to type it. */
                            if (pipe_position == argument_size-1) {
                                for (int i = 0; i < argument_size; i++) {
                                    if (args[i] != (char*)0) {
                                        free(args[i]);
                                        args[i] = (char*)0;
                                    }
                                }
                                printf("Please enter commands for second process for pipe.\n");
                                while (args[0] == (char*)0) {
                                    argument_size = ReadInput(args, MAX_LINE/2, stdin, verbose);
                                }
                            } else {
                                for (int i = 0, j = pipe_position+1; j < argument_size; i++, j++) {
                                    free(args[i]);
                                    args[i] = (char*)malloc((MAX_LINE+1)*sizeof(char));
                                    strcpy(args[i], args[j]);
                                    free(args[j]);
                                    args[j] = (char*)0;
                                }
                                argument_size -= (pipe_position + 1);
                                if (args[argument_size] != (char*)0) {
                                    free(args[argument_size]);
                                    args[argument_size] = (char*)0;
                                }
                            }
                            if (verbose == true) {
                                printf("The second part of commands are(for pipe communication):\n");
                                for (int i = 0; i < argument_size; i++) {
                                    printf("%s ", args[i]);
                                }
                                printf("\n");
                            }
                            /* After checking alias and paths, shell executes the commands. */
                            if (verbose == true) {
                                printf("Checking alias for input commands.\n");
                            }
                            check_alias(args, argument_size, alias_list, verbose);
                            if (verbose == true) {
                                printf("Check complete.\nStart path searching.\n");
                            }
                            search_path(args, envir_path, verbose);
                            if (args[argument_size] != (char*)0) {
                                free(args[argument_size]);
                                args[argument_size] = (char*)0;
                            }
                            if (verbose == true) {
                                printf("Process pid %d is waiting for pid %d\n", getpid(), pid_1);
                            }
                            /* When use a pipe, it is better to invoke wait to wait for the other process in order to run concurrently. */
                            waitpid(pid_1, NULL, 0);
                            if (verbose == true) {
                                printf("The process pid: %d is about to finish.\n", getpid());
                            }
                            /* Recieving output from child process. */
                            dup2(pipe_des[0], 0);
                            close(pipe_des[0]);
                            if (execvp(args[0], args) == -1) {
                                fprintf(stderr, "Child process ends abnormally.\n");
                            }
                            exit(0);
                        }
                    /* If there is no need for pipe, child process just execute commands normally. */
                    } else {
                        if (args[argument_size] != (char*)0) {
                            free(args[argument_size]);
                            args[argument_size] = (char*)0;
                        }
                        if (verbose == true) {
                            printf("Checking alias for input commands.\n");
                        }
                        check_alias(args, argument_size, alias_list, verbose);
                        if (verbose == true) {
                            printf("Check complete.\nStart path searching.\n");
                        }
                        search_path(args, envir_path, verbose);
                        if (verbose == true) {
                            printf("The process pid: %d is about to finish.\n", getpid());
                        }
                        if (execvp(args[0], args) == -1) {
                            printf("Child process ends abnormally.\n");
                        }
                    }
                    exit(0);
                } else {
                    /* The shell process, if the commands include a '&' at last, this process should wait for its child. */
                    if (invoke_wait == true || script == true) {
                        if (verbose == true) {
                            printf("Shell process (pid: %d) is waiting for child process (pid: %d).\n", getpid(), pid);
                        }
                        waitpid(pid, NULL, 0);
                    } else {
                        if (verbose == true) {
                            printf("Shell process will not wait for child process.\n");
                        }
                    }
                    /* Remove temporary file generated by script mode. */
                    if (script == true) {
                        remove("./.cs543_tmp_script");
                    }
                    if (verbose == true) {
                        printf("Shell process goes on.\n");
                    }
                }
            }
        }
        /* Reset all configuration flasgs. */
        pipe_exist = false;
        invoke_wait = false;
        script = false;
        /* Delete pointers used malloc() */
        for (int i = 0; i < argument_size; i++) {
            if (args[i] != (char*)0) {
                free(args[i]);
                args[i] = (char*)0;
            }
        }
    }
    /* Free the pointers for history command list. */
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < MAX_LINE/2+1; j++) {
            if (cmd_history[i][j] != (char*)0) {
                free(cmd_history[i][j]);
                cmd_history[i][j] = (char*)0;
            }
        }
    }
    printf("Shell exits successfully.\n");
    return 0;
}
