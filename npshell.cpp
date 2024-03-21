#include <csignal>
#include <iostream>
#include <queue>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
#include <wait.h>
using namespace std;

struct Command {
    string exec_command;
    bool is_error_pipe = false;
    // -1: no pipe after this command; 0:oridinary pipe; >0: number of commands need to be jumped
    int pipe_number;
    pair<int, int> input_pipe;
    pair<int, int> output_pipe;
    pair<int, int> error_pipe;
    // int input_pipe[2];
    // int output_pipe[2];
    // int error_pipe[2];
};

struct Job {
    int job_id;
    bool is_built_in_command = false;
    vector<string> built_in_command;
    queue<Command> command_queue;
};

pair<int, int> create_pipe() {
    // pipefd[0]: read, pipefd[1]: write
    // create pipe
    int pipefd[2];
    int create_pipe = pipe(pipefd);
    // create pipe failed
    while (create_pipe < 0) {
        create_pipe = pipe(pipefd);
    }

    pair<int, int> temp;
    temp.first = pipefd[0];
    temp.second = pipefd[1];
    return temp;
}

void open_pipe(int in, int out) {
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    return;
}

void close_pipe(pair<int, int> p) {
    close(p.first);
    close(p.second);
    return;
}

void handle_built_in_command(vector<string> tokens) {
    // Terminate npshell
    if (tokens[0] == "exit") {
        exit(0);
    } else if (tokens[0] == "setenv") {
        setenv(tokens[1].c_str(), tokens[2].c_str(), 1);
    } else if (tokens[0] == "printenv") {
        const char *environment_variable = getenv(tokens[1].c_str());
        // check the variable exists or not
        if (environment_variable)
            cout << environment_variable << endl;
    }
}

int parse(queue<Job> &job_queue, const string &str, const char &delimiter, int job_id) {

    queue<Command> command_queue;
    Job job;
    Command command;
    string exec_command = "";
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {

        // built-in command arguments
        if (job.is_built_in_command) {
            job.built_in_command.push_back(token);
        }
        // First built-in command
        else if (token == "setenv" || token == "printenv" || token == "exit") {
            job.is_built_in_command = true;
            job.built_in_command.push_back(token);

        }
        // pipe or numbered pipe
        else if (token[0] == '|' || token[0] == '!') {
            // oridinary pipe = 0
            if (token == "|") {

                command.pipe_number = 0;
                command.exec_command = exec_command;
                exec_command = "";
                command_queue.push(command);
                command = Command();
            }
            // numbered pipe
            else if (token[0] == '|') {

                const char *t = token.c_str();
                command.pipe_number = atoi(t + 1);
                command.exec_command = exec_command;
                exec_command = "";
                command_queue.push(command);
                command = Command();
                job_id++;
                job.job_id = job_id;
                job.command_queue = command_queue;
                command_queue = queue<Command>();
                job_queue.push(job);
                job = Job();

            }
            // error pipe
            else if (token[0] == '|' || token[0] == '!') {

                const char *t = token.c_str();
                command.pipe_number = atoi(t + 1);
                command.exec_command = exec_command;
                exec_command = "";
                command.is_error_pipe = true;
                command_queue.push(command);
                command = Command();

                job_id++;
                job.job_id = job_id;
                job.command_queue = command_queue;
                command_queue = queue<Command>();
                job_queue.push(job);
                job = Job();
            }
        }
        // other commands
        else {
            if (exec_command.length() != 0) {
                exec_command += " " + token;

            } else {
                exec_command = token;
            }
        }
    }

    if (job.is_built_in_command) {
        job_id++;
        job.job_id = job_id;
        job_queue.push(job);
        job = Job();
    }
    // last command
    else if (exec_command != "") {
        command.pipe_number = -1; // no pipe
        command.exec_command = exec_command;
        exec_command = "";
        command_queue.push(command);
        command = Command();
        job_id++;
        // cout << "job_id = " << job_id << endl;
        job.job_id = job_id;
        job.command_queue = command_queue;
        // cout << command_queue.size() << endl;
        command_queue = queue<Command>();
        job_queue.push(job);
        job = Job();
    }

    // while (!job_queue.empty()) {
    //     Job c_job = job_queue.front();
    //     cout << "job id = " << c_job.job_id << endl;
    //     ;
    //     while (!c_job.command_queue.empty()) {
    //         Command c_command = c_job.command_queue.front();
    //         cout << "exec command = " << c_command.exec_command;
    //         cout << " ; pipe number = " << c_command.pipe_number << endl;
    //         c_job.command_queue.pop();
    //     }
    //     job_queue.pop();

    // }
    return job_id;
    // return result;
}

vector<string> split(const string &str, const char &delimiter) {
    vector<string> result;
    stringstream ss(str);
    string tok;

    while (getline(ss, tok, delimiter)) {
        result.push_back(tok);
    }

    return result;
}

char **to_char_array(vector<string> input) {
    char **args;
    args = new char *[input.size() + 1];
    for (int i = 0; i < input.size(); i++) {
        if (i == 0) {
            // args[i] = strdup(("bin/"+input[i]).c_str());
            args[i] = strdup((input[i]).c_str());

        } else {
            args[i] = strdup((input[i]).c_str());
        }
    }
    args[input.size()] = NULL;
    return args;
}

void execute(Command command) {

    // fork process
    pid_t pid;
    while (1) {
        pid = fork();
        if (pid >= 0) {
            break;
        }
    }

    // child process
    if (pid == 0) {
        // close other numbered pipes which will be not used in this child process ???

        // first command
        if (command.input_pipe.first == 0) {

            if (command.is_error_pipe) {
                dup2(command.output_pipe.second, STDERR_FILENO);
            }
            dup2(command.output_pipe.second, STDOUT_FILENO);
            close_pipe(command.output_pipe);
        }
        // last command (no pipe after this command)
        else if (command.pipe_number == -1) {
            dup2(command.input_pipe.first, STDIN_FILENO);
            close_pipe(command.input_pipe);
        } else if (command.is_error_pipe) {
            dup2(command.output_pipe.second, STDERR_FILENO);
            open_pipe(command.input_pipe.first, command.output_pipe.second);
            close_pipe(command.output_pipe);
            close_pipe(command.input_pipe);
        } else {
            open_pipe(command.input_pipe.first, command.output_pipe.second);
            close_pipe(command.output_pipe);
            close_pipe(command.input_pipe);
        }
        // exec
        // string c_exec = command.exec_command;
        vector<string> token = split(command.exec_command, ' ');
        char **args = to_char_array(token);
        // string command(args[0]);
        if (execvp(args[0], args) == -1) {
            cerr << "Unknown command: [" << args[0] << "]." << endl;
            exit(0);
        }

    }
    // parent process
    else {
        //  if there is a input pipe, deallocate the file descriptor
        if (command.input_pipe.first != 0) {
            close_pipe(command.input_pipe);
            // wait(NULL);
        }

        // if there is any pipe (including ordinary pipe and numbered pipe) after the command, don't wait
        if (command.pipe_number >= 0) {
            signal(SIGCHLD, SIG_IGN);
            // wait(NULL);

        } else {
            wait(NULL);
        }
        // wait(NULL);
    }
    return;
}

int main() {

    // set initial environment variable
    setenv("PATH", "bin:.", 1);

    string user_input;
    int job_id = 0;
    while (1) {
        cout << "% ";
        getline(cin, user_input);
        if (cin.eof()) {
            exit(0);
        }
        // cout<<user_input<<endl;

        // parse user input
        char delimiter = ' ';
        queue<Job> job_queue;
        job_id = parse(job_queue, user_input, delimiter, job_id);
        cout<<"total job size: "<< job_queue.size()<<endl;
        while (!job_queue.empty()) {
            Job c_job = job_queue.front();
            cout << "job id = " << c_job.job_id << endl;
            if (c_job.is_built_in_command) {
                handle_built_in_command(c_job.built_in_command);
                job_queue.pop();
                continue;
            }
            cout << "command queue size: " << c_job.command_queue.size() << endl;
            while (!c_job.command_queue.empty()) {
                Command c_command = c_job.command_queue.front();
                c_job.command_queue.pop();

                // oridinary pipe after this command
                if (c_command.pipe_number == 0) {
                    // read next command
                    // Command n_command = c_job.command_queue.front();
                    Command *n_command = &c_job.command_queue.front();

                    // cout<<"before create pipe: "<<endl;
                    // cout<<"  current input pipe: "<<c_command.input_pipe.first<<" "<<c_command.input_pipe.second<<endl;
                    // cout<<"  current output pipe: "<<c_command.output_pipe.first<<" "<<c_command.output_pipe.second<<endl<<endl;
                    // Command test_command = c_job.command_queue.front();
                    // cout<<"  next input pipe: "<<test_command.input_pipe.first<<" "<<test_command.input_pipe.second<<endl;

                    // create pipe and assign it to current command output pipe
                    c_command.output_pipe = create_pipe();

                    // cout<<"After create pipe: "<<endl;
                    // cout<<"  current input pipe: "<<c_command.input_pipe.first<<" "<<c_command.input_pipe.second<<endl;
                    // cout<<"  current output pipe: "<<c_command.output_pipe.first<<" "<<c_command.output_pipe.second<<endl<<endl;

                    // assign current command output pipe to next command input pipe
                    n_command->input_pipe = c_command.output_pipe;

                    // Command test_command2 = c_job.command_queue.front();
                    // cout<<"  next input pipe: "<<test_command2.input_pipe.first<<" "<<test_command2.input_pipe.second<<endl;

                    cout << "execute command: " << c_command.exec_command << endl;
                    execute(c_command);
                }
                // no pipe after this command
                else if(c_command.pipe_number == -1){
                    execute(c_command);

                }
            }

            job_queue.pop();
        }
    }

    return 0;
}