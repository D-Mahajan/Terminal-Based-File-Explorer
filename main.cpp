#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <algorithm>
#include <termios.h>
#include <dirent.h>
#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <fcntl.h>
#include <string>
#include <vector>
#include <time.h>
#include <stack>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

#define ESC 27
#define pos() printf("%c[%d;%dH", ESC, xcur, ycur)

//GLOBAL VARIABLES
char *root;
unsigned int rowSize, colSize;
vector<string> dirList, cmdList;
int searchflag = 0;
unsigned int totalFiles;
struct termios orig_termios;
int scroll_track;
char *cur_Path;
int lastLine = rowSize + 1;
stack<string> back, forw;
int place_holder;
int xcur, ycur;

//Global Functions
void showError(string str);
void print_directory(const char *dirName, const char *root);
void open_directory(const char *path);
int getFilePrintingcount();
void open_dir(const char *path);
int getDirectoryCount(const char *path);
void enable_Raw();
void using_cursor(int &, int &);
int command_mode();
void copy_dir(string, string);
void copy_file(string, string);

int main()
{

	string s = ".";
	char *path = new char[s.length() + 1];
	strcpy(path, s.c_str());
	root = path;
	open_dir(".");
	cur_Path = root;

	enable_Raw();

	return 0;
}

//-----------------------NORMAL MODE---------------------------------------------//
void enable_Raw()
{
	struct termios original, newval;
	char key;

	tcgetattr(STDIN_FILENO, &original);

	struct termios raw = original;
	raw.c_lflag &= ~(ECHO | ICANON);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
	{
		cout << "Could not set attributes for termios" << endl;
		return;
	}

	printf("%c[1;88H", ESC); //positioning of cursor
	xcur = 1, ycur = 88;

	while (1)
	{

		printf("%c[%d;%dH", ESC, getFilePrintingcount() + 1, 1);
		cout << "-----NORMAL MODE-----";

		pos();

		key = cin.get();
		if (key == 24)
		{							//--ctrl+x
			printf("\033[H\033[J"); //------------used to clear the screen.
			break;
		}
		else if (key == 27)
		{

			using_cursor(xcur, ycur);
		}
		else if (key == 'l')
		{ //scrolling down
			if (rowSize + scroll_track >= totalFiles)
				continue;
			printf("\033[H\033[J");		   //------------used to clear the screen.
			printf("%c[%d;%dH", 27, 1, 1); // sets cursor to 1st row, 1st column
			scroll_track++;

			int from = scroll_track;

			int to = (totalFiles <= rowSize) ? totalFiles : rowSize + scroll_track;

			unsigned i = from;
			while (i < to)
			{
				char *tempFileName = new char[dirList[i].length() + 1];
				strcpy(tempFileName, dirList[i++].c_str());

				print_directory(tempFileName, cur_Path);
			}
		}

		else if (key == 'k')
		{ //scrolling up
			if (rowSize + scroll_track <= rowSize)
				continue;
			printf("\033[H\033[J");		   //------------used to clear the screen.
			printf("%c[%d;%dH", 27, 1, 1); // sets cursor to 1st row, 1st column
			--scroll_track;

			int from = scroll_track;

			int to = (totalFiles <= rowSize) ? totalFiles : rowSize + scroll_track;

			unsigned i = from;
			while (i < to)
			{
				char *tempFileName = new char[dirList[i].length() + 1];
				strcpy(tempFileName, dirList[i++].c_str());

				print_directory(tempFileName, cur_Path);
			}
		}

		else if (key == 10)
		{ //ENTER IS PRESSED
			int pos = xcur - 1 + scroll_track;
			string choice = dirList[pos], fullPath;
			if (searchflag != 1)
				fullPath = (string)cur_Path + (string) "/" + choice;
			else
				fullPath = choice;

			struct stat sb;

			char *path = new char[fullPath.length() + 1];
			strcpy(path, fullPath.c_str());

			stat(path, &sb);

			if ((sb.st_mode & S_IFMT) == S_IFREG) ////If file type is Regular File
			{

				int fileOpen = open("/dev/null", O_WRONLY);
				dup2(fileOpen, 2);
				close(fileOpen);
				pid_t processID = fork();
				if (processID != 0)
					continue;
				else if (!processID)
				{
					execlp("xdg-open", "xdg-open", path, NULL);
					exit(0);
				}
			}
			else if ((sb.st_mode & S_IFMT) == S_IFDIR) //If file type is Directory
			{
				if (choice == string("."))
					continue;

				xcur = 1;
				searchflag = 0;

				if (choice == string(".."))
				{

					while (!forw.empty()) //Clearing forw stack
						forw.pop();
					back.push(string(cur_Path));

					{ //setting current path as
						size_t index;
						index = ((string)cur_Path).find_last_of("/\\");
						string new_temp = ((string)cur_Path).substr(0, index);
						strcpy(cur_Path, new_temp.c_str());
					}
				}
				else
				{
					if (cur_Path != NULL)
					{

						while (!forw.empty()) //Clearing forw stack
							forw.pop();
						back.push(string(cur_Path));
					}
					cur_Path = path;
				}

				open_dir(cur_Path);
			}
			else
			{
				showError("Unknown File !!! :::::" + string(choice));
			}
		}

		else if (key == 72 || key == 104) //HOME pressed
		{
			string temp_path = string(cur_Path);
			if (temp_path != string(root))
			{

				while (!forw.empty()) //Clearing forw stack
					forw.pop();
				searchflag = 0;
				back.push(string(cur_Path));
				strcpy(cur_Path, root);

				open_dir(cur_Path);
				xcur = 1;
				ycur = 88;
				printf("%c[%d;%dH", ESC, xcur, ycur);
			}
		}

		else if (key == 127) //BACKSPACE
		{

			string cpath = string(cur_Path);
			if (searchflag != 1 && (strcmp(cur_Path, root) != 0))
			{

				while (!forw.empty()) //Clearing forw stack
					forw.pop();
				back.push(cur_Path);

				{ //setting current path as
					size_t index;
					index = ((string)cur_Path).find_last_of("/\\");
					string new_temp = ((string)cur_Path).substr(0, index);
					strcpy(cur_Path, new_temp.c_str());
				}
				open_dir(cur_Path);
				xcur = 1;
				ycur = 88;
				pos();
			}
		}

		else if (key == 58)
		{ //COMMAND MODE

			struct winsize win;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
			place_holder = win.ws_row - 1;
			printf("%c[%d;%dH", ESC, place_holder, 1);
			printf("%c[2K", 27);
			cout << "-----COMMAND MODE-----(Press ESC to exit)";
			printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
			int result = command_mode();
			if (result == 2)
			{
				break;
			}
			else
			{
				if (result == 1)
				{
					open_dir(cur_Path);
				}
				else
				{

					searchflag = 0;
					open_dir(cur_Path);
				}
			}
			printf("%c[%d;%dH", ESC, place_holder, 1);
			printf("%c[2K", 27);
		}
	}

	//reset terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

//-----------------------COMMAND MODE--------------------------------------------//

int search(string from, string file_name)
{
	DIR *root_dir = opendir(from.c_str());

	if (!root_dir)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return 2;
	}
	else
	{
		struct dirent *element;
		while (element = readdir(root_dir))
		{
			if (((string)element->d_name == ".") || ((string)element->d_name == ".."))
				continue;
			else
			{
				string name = element->d_name;
				if (name == file_name)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "True";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
					return 0;
				}
				string src = from + "/" + (string)element->d_name;
				struct stat sb;
				int x = stat(src.c_str(), &sb);

				if (S_ISDIR(sb.st_mode))
				{
					int status = search(src, file_name);
					if (status == 0)
						return status;
				}
			}
		}
	}
	closedir(root_dir);

	return 1;
}

void del_dir(string to_path)
{

	DIR *dir;
	dir = opendir(to_path.c_str());
	if (!dir)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return;
	}

	struct dirent *element;
	while (element = readdir(dir))
	{
		if (((string)element->d_name == ".") || ((string)element->d_name == ".."))
			continue;
		else
		{
			string src = to_path + "/" + (string)element->d_name;

			struct stat sb;
			int x = stat(src.c_str(), &sb);

			if (x)
			{
				printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
				cout << "\033[0;31m"
					 << "ERROR IN DELETING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
				cout << "\033[0m";
				printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				continue;
			}

			if (!(S_ISDIR(sb.st_mode)))
			{ //file
				if (remove(src.c_str()) != 0)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN DELETING FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
			}
			else
			{
				del_dir(src);
			}
		}
	}
	closedir(dir);
	if (remove(to_path.c_str()) != 0)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN DELETING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
	}

	return;
}

void copy_file(string from_path, string to_path)
{

	FILE *from, *to;
	if ((to = fopen(to_path.c_str(), "w+")) == NULL)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN DESTINATION. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return;
	}

	if ((from = fopen(from_path.c_str(), "r")) == NULL)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN SOURCE. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return;
	}

	char ch;
	while (!feof(from))
	{
		ch = fgetc(from);
		if (ferror(from))
		{
			printf("%c[%d;%dH", ESC, place_holder - 1, 1);
			cout << "ERROR IN SOURCE HANDLING";
			exit(1);
		}
		if (!feof(from))
			fputc(ch, to);
		if (ferror(from))
		{
			printf("%c[%d;%dH", ESC, place_holder - 1, 1);
			cout << "ERROR IN WRITING TO DESTINATION";
			exit(1);
		}
	}

	struct stat ss, dd;
	stat(from_path.c_str(), &ss);
	stat(to_path.c_str(), &dd);
	int status = chown(to_path.c_str(), ss.st_uid, ss.st_gid);
	if (status)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		cout << "ERROR IN OWNERSHIP USING chown";
	}
	status = chmod(to_path.c_str(), ss.st_mode);
	if (status)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		cout << "ERROR IN OWNERSHIP USING chmod";
	}

	fclose(from);
	fclose(to);
}

void copy_dir(string from_path, string to_path)
{
	int status = mkdir(to_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (status == -1)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN MAKING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return;
	}

	DIR *dir;
	dir = opendir(from_path.c_str());
	if (!dir)
	{
		printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
		cout << "\033[0;31m"
			 << "ERROR IN DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
		cout << "\033[0m";
		printf("%c[%d;%dH", ESC, place_holder - 1, 1);
		return;
	}
	struct dirent *element;
	while (element = readdir(dir))
	{
		if (((string)element->d_name == ".") || ((string)element->d_name == ".."))
			continue;
		else
		{
			string src = from_path + "/" + (string)element->d_name;
			string dest = to_path + "/" + (string)element->d_name;

			struct stat sb;
			int x = stat(src.c_str(), &sb);
			if (x)
			{
				printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
				cout << "\033[0;31m"
					 << "ERROR IN MAKING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
				cout << "\033[0m";
				printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				copy_file(src, dest);
				continue;
			}

			if (!(S_ISDIR(sb.st_mode)))
			{
				copy_file(src, dest);
			}
			else if ((S_ISDIR(sb.st_mode)))
			{

				copy_dir(src, dest);
			}
		}
	}

	closedir(dir);
	return;
}

string pathProcessing(string str)
{
	string abs_path = "";
	string base_path = string(root);
	char f_ch = str[0];

	if (f_ch == '~')
	{
		abs_path = base_path + str.substr(1, str.length());
	}
	else if (f_ch == '/')
	{
		abs_path = base_path + str;
	}
	else if (f_ch == '.')
	{
		abs_path = string(cur_Path) + str.substr(1, str.length());
	}
	else
	{
		abs_path = string(cur_Path) + "/" + str;
	}

	return abs_path;
}

int command_mode()
{
	char key;
	key = getchar();
	while (key == 10 || key == 127)
	{
		key = getchar();
	}
	cmdList.clear();

	while (key != 27 && key != 24)
	{
		cmdList.clear();
		cout << key;
		string input = ""; //MANAGING INPUT COMMAND
		input = input + key;

		while (((key = getchar()) != 10) && key != 27)
		{
			if (key == 24)
				return 0;

			if (key == 127)
			{
				//clear input line;
				printf("%c[%d;%dH", 27, place_holder - 1, 1);
				printf("%c[2K", 27);
				for (int i = 0; i < cmdList.size(); i++)
					cout << cmdList[i] << " ";

				if (input.length() <= 1)
				{

					input = "";
				}
				else
				{
					input = input.substr(0, input.length() - 1);
				}
				cout << input;
			}
			else if (key == ' ')
			{

				cmdList.push_back(input);

				cout << key;
				input = "";
			}
			else
			{
				input = input + key;
				cout << key;
			}
		}
		if (key == 27)
		{
			return 0;
		}
		if (key == 10)
		{ //pressing enter after typing command
			cmdList.push_back(input);
			printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
			printf("%c[2K", 27);
			printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor

			vector<string> address_vector;
			for (int i = 1, k = 0; i < cmdList.size(); i++, k++)
			{
				if (cmdList[0] != "create_file" && cmdList[0] != "create_dir" && cmdList[0] != "search")
				{
					string s = pathProcessing(cmdList[i]);
					address_vector.push_back(s);
				}
			}

			if (cmdList[0] == "rename")
			{
				if (cmdList.size() != 3)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN RENAME COMMAND. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";

					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
				}
				else
				{
					string initName = cmdList[1];
					string finalName = cmdList[2];
					int x = rename(initName.c_str(), finalName.c_str());

					if (x == -1)
					{
						printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
						cout << "\033[0;31m"
							 << "ERROR IN RENAME COMMAND. PRESS ANY KEY TO RE-ENTER COMMAND";
						cout << "\033[0m";

						printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					}
				}
			}

			else if (cmdList[0] == "create_file")
			{
				if (cmdList.size() <= 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN CREATING FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					int size = cmdList.size();
					string dest = pathProcessing(cmdList[size - 1]);

					for (int i = 1; i < size - 1; i++)
					{
						string temp = dest + "/" + cmdList[i];
						FILE *file;
						file = fopen(temp.c_str(), "w+");

						if (!file)
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "ERROR IN CREATING FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
							cout << "\033[0m";
							fclose(file);
							break;
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
						fclose(file);
					}
				}
			}

			else if (cmdList[0] == "create_dir")
			{
				if (cmdList.size() <= 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN CREATING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					int size = cmdList.size();
					string dest = pathProcessing(cmdList[size - 1]);

					for (int i = 1; i < size - 1; i++)
					{
						string d = dest + "/" + cmdList[i];

						if (mkdir(d.c_str(), 0755) != 0)
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "ERROR IN CREATING DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
							cout << "\033[0m";

							break;
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
					}
				}
			}

			else if (cmdList[0] == "copy")
			{

				if (cmdList.size() <= 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN COPYING. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					for (int i = 0; i < address_vector.size() - 1; i++)
					{
						string new_data = address_vector[i];

						char *from_path = new char[new_data.length() + 1];
						strcpy(from_path, new_data.c_str());

						size_t index; // taking file name
						index = new_data.find_last_of("/\\");
						string new_name = new_data.substr(index, new_data.length()); //this hasname as /ABC

						string temp_dest = address_vector[address_vector.size() - 1] + new_name;
						char *to_path = new char[temp_dest.length() + 1];
						strcpy(to_path, temp_dest.c_str());

						struct stat ss;
						int x = stat(from_path, &ss);

						if (x == -1)
							perror("lstat");

						if (S_ISDIR(ss.st_mode))
						{
							//copy directory
							copy_dir(new_data, temp_dest);
						}
						else
						{
							//copy file
							copy_file(new_data, temp_dest);
						}
					}
				}
			}

			else if (cmdList[0] == "delete_file")
			{
				if (cmdList.size() < 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN DELETING. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					for (int i = 0; i < address_vector.size(); i++)
					{
						string to_path = address_vector[i];

						char *path = new char[to_path.length() + 1];
						strcpy(path, to_path.c_str());

						struct stat ss;
						int x = stat(path, &ss);
						if (x == -1)
							perror("lstat");

						if (S_ISDIR(ss.st_mode))
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "PATH IS NOT A FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
							cout << "\033[0m";
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
						else
						{
							if (remove(to_path.c_str()) != 0)
							{
								printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
								cout << "\033[0;31m"
									 << "ERROR IN DELETING FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
								cout << "\033[0m";
								printf("%c[%d;%dH", ESC, place_holder - 1, 1);
							}
						}
					}
				}
			}

			else if (cmdList[0] == "delete_dir")
			{
				if (cmdList.size() < 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN DELETING. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					for (int i = 0; i < address_vector.size(); i++)
					{
						string to_path = address_vector[i];

						struct stat ss;
						int x = stat(to_path.c_str(), &ss);
						if (x == -1)
							perror("lstat");

						if (S_ISDIR(ss.st_mode))
							del_dir(to_path);
						else
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "PATH IS NOT A DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
							cout << "\033[0m";
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
					}
				}
			}

			else if (cmdList[0] == "move")
			{
				if (cmdList.size() < 3)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN MOVING. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					for (int i = 0; i < address_vector.size() - 1; i++)
					{
						string new_data = address_vector[i];

						char *from_path = new char[new_data.length() + 1];
						strcpy(from_path, new_data.c_str());

						size_t index; // taking file name
						index = new_data.find_last_of("/\\");
						string new_name = new_data.substr(index, new_data.length()); //this hasname as /ABC

						string temp_dest = address_vector[address_vector.size() - 1] + new_name;
						char *to_path = new char[temp_dest.length() + 1];
						strcpy(to_path, temp_dest.c_str());

						struct stat ss;
						int x = stat(from_path, &ss);

						if (x == -1)
							perror("lstat");

						if (S_ISDIR(ss.st_mode))
						{

							copy_dir(new_data, temp_dest);
							del_dir(new_data);
						}
						else
						{
							//copy file
							copy_file(new_data, temp_dest);
							if (remove(new_data.c_str()) != 0)
							{
								printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
								cout << "\033[0;31m"
									 << "ERROR IN DELETING FILE. PRESS ANY KEY TO RE-ENTER COMMAND";
								cout << "\033[0m";
								printf("%c[%d;%dH", ESC, place_holder - 1, 1);
							}
						}
					}
				}
			}

			if (cmdList[0] == "goto")
			{
				if (cmdList.size() != 2)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN GOTO. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{

					string temp = address_vector[0];
					if (temp == "./" || temp == "~/")
					{
						printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
						cout << "\033[0;31m"
							 << "ALREADY IN SAME DIRECTORY. PRESS ANY KEY TO RE-ENTER COMMAND";
						cout << "\033[0m";
						printf("%c[%d;%dH", ESC, place_holder - 1, 1);
					}
					else
					{
						struct stat ss;
						int x = stat(temp.c_str(), &ss);

						if (x == -1)
							perror("lstat");

						if (S_ISDIR(ss.st_mode))
						{
							while (!forw.empty()) //Clearing forw stack
								forw.pop();
							back.push(string(cur_Path));
							char *path = new char[temp.length() + 1];
							strcpy(path, temp.c_str());
							cur_Path = path;

							open_dir(cur_Path);
							printf("%c[%d;%dH", ESC, getFilePrintingcount() + 1, 1);
							cout << "-----NORMAL MODE-----";
							struct winsize win;
							ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
							place_holder = win.ws_row - 1;
							printf("%c[%d;%dH", ESC, place_holder, 1);
							printf("%c[2K", 27);
							cout << "-----COMMAND MODE-----(Press ESC to exit)";
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							xcur = 1, ycur = 88;
						}
						else
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "ERROR IN DIRECORY ADDRESS. PRESS ANY KEY TO RE-ENTER COMMAND";
							cout << "\033[0m";
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
					}
				}
			}

			else if (cmdList[0] == "search")
			{
				if (cmdList.size() <= 1)
				{
					printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
					cout << "\033[0;31m"
						 << "ERROR IN SEARCH. PRESS ANY KEY TO RE-ENTER COMMAND";
					cout << "\033[0m";
					printf("%c[%d;%dH", ESC, place_holder - 1, 1);
				}
				else
				{
					int size = cmdList.size();

					string file_name = cmdList[size - 1];
					if (file_name == "")
					{
						printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
						cout << "\033[0;31m"
							 << "ERROR IN SEARCH. PRESS ANY KEY TO RE-ENTER COMMAND";
						cout << "\033[0m";
						printf("%c[%d;%dH", ESC, place_holder - 1, 1);
					}
					else
					{
						int x = search(cur_Path, file_name);
						if (x == 1)
						{
							printf("%c[%d;%dH", ESC, place_holder - 1, 1); //positioning of cursor
							cout << "\033[0;31m"
								 << "False";
							cout << "\033[0m";
							printf("%c[%d;%dH", ESC, place_holder - 1, 1);
						}
					}
				}
			}
		}

		key = getchar();
		printf("%c[2K", 27);
		while (key == 10 || key == 127)
		{
			key = getchar();
		}
	}
	return 0;
}

//-----------------------USING CURSOR--------------------------------------------//

void using_cursor(int &xcur, int &ycur)
{ //Addition to the prev implementation **********************

	char c = cin.get();
	c = cin.get();
	if (c == 'A')
	{ //------UP ARROW
		xcur--;
		if (xcur <= 0)
		{
			xcur++;
			return;
		}
		pos();
	}
	else if (c == 'B')
	{ //Down Arrow
		//xcur++;
		int len = getFilePrintingcount() - 1;

		if (xcur <= len)
			xcur++;
		pos();
	}
	else if (c == 'D') //left arrow
	{

		if (back.empty())
			return;
		else
		{
			ycur = 88;
			xcur = 1;
			string cpath = string(cur_Path);

			if (searchflag == 1)
			{
			}
			else
			{
				forw.push(string(cur_Path));
			}
			searchflag = 0;
			string top = back.top();
			back.pop();
			strcpy(cur_Path, top.c_str());

			open_dir(cur_Path);

			pos();
		}
	}
	else if (c == 'C')
	{ //right arrow
		if (!forw.empty())
		{
			ycur = 88;
			xcur = 1;
			string cpath = string(cur_Path);
			if (searchflag == 1)
			{
			}
			else
				back.push(string(cur_Path));
			searchflag = 0;
			string top = forw.top();
			forw.pop();
			strcpy(cur_Path, top.c_str());

			open_dir(cur_Path);

			pos();
		}
	}
}

//-----------------------PRINT DIRECTORY-----------------------------------------//
int getFilePrintingcount()
{
	int lenRecord;
	struct winsize win;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
	rowSize = win.ws_row - 12;
	colSize = win.ws_col;
	if (totalFiles > rowSize)
		lenRecord = rowSize;
	else
		lenRecord = totalFiles;

	return lenRecord;
}

void showError(string str)
{

	cout << endl;
	cout << "\033[0;31m" << str << endl;
	cout << "\033[0m";
	cout << ":";
}

int getDirectoryCount(const char *path)
{
	int count = 0;
	dirList.clear();
	DIR *d;
	struct dirent *dir;
	d = opendir(path);

	if (!d)
	{
		showError("Directory does not exit:::");
	}
	else
	{
		while ((dir = readdir(d)) != NULL)
		{

			if (!((string(dir->d_name) == "..") && (strcmp(path, root) == 0)))

			{

				dirList.push_back(string(dir->d_name));
				count++;
			}
		}
		closedir(d);
	}
	return count;
}

void print_directory(const char *dirName, const char *root)
{

	struct stat sb;
	struct passwd *tf;
	struct group *gf;
	string temp_path;
	if (searchflag != 1)
		temp_path = string(root) + "/" + string(dirName);

	else
		temp_path = string(dirName);

	char *path = new char[temp_path.length() + 1];
	strcpy(path, temp_path.c_str());

	int x = stat(path, &sb);

	if (x == -1)
		perror("lstat");

	printf((S_ISDIR(sb.st_mode)) ? "d" : "-");
	printf((sb.st_mode & S_IRUSR) ? "r" : "-");
	printf((sb.st_mode & S_IWUSR) ? "w" : "-");
	printf((sb.st_mode & S_IXUSR) ? "x" : "-");
	printf((sb.st_mode & S_IRGRP) ? "r" : "-");
	printf((sb.st_mode & S_IWGRP) ? "w" : "-");
	printf((sb.st_mode & S_IXGRP) ? "x" : "-");
	printf((sb.st_mode & S_IROTH) ? "r" : "-");
	printf((sb.st_mode & S_IWOTH) ? "w" : "-");
	printf((sb.st_mode & S_IXOTH) ? "x" : "-");

	tf = getpwuid(sb.st_uid);
	printf("\t%-8s ", tf->pw_name); //-------alligns string to the left: "abc-----"

	gf = getgrgid(sb.st_gid);
	printf("\t%-8s ", gf->gr_name); //-------alligns string to the left: "abc-----"

	long long size = sb.st_size;
	char unit = 'B';
	if (size > 1024)
	{
		size /= 1024;
		unit = 'K';
	}
	if (size > 1024)
	{
		size /= 1024;
		unit = 'M';
	}
	if (size > 1024)
	{
		size /= 1024;
		unit = 'G';
	}

	printf("\t%lld", size);

	printf("%-3c", unit);

	char *tt = (ctime(&sb.st_mtime));
	tt[strlen(tt) - 1] = '\0';
	printf("\t%30s", tt);

	if (S_ISDIR(sb.st_mode))
	{
		printf("\033[1;32m");
		printf("\t%-20s\n", dirName);
		printf("\033[0m");
	}
	else
		printf("\t%-20s\n", dirName);
}

void open_dir(const char *path)
{
	DIR *d;
	d = opendir(path);

	if (d)
	{
		scroll_track = 0;
		dirList.clear();
		totalFiles = getDirectoryCount(path);
		unsigned int len = getFilePrintingcount();

		sort(dirList.begin(), dirList.end());

		printf("\033[H\033[J");		   //------------used to clear the screen.
		printf("%c[%d;%dH", 27, 1, 1); // sets cursor to 1st row, 1st column

		unsigned int i = 0, itr = 1;

		while (i < totalFiles && itr <= len)
		{
			char *tempFileName = new char[dirList[i].length() + 1];
			strcpy(tempFileName, dirList[i++].c_str());
			print_directory(tempFileName, path);
			itr++;
		}
	}
	else
	{

		return;
	}
}
