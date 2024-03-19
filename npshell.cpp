#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include <sstream>
using namespace std;


vector<string> split(const string &str, const char &delimiter)
{
    vector<string> result;
    stringstream ss(str);
    string tok;

    while (getline(ss, tok, delimiter))
    {
        result.push_back(tok);
    }

    return result;
}

bool handle_built_in_command(vector<string> tokens)
{
    // Terminate npshell
    if (tokens[0] == "exit")
    {
        exit(0);
    }
    else if (tokens[0] == "setenv")
    {
        setenv(tokens[1].c_str(), tokens[2].c_str(), 1);
        return true;
    }
    else if (tokens[0] == "printenv")
    {
        const char *environment_variable = getenv(tokens[1].c_str());
        // check the variable exists or not
        if (environment_variable)
            cout << environment_variable << endl;
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{

    // set initial environment variable
    setenv("PATH", "bin:.", 1);

    string user_input;
    while (1)
    {
        cout << "% ";
        getline(cin, user_input);
        if (cin.eof())
        {
            exit(0);
        }
        // cout<<user_input<<endl;

        // split user input
        char delimiter = ' ';
        vector<string> tokens = split(user_input, delimiter);
        // for(int i=0; i<tokens.size();i++){
        //     cout<<tokens[i]<<endl;
        // }

        // built-in command
        if (handle_built_in_command(tokens))
        {
            continue;
        }
        else{ // others

        }
    }

    return 0;
}