#include <csignal>
#include <iostream>
#include <map>
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

pair<int, int> create_numbered_pipe(int des_job_id, map<int, pair<int, int>> &numbered_pipe_map) {
    pair<int, int> temp;
    if ((numbered_pipe_map.find(des_job_id) != numbered_pipe_map.end())) {
        temp.first = numbered_pipe_map[des_job_id].first;
        temp.second = numbered_pipe_map[des_job_id].second;
    } else {
        temp = create_pipe();
    }
    numbered_pipe_map[des_job_id] = temp;
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

vector<string> preprocess(const string &str, const char &delimiter, bool &redirection, string &filename) {
    vector<string> result;
    stringstream ss(str);
    string tok;
    bool flag = false;
    while (getline(ss, tok, delimiter)) {
        if (flag == true) {
            filename = tok;
            flag = false;
        } else if (tok == ">") {
            redirection = true;
            flag = true;
        } else {

            result.push_back(tok);
        }
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

void execute(Command command, map<int, pair<int, int>> &numbered_pipe_map) {

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

        // some data pass to this command through pipe
        if (command.input_pipe.first != 0) {
            dup2(command.input_pipe.first, STDIN_FILENO);
            close_pipe(command.input_pipe);
        }

        // pipe after this command
        if (command.output_pipe.second != 0) {
            // if there is a pipe for stderr after this command, duplicate the file descriptor
            if (command.is_error_pipe)
                dup2(command.output_pipe.second, STDERR_FILENO);
            dup2(command.output_pipe.second, STDOUT_FILENO);
            close_pipe(command.output_pipe);
        }

        // deallocate the file descriptors stored in numbered_pipe_map
        for (map<int, pair<int, int>>::iterator it = numbered_pipe_map.begin(); it != numbered_pipe_map.end(); ++it) {

            close_pipe(it->second);
        }

        // exec
        bool redireciton = false;
        string file_name = "";
        vector<string> token = preprocess(command.exec_command, ' ', redireciton, file_name);

        char **args = to_char_array(token);
        if (redireciton) {
            freopen(file_name.c_str(), "w", stdout);
        }
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
    map<int, pair<int, int>> numbered_pipe_map;
    int job_id = 0;
    while (1) {
        cout << "% ";
        getline(cin, user_input);
        if (cin.eof()) {
            exit(0);
        }

        // parse user input
        char delimiter = ' ';
        queue<Job> job_queue;
        job_id = parse(job_queue, user_input, delimiter, job_id);
        while (!job_queue.empty()) {
            Job c_job = job_queue.front();
            if (c_job.is_built_in_command) {
                handle_built_in_command(c_job.built_in_command);
                job_queue.pop();
                continue;
            }
            // cout << "command queue size: " << c_job.command_queue.size() << endl;
            while (!c_job.command_queue.empty()) {
                Command c_command = c_job.command_queue.front();
                c_job.command_queue.pop();
                // cout << "current execute command: " << c_command.exec_command << endl;

                // oridinary pipe after this command
                if (c_command.pipe_number == 0) {

                    // read next command
                    Command *n_command = &c_job.command_queue.front();

                    // create pipe and assign it to current command output pipe
                    c_command.output_pipe = create_pipe();

                    // assign current command output pipe to next command input pipe
                    n_command->input_pipe = c_command.output_pipe;
                }
                // numbered pipe
                else if (c_command.pipe_number > 0) {
                    c_command.output_pipe = create_numbered_pipe(c_job.job_id + c_command.pipe_number, numbered_pipe_map);
                    // cout << "new numbered pipe" << c_command.output_pipe.first << " " << c_command.output_pipe.second << endl;
                }

                // if there exists numbered piped which send data to this command
                if (numbered_pipe_map.find(c_job.job_id) != numbered_pipe_map.end()) {
                    // cout << "someone send data!!!!!!!!!!!!!" << endl;
                    // cout << "map data" << endl;
                    for (map<int, pair<int, int>>::const_iterator it = numbered_pipe_map.begin();
                         it != numbered_pipe_map.end(); ++it) {
                        // std::cout <<"job_id: "<< it->first << "pipe value: " << it->second.first << " " << it->second.second << "\n";
                    }

                    c_command.input_pipe = numbered_pipe_map[c_job.job_id];
                    numbered_pipe_map.erase(c_job.job_id);
                    // cout << "check input pipe: " << c_command.input_pipe.first << " " << c_command.input_pipe.second << endl;
                }

                // if no pipe after this command, execute directly
                execute(c_command, numbered_pipe_map);
            }

            job_queue.pop();
        }
    }

    return 0;
}