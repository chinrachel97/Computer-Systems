#include <iostream>
#include <cctype>
#include <string>
#include <cstdio>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>

using namespace std;

// global variables
string CMD = "";
int INDEX;

// get the next token
string next_token(){
	for(INDEX; INDEX<CMD.size();){
		switch(CMD[INDEX]){
			case '<':
				++INDEX;
				return "<";
			case '>':
				++INDEX;
				return ">";
			case '&':
				++INDEX;
				return "&";
			case '|':
				++INDEX;
				return "|";
			case ' ':
				++INDEX;
				break;
			case '\'':{
				string squote = "";
				++INDEX;
				while(CMD[INDEX] != '\''){
					squote += CMD[INDEX];
					++INDEX;
				}
				++INDEX;
				return squote;
			}
			case '"':{
				string dquote = "";
				++INDEX;
				while(CMD[INDEX] != '"'){
					dquote += CMD[INDEX];
					++INDEX;
				}
				++INDEX;
				return dquote;
			}
			case '$':{
				if(INDEX+1 >= CMD.size() || CMD[INDEX+1] != '(')
					break;
				string dsign = "";
				dsign += "$";
				++INDEX;
				while(CMD[INDEX] != ')'){
					dsign += CMD[INDEX];
					++INDEX;
				}
				dsign += ')';
				++INDEX;
				return dsign;
			}
			default:
				string word = "";
				while(isalnum(CMD[INDEX]) || 
					CMD[INDEX] == '-' ||
					CMD[INDEX] == '/' ||
					CMD[INDEX] == '=' || 
					CMD[INDEX] == '.'){
					word += CMD[INDEX];
					++INDEX;
				}
				if(word.size() == 0) 
					++INDEX;
				return word;
		}
	}
	return "";
}

int main(){
	vector<vector<string>> outer_vec;
	char buf[100] = "";
	vector<char*> history;
	char* h = getcwd(buf, 100);
	history.push_back(h);
	cout << "Hello there.\n";
	
	while(true){
		// reset
		INDEX = 0;
		string tk = "";
		outer_vec.clear();

		// get the command
		cout << "Please type in a command.\n$ ";
		getline(cin, CMD);
		
		// parse the command 
		vector<string> inner_vec;
		while(INDEX < CMD.size()){	
			tk = next_token();
			if(tk == "exit")
				return 0;
			else if(tk != "|")
				inner_vec.push_back(tk);
			else{
				outer_vec.push_back(inner_vec);
				inner_vec.clear();
			}
		}
		outer_vec.push_back(inner_vec);
		
		// validate redirects
		for(int i=0; i<outer_vec.size(); ++i)
			for(int j=0; j<outer_vec[i].size(); ++j){
				string curr = outer_vec[i][j];
				if((curr == "<" && i != 0) ||
					curr == ">" && i != outer_vec.size()-1){
					cout << "ERROR: Invalid redirect.\n";
					return -1;
				}
			}
			
		// pipe setup
		int fds[2*(outer_vec.size())];
		for(int i=0; i<outer_vec.size(); ++i)
			pipe(&fds[2*i]);
		
		// execute parts in order
		for(int i=0; i<outer_vec.size(); ++i){
			int child_status;
			int pid = fork();
			if(pid == 0){
				
				// not a special command
				if(outer_vec[i].size()>0 && outer_vec[i][0] != "cd"){
						
					// redirect input to read from prev part
					if(i != 0){
						dup2(fds[2*(i-1)], 0);
						close(fds[2*(i-1)]);
					}
					
					// redirect output to the next process
					if(i != outer_vec.size()-1){
						dup2(fds[(2*i)+1], 1);
						close(fds[(2*i)+1]);
					}

					// execute
					char* c_cmd = (char*)outer_vec[i][0].c_str();
					char* args[outer_vec[i].size()+1];
					args[0] = (char*)"eh";
					args[outer_vec[i].size()] = NULL;
					char* args_backup[2] = {(char*)"eh", NULL};
					for(int j=1; j<outer_vec[i].size(); ++j){
						// check for file I/O redirects
						if(outer_vec[i][j] == "<"){
							if(j+1 >= outer_vec[i].size()){
								cout << "ERROR: filename not given.\n";
								exit(0);
							}
							char* filename = (char*)outer_vec[i][j+1].c_str();
							int fd = open(filename, O_RDONLY);
							dup2(fd, 0);
							execvp(c_cmd, args);
						}
						else if(outer_vec[i][j] == ">"){
							if(j+1 >= outer_vec[i].size()){
								cout << "ERROR: filename not given.\n";
								exit(0);
							}
							char* filename = (char*)(outer_vec[i][j+1].c_str());
							int fd = open(filename, O_WRONLY|O_CREAT, 
								S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							dup2(fd, 1);
							if(outer_vec[i].size() == 3){
								execvp(c_cmd, args_backup);
							}
							char* args_inp[j+1];
							for(int k=1; k<j+1; ++k)
								args_inp[k] = args[k];
							args_inp[0] = (char*)"eh";
							args_inp[j] = NULL;
							execvp(c_cmd, args_inp);
						}
						else if(outer_vec[i][j] == "&"){
							int inner_child_status;
							int inner_pid = fork();
							if(inner_pid == 0){
								char* bg_cmd = (char*)outer_vec[i][0].c_str();
								args[outer_vec[i].size()-1] = NULL;
								setpgid(0, 0);
								execvp(bg_cmd, args);
							}
							else{
								waitpid(-1, &inner_child_status, WNOHANG);
								exit(0);
							}
						}
						else
							args[j] = (char*)outer_vec[i][j].c_str();
					}
					execvp(c_cmd, args);
				}
				else if(outer_vec[i].size()>0 && outer_vec[i][0] == "cd"){
					if(outer_vec[i].size() == 1){
						history.push_back(getenv("HOME"));
						chdir(getenv("HOME"));
					}
					else if(outer_vec[i].size() == 2 && outer_vec[i][1] == "-"){
						if(history.size()-2 >= 0){
							chdir(history[history.size()-2]);
							history.push_back(history[history.size()-1]);
						}
					}
					else{
						char* path = (char*)outer_vec[i][1].c_str();
						history.push_back(path);
						chdir(path);
					}
				}
			}
			else{
				wait(&child_status);
				if(outer_vec.size() > 1){
					close(fds[2*(i-1)]);
					close(fds[(2*i)+1]);
				}
			}
		}
	}
	
	return 0;
}