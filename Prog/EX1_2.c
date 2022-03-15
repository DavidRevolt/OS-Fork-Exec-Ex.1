#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>



void mystrcat(char* a, char* b);
void explorer(char* directoryPath, char* studentName);
int compile(char* cFlie, char* directory);
void studentResult(char* main);
int compareFiles(char* file1, char* file2);
void writeToResult(char* name, int score);
int mystrcmp(char* str1, char* str2);
void printTextFile(char* fileName);

//GLOBAL VARS
char* studentDir;
char* inputFlie;
char* expectedOutput;
int resultFD;



int main(int argc, char* argv[]) {

	//////////////////////////////////////////////////////////////////
	//open the config file, read 3 lines
	//////////////////////////////////////////////////////////////////
	if (argc != 2) return 1;
	char* configLocation = argv[1];

	int fdConfig = open(configLocation, O_RDONLY);
	if (fdConfig < 0)
	{
		printf("\nFailed to open config file\n");
		exit(-1);

	}

	char configText[3][256] = { '\0' };
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 256; j++)
		{
			int temp = read(fdConfig, &configText[i][j], 1);
			if (temp == 0)
				break;
			if (configText[i][j] == '\n')
			{
				configText[i][j - 1] = '\0';
				break;
			}
		}

	close(fdConfig);

	studentDir = configText[0]; //Line1
	inputFlie = configText[1];  //Line2
	expectedOutput = configText[2]; //Line3

	printf("\n\033[32;1;4mReading Config File successfully:\033[0m\n");
	printf("%s\n", configText[0]);
	printf("%s\n", configText[1]);
	printf("%s\n\n", configText[2]);
	///////////////////////////////////////////////////////////////////////////////


	//Create results.csv
	resultFD = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (resultFD < 0)
	{
		printf("\nFailed to open results.csv file\n");
		exit(-1);
	}
	///////////////////////////////////////////////////////////////////////////////

	explorer(studentDir, NULL);
	close(resultFD);
	printTextFile("results.csv");
	return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//Explorer: Compile .c Files Inside Students Directory
///////////////////////////////////////////////////////////////////////////////////////////////////
void explorer(char* directoryPath, char* studentName)
{

	DIR* dir = opendir(directoryPath);
	if (dir == NULL)
	{
		//printf("error open dir\n");
		printf("\033[32;1mError opening directory %s\n Because: %s\n\033[0m\n", directoryPath, strerror(errno));
		exit(-1);
	}
	else
		printf("\nDirectory \033[33;1m%s\033[0m opened \033[32;1msuccessfully:\033[0m\n", directoryPath);


	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		//found sub directory
		if (entry->d_type == DT_DIR)
		{

			//Skip . or .. Directory Names
			if ((entry->d_name[0] == '.'))
				continue;


			// Create Sub-Directory Full Path:
			char newPath[256] = { '\0' };
			mystrcat(newPath, directoryPath);
			mystrcat(newPath, "/");
			mystrcat(newPath, entry->d_name);

			explorer(newPath, entry->d_name);
		}

		//found a file
		else
		{
			char cFileName[255] = { '\0' };
			mystrcat(cFileName, studentName);
			mystrcat(cFileName, ".c");

			if (mystrcmp(entry->d_name, cFileName))
			{
				//Create .C File Full Path:
				char cFliePath[256] = { '\0' };
				mystrcat(cFliePath, directoryPath);
				mystrcat(cFliePath, "/");
				mystrcat(cFliePath, entry->d_name);

				printf("C File \033[35;1m%s\033[0m Found at: \033[35;4m%s\033[0m ,Student:\033[34;1m%s\033[0m \n", entry->d_name, cFliePath, studentName);


				//Create main.out File Full Path:
				char mainPath[255] = { '\0' };
				mystrcat(mainPath, directoryPath);
				mystrcat(mainPath, "/main.out");

				//compile
				if (1 == compile(cFliePath, mainPath))
				{
					printf("main.out Path: \033[35;4m%s\033[0m\n", mainPath);

					//use student prog and write the result to Temp txt
					studentResult(mainPath);
					//compare student temp txt with expectedOutput and return score: 2 if the same, 1 if not.
					int compareResult = compareFiles(expectedOutput, "./program_output.txt");
					//write the final score+studentname
					writeToResult(studentName, compareResult);
				}
				else//compile failed
				{
					writeToResult(studentName, 1);
				}
			}
		}
	}
	closedir(dir);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//String Append
///////////////////////////////////////////////////////////////////////////////////////////////////
void mystrcat(char* a, char* b) {
	int ln = 0;

	while (a[ln] != '\0') {
		ln++;
	}
	int i = 0;
	while (b[i] != '\0') {
		a[ln + i] = b[i];
		i++;
	}
	a[ln + i + 1] = '\0';

}



///////////////////////////////////////////////////////////////////////////////////////////////////////
//Compile: recive C File path, destination directory and compile into main.out, return 1 for success
//////////////////////////////////////////////////////////////////////////////////////////////////////
int compile(char* cFlie, char* destination)
{

	pid_t  pid = fork();
	if (pid < 0)
	{
		printf("\033[41;1mERROR: FORK FAILED AT COMPILE!\033[0m\n\n");
		return -1;
	}

	if (pid == 0)
	{
		execlp("gcc", "gcc", cFlie, "-o", destination, NULL);
		printf("\033[41;1mEXEC GCC FAILED!\033[0m\n\n");//if we reach here => exe failed		
		exit(-1);
	}

	int status;
	wait(&status);
	{
		if (WEXITSTATUS(status) != 0) {
			printf("\033[41;1mERROR: COMPILE FAILED!\033[0m\n");
			return -1;
		}
		else
		{
			printf("\033[42;1mCOMPILED SUCCESSFULLY :)\033[0m\n");
			return 1;
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//recive main.out and print output to program_output.txt
///////////////////////////////////////////////////////////////////////////////////////////////////
void studentResult(char* main)
{
	int inputFlieFD = open(inputFlie, O_RDONLY);
	int userOutFD = open("program_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (inputFlieFD < 0 || userOutFD < 0)
	{
		printf("\nFailed to open %s file or program_output.txt \n", inputFlie);
		exit(-1);
	}


	pid_t  pid = fork();
	if (pid < 0)
	{
		printf("\033[41;1mERROR: FORK FAILED AT studentResult!\033[0m\n\n");
		exit(-1);
	}
	if (pid == 0)
	{
		dup2(inputFlieFD, 0);
		dup2(userOutFD, 1);
		execlp(main, main, NULL);
		printf("\033[41;1mEXEC AT studentResult FAILED!\033[0m\n\n");//if we reach here => exe failed		
		exit(-1);
	}

	int status;
	wait(&status);
	close(inputFlieFD);
	close(userOutFD);
	printf("\033[42;1mStudent program output is written into program_output.txt\033[0m\n");
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//*Compare Files using Comp.out
///////////////////////////////////////////////////////////////////////////////////////////////////
int compareFiles(char* file1, char* file2)
{
	pid_t  pid = fork();
	if (pid < 0)
	{
		printf("\033[41;1mERROR: FORK FAILED AT compareFiles!\033[0m\n\n");
		return -1;
	}

	if (pid == 0)
	{
		execl("./comp.out", "./comp.out", file1, file2, NULL);
		printf("\033[41;1mEXEC AT compareFiles FAILED!\033[0m\n\n");//if we reach here => exe failed		
		exit(-1);
	}

	int status;
	wait(&status);
	return WEXITSTATUS(status);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//*Write student name and his score to result.csv
///////////////////////////////////////////////////////////////////////////////////////////////////
void writeToResult(char* name, int score)
{
	int ScreenFD = dup(1);
	pid_t  pid = fork();
	if (pid < 0)
	{
		printf("\033[41;1mERROR: FORK FAILED AT studentResult!\033[0m\n\n");
		exit(-1);
	}

	if (pid == 0)
	{
		dup2(resultFD, 1);
		printf("%s,%d\n", name, score);
		dup2(ScreenFD, 1);
		exit(1);
	}
	else
	{
		int status;
		wait(&status);
		dup2(ScreenFD, 1);
		printf("\033[42;1m%s,%d has been written to result.csv\n\033[0m\n\n", name, score);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//*Compare strings
///////////////////////////////////////////////////////////////////////////////////////////////////
int mystrcmp(char* str1, char* str2) {
	int i = 0;
	int j = 0;
	while (str1[i] != '\0') {
		i++;
	}
	while (str2[j] != '\0') {
		j++;
	}

	if (i != j) { return 0; }
	else
	{
		for (i = 0; i <= j; i++) {
			if (str1[i] == str2[i]) {
				continue;
			}
			else return 0;
		}
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//*Print Text frome file
///////////////////////////////////////////////////////////////////////////////////////////////////
void printTextFile(char* fileName)
{
	int textFile = open(fileName, O_RDONLY);
	if (resultFD < 0)
	{
		printf("\nFailed to open results.csv file for printing\n");
		exit(-1);
	}
	printf("\n\033[32;1;4mPrinting %s File:\033[0m\n",fileName);
	char buf;
	while (read(textFile, &buf, 1))
		printf("%c", buf);
	close(textFile);
	printf("\n");
	return;
}
