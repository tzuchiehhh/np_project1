#include <iostream>
#include <queue>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
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
        while (!job_queue.empty()) {
            Job c_job = job_queue.front();
            cout << "job id = " << c_job.job_id << endl;
            if (c_job.is_built_in_command) {
                handle_built_in_command(c_job.built_in_command);
                job_queue.pop();
                continue;
            }
            while (!c_job.command_queue.empty()) {
                Command c_command = c_job.command_queue.front();
                cout << "exec command = " << c_command.exec_command;
                cout << " ; pipe number = " << c_command.pipe_number << endl;
                c_job.command_queue.pop();
            }

            job_queue.pop();
        }
    }

    return 0;
}