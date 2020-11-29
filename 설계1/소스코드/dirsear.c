#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "ssu_gettimeofday.h"

#define BUFFER_SIZE 1024
#define ERROR 0
#define WARNING 0.1

struct student {
	char id[16];
} student_id[100];

struct file {
	char pathname[1024];
	char filename[1024];
	char type[1024];
	float score;
} store[100];
int cnt;
int iserror = 0;
char midvalue[128];
struct score {
	char number[16];
	float score;
} num_score[100];
char error[16] = "error";
float sumscore;

static int filter(const struct dirent *dirent);
int scan_ans(char *path);
void scan_std(char *path);
void blank(char *path);
void program(char *path, char *id);
void samestring(char *path, int count);
char* clear(char *text);
void samescore(void);
void difscore(void);
void order(void);
void programscore(char *compile_path, char *exe_path, char *fname,  int count, char *id);
void timer(int sig);
int pidreturn(char *process);

int main(int argc, char *argv[])
{
	if (argc < 3) { 
		fprintf(stderr, "usage : %s <foler> <folder>\n", argv[0]);
		exit(1);
	}

	int fd;
	int a=0;
	int select_type;
	char path[1024] = "./ssu_score/";
	char path1[1024]  = "./ssu_score/";
	FILE *fp;
	struct timeval begin_t, end_t;

	gettimeofday(&begin_t, NULL);

	strcat(path, argv[2]);
		scan_ans(path);

		printf("\n");
	if((fd = open("./ANS/score_table.csv", O_CREAT|O_EXCL, 0644)) != -1) { 
		order();
		printf("score_table.csv file doesn't exist in TRUEDIR!\n");
		printf("1. input blank qustion and program question's score. ex)0.5 1\n");
		printf("2. input all question's score. ex)Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &select_type);
		if(select_type == 1)
			samescore();
		else if(select_type == 2)
			difscore();
		else {
			fprintf(stderr, "input again");
			exit(1);
		}

		if((fp=fopen("./ANS/score.csv", "a+")) < 0) {
			fprintf(stderr, "score.csv open error\n");
			exit(1);
		}
		else {
			fprintf(fp, " ,");
			for (a = 0; a<cnt; a++) 
				fprintf(fp, "%s,", store[a].filename);
			fprintf(fp, "%s, \n", "sum");
			fclose(fp);
		}
		strcat(path1, argv[1]);
		scan_std(path1);
		
		gettimeofday(&end_t, NULL);
		ssu_runtime(&begin_t, &end_t);

		exit(0);
	}

}


void samescore(void)
{
	FILE *fp;
	float temp;
	printf("Input value of blank question : ");
	scanf("%f", &temp);

	for(int a = 0; a < cnt; a++)
		if(strcmp(store[a].type, "txt")==0) {
			strcpy(num_score[a].number, store[a].filename);
			num_score[a].score = temp;
			fp = fopen("./ANS/score_table.csv","a+");
			fprintf(fp, "%10s, %10.2f,\n", store[a].filename, temp);
			fclose(fp);
		}
		
	printf("Input value of program question : ");
	scanf("%f", &temp);

	for(int a=0; a < cnt; a++)
		if(strcmp(store[a].type, "c")==0) {
			strcpy(num_score[a].number, store[a].filename);
			num_score[a].score = temp;
			fp = fopen("./ANS/score_table.csv","a+");
			fprintf(fp, "%10s, %10.2f,\n", store[a].filename, temp);
			fclose(fp);
		}
}

void difscore(void)
{
	FILE *fp;
	float temp;
	for(int a=0; a<cnt; a++)
		if(strcmp(store[a].type, "txt") == 0) {
			printf("Input value of %s: ", store[a].filename);
			scanf("%f", &temp);
			num_score[a].score = temp;
			strcpy(num_score[a].number, store[a].filename);
			fp = fopen("./ANS/score_table.csv","a+");
			fprintf(fp, "%10s, %10.2f,\n", store[a].filename, temp);
			fclose(fp);
		}
	
			
		
	for(int a=0; a<cnt; a++)
		if(strcmp(store[a].type, "c") == 0) {
			printf("Input value of %s: ", store[a].filename);
			scanf("%f", &temp);
			num_score[a].score = temp;
			strcpy(num_score[a].number, store[a].filename);
			fp = fopen("./ANS/score_table.csv","a+");
			fprintf(fp, "%10s, %10.2f,\n", store[a].filename, temp);
			fclose(fp);
		
		}
		
}

static int filter(const struct dirent *dirent)
{
	if(!(strcmp(dirent->d_name,".")) || !(strcmp(dirent->d_name, "..")))
		return 0;
	else
		return 1;
}

int  scan_ans(char *path)
{
	char *ptr;
	char *ptr1;
	char anspath[1024];
	char temp[1024];
	char ans_result[1024];
	char ans_temp[1024];
	struct dirent **namelist;
	struct stat statbuf;
	int count;
	int a;
	int fd;

	if((count = scandir(path, &namelist, filter, alphasort)) == -1) {
		fprintf(stderr, "error for scandir\n");
		exit(1);
	}


	for(a = 0; a < count; a++) {

		strcpy(anspath, path);
		strcat(anspath, "/");
		strcat(anspath, namelist[a]->d_name);

		if(lstat(anspath, &statbuf) < 0) {
			fprintf(stderr, "lstat error\n");
			exit(1);
		}



		if(S_ISDIR(statbuf.st_mode))
			scan_ans(anspath);
		else {
			strcpy(store[cnt].pathname, anspath);
			strcpy(store[cnt].filename, namelist[a]->d_name);

			strcpy(temp, store[cnt].filename);

			ptr1 = strtok(temp, ".");

			ptr = strtok(NULL, ".");

			if(ptr == NULL)
				printf("%s file has no type\n", store[cnt].pathname);
			else {
				strcpy(store[cnt].type, ptr);
				store[cnt].type[strlen(ptr)] = 0;
			}
			if(strcmp(ptr,"c") == 0) {
				strcpy(ans_result, "gcc ");
				strcat(ans_result, store[cnt].pathname);
				strcat(ans_result, " -o ");

				strcpy(ans_temp, "./ANS");
				if(mkdir(ans_temp, 0766) == -1 && errno!=EEXIST) {
					fprintf(stderr, "%s mkdir error\n", ans_temp);
					exit(1);
				}
				strcat(ans_temp, "/");
				strcat(ans_temp, ptr1);

				if(mkdir(ans_temp, 0766) == -1&& errno!=EEXIST) {
					fprintf(stderr, "%s mkdir error\n", ans_temp);
					exit(1);
				}
				
				strcat(ans_temp, "/");
				strcat(ans_temp, ptr1);

				strcat(ans_result, ans_temp);
				strcat(ans_result, ".exe ");
				strcat(ans_result, "-lpthread");
				
				system(ans_result);
				
				strcpy(ans_result, ans_temp);
				strcat(ans_result, ".stdout");

				if((fd = open(ans_result, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) {
					fprintf(stderr, "%s open error\n", ans_result);
					exit(1);
				}
				int tmp;
				tmp = open("dummy", O_RDWR|O_CREAT|O_TRUNC, 0644);
									
				strcat(ans_temp, ".exe");
				dup2(1, tmp);
				dup2(fd, 1);
				close(fd);

				system(ans_temp);
				
				dup2(tmp, 1);

				close(tmp);
				remove("dummy");

			}	
			cnt++;
		}
	}

	for(a = 0; a < count; a++)
		free(namelist[a]);

	free(namelist);

}

void scan_std(char *path) 
{
	int count;
	int a;
	struct dirent **namelist;
	char stdpath[1024];
	FILE *fp;

	if((count = scandir(path, &namelist, filter, alphasort)) == -1) {
		fprintf(stderr, "error for scandir\n");
		exit(1);
	}

	for(a=0; a<count; a++) {
		strcpy(student_id[a].id, namelist[a]->d_name);
	}
	for(a = 0; a < count; a++) {
		strcpy(stdpath, path);
		strcat(stdpath, "/");
		strcat(stdpath, student_id[a].id);
		if((fp = fopen("./ANS/score.csv", "a+")) < 0) {
			fprintf(stderr, "fopen error for ./ANS/score.csv\n");
			exit(1);
		}
		else
			fprintf(fp,  "%s", student_id[a].id);

		blank(stdpath);
		program(stdpath, student_id[a].id);
		fprintf(fp, "%.2f,", sumscore);
		fprintf(fp, "\n");
		fclose(fp);
		printf("%s is finished..\n", student_id[a].id);
	}
	

}

void blank(char *path)
{
	int i, a;
	int fd;
	int length;
	int size;
	char buf[BUFFER_SIZE];
	char stdstr[1024];
	char blankpath[1024];
	FILE *fp;
	
		
		for(a=0; a<cnt; a++) {

			if(strcmp(store[a].type, "txt") == 0) {
				strcpy(blankpath, path);
				strcat(blankpath, "/");
				strcat(blankpath, store[a].filename);

				if((fd = open(blankpath, O_RDONLY)) < 0) {
					fprintf(stderr, "%s does not exist\n", path);
					close(fd);
				}
				else {
					while ((length = read(fd, buf, BUFFER_SIZE)) > 0) {
						strcpy(stdstr, buf);
						size = length;
					}
					stdstr[size] = 0;
					samestring(stdstr, a);
					strcpy(stdstr, "");
					close(fd);
				}

			}
		}
	
	
}

void program(char *path, char *id) 
{
	char resultpath[1024] = "./STD";
	char programpath[1024];
	char programtemp[1024];
	char temp[1024];
	char *ptr;
	char *buf[1024];
	int i;
	int fd;
	int a;
	FILE *fp;


	for (a=0; a<cnt; a++) {
		if((strcmp(store[a].type, "c"))==0) {

			if(mkdir(resultpath, 0766) == -1 && errno != EEXIST) {
				fprintf(stderr, "1%s mkdir error\n", resultpath);
				exit(1);
			}
			strcat(resultpath, "/");
			strcat(resultpath, id); 

			if(mkdir(resultpath, 0766) == -1 && errno != EEXIST) {
				fprintf(stderr, "2%s makedir error\n", resultpath); 
				exit(1);
			}
			strcpy(temp, store[a].filename);

			ptr = strtok(temp, ".");

			strcat(resultpath, "/");
			strcat(resultpath, ptr);
			strcat(resultpath, ".stdexe");

			strcpy(programpath, path); 
			strcat(programpath, "/");
			strcat(programpath, store[a].filename);

			strcpy(programtemp, "gcc ");
			strcat(programtemp, programpath);
			strcat(programtemp, " -o ");
			strcat(programtemp, resultpath);
			strcat(programtemp, " -lpthread");

			char buf[1024];
			int i;
			int fd;

			struct sigaction act;
			act.sa_handler = timer;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;

			if(sigaction(SIGALRM, &act, 0) == -1)
			{   
				puts("sigaction () error\n");
				exit(1);
			}

			alarm(5);

			programscore(programtemp, resultpath, ptr, a, id);

			if((fp=fopen("./ANS/score.csv", "a+")) < 0) {
				fprintf(stderr, "score.csv open error\n");
				exit(1);
			}
			
			strcpy(programpath, "");
			strcpy(resultpath, "./STD");
			strcpy(programtemp, "");

		}
	}
}
void programscore(char *compile_path, char *exe_path, char *fname, int count, char *id)
{
	int length;
	int fd1, fd2;
	char buf[BUFFER_SIZE];
	char temp[BUFFER_SIZE];
	char error_path[1024];
	char stdout_path[1024];
	char *ptr;
	int warning_cnt=0;
	int fd;
	FILE *fp;

	
	fp = fopen("./ANS/score.csv", "a+");
	
	//fd1 = dup(1);
	strcpy(error_path, "./");
	strcat(error_path, error);
	if((mkdir(error_path, 0766) == -1 && errno != EEXIST)) {
		fprintf(stderr, "%s mkdir error\n", error_path);
	}
	strcat(error_path, "/");
	strcat(error_path, id);
	if((mkdir(error_path, 0766) == -1 && errno != EEXIST)) {
		fprintf(stderr, "%s mkdir error\n", error_path);
	}

	strcpy(stdout_path, "./");
	strcat(stdout_path, error);
	strcat(stdout_path, "/");
	strcat(stdout_path, id);
	strcat(stdout_path, "/");
	strcat(stdout_path, fname);
	strcat(stdout_path, "_error.txt");

	if((fd = open(stdout_path, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "error for open %s\n", error_path);
		exit(1);
	}

	int tmp,tmp1;
	tmp = open("dummy1", O_RDWR|O_CREAT|O_TRUNC, 0644);

	dup2(2, tmp);
	dup2(fd, 2);
	close(fd);

	system(compile_path);
	dup2(tmp, 2);
	close(tmp);
	remove("dummy1");
	
	strcpy(error_path,"./STD/");
	strcat(error_path, id);
	strcat(error_path, "/");
	strcat(error_path, fname);
	strcat(error_path, ".stdout");

	strcpy(midvalue, exe_path);
	if((fd1 = open(error_path, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) 
		fprintf(stderr, "error for open %s\n", error_path);

	tmp1 = open("dummy1", O_RDWR|O_CREAT|O_TRUNC, 0644);

	dup2(1, tmp1);
	dup2(fd1, 1);

	system(exe_path);
	close(fd1);
	dup2(tmp1, 1);
	close(tmp1);
	remove("dummy");
	
	fd = open(stdout_path, O_RDWR|O_CREAT|O_TRUNC, 0644);
	while(1) {
		length = read(fd, buf, BUFFER_SIZE);

		strcpy(temp, buf);
		if (length <= 0)
			break;

		while(1) {
			ptr = strstr(temp, "warning");
			if(ptr != NULL){
				strcpy(temp, ptr);
				warning_cnt++;
			}
			else
				break;
		}

		while(1) {
			ptr = strstr(buf, "error");
			if(ptr != NULL) {
				iserror = 1;
				break;
			}
		}
		if(iserror == 1) {
			fprintf(fp, "0,");
		}
		else {
			fprintf(fp, "%.2f,", num_score[count].score - warning_cnt*WARNING); 
			sumscore = sumscore + num_score[count].score - warning_cnt*WARNING;
		}
	}

	close(fd);

}


void samestring(char *strings, int count)
{
	int std_size = strlen(strings);
	int ans_size;
	char std_string[1024];
	char ans_string[2048];
	char word;
	int a, n;
	int fd;
	int point=0;
	int startp=0;
	int answercnt=0;
	FILE *fp;

	strcpy(std_string, strings);

	fp = fopen("./ANS/score.csv", "a+");
	
	while(1) {
		if((std_string[std_size-1] == 32) || (std_string[std_size-1] == '\n')){
			std_size--;
			std_string[std_size] = 0;
		}
		else{
			break;
		}
	}

	if((fd = open(store[count].pathname, O_RDONLY)) < 0) {
		fprintf(stderr, "%s open error\n", store[count].pathname);
	}
	while(1){
		while (n==1) {
			if (ans_size = (read(fd, &word, 1)>0)) {
				if (word != ':')  {
					ans_string[point] = word;
					point++;
				}
				else 
					n=0;
			}
			else
				n=0;
		}
		n=1;

		lseek(fd, startp, 0);
		read(fd, ans_string, point);

		if(ans_string[point-1] == 32 || ans_size == 0){
			ans_string[point-1] = 0;
		}

		answercnt++;

		strcpy(ans_string, clear(ans_string));
		strcpy(std_string, clear(std_string));


		if(strcmp(ans_string, std_string) == 0){
			fprintf(fp, "%.2f,", num_score[count].score);
			sumscore = sumscore + num_score[count].score;
			break;
		}
		else{
			if(ans_size == 0) {
				fprintf(fp, "%d,", 0);
				break;
			}
			else{
				startp = lseek(fd, 2, SEEK_CUR);
				point = 0;
			}
		}
	}
	fclose(fp);
	close(fd);

}

char* clear(char *text)
{
	int a = 0;
	int b = 0;
	static char temp[1024];

	while(1)
	{
		if(text[a] ==  '>'||text[a]=='<'||text[a]==','||text[a]=='='||text[a]==')'||text[a]=='('||text[a]=='|'||text[a]=='&') {
			if(text[a-1] == 32) {
				temp[--b] = text[a];
				b++;
				if(text[a+1] == 32) {
					temp[b] = text[a+2];
					a += 2;
					b++;
				}
			}

			else if(text[a+1] == 32) {
				temp[b] = text[a];
				temp[++b] = text[a+2];
				a += 2;
				b++;
			}
			else {
				temp[b] = text[a];
				b++;
			}

		}

		else if(((text[a]=='|')&&(text[a+1]=='|')) || ((text[a]=='&')&&(text[a+1]=='&'))||((text[a]=='>')&&(text[a]=='='))||((text[a]=='<')&&(text[a]=='='))) {
			if (text[a-1] == 32) {
				temp[--b] = text[a];
				temp[++b] = text[a+1];
				b++;
				if (text[a+2] == 32) {
					temp[b] = text[a+3];
					a += 3;
					b++;
				}
				else
					a++;
			}

			else if(text[a+2] == 32) {
				temp[b] = text[a];
				temp[++b] = text[a+3];
				a += 3;
				b++;
			}
		}

		else {
			temp[b] = text[a];
			b++;
		}

		if(text[a] == 0) {
			break;
		}

		a++;
	}
	return temp;
}

void order() 
{
	struct file temp[100];
	char *ptr;
	struct file tempstr;
	char  tempstr1[100];

	for(int i = 0 ; i < cnt; i++) {
		int j = 0;
		temp[i] = store[i];
		ptr = strtok(temp[i].filename, ".");
		strcpy(temp[i].filename, ptr);
		temp[i].filename[strlen(ptr)] = 0;
		while(1) {
			if(temp[i].filename[j] == '-') { 
				temp[i].filename[j] = '.';
			}
			else if(temp[i].filename[j] == 0) 
				break;
			j++;
		}
	}

	for (int i=0; i<cnt; i++) {
		for (int j=0; j<(cnt-1) ; j++) {
			if((atof)(temp[j].filename) > (atof)(temp[j+1].filename)) {
				tempstr = store[j];
				store[j] = store[j+1];
				store[j+1] = tempstr;
	
				strcpy(tempstr1, temp[j].filename);
				strcpy(temp[j].filename, temp[j+1].filename);
				strcpy(temp[j+1].filename, tempstr1);
			}
		}
	}
}

void timer(int sig)
{
	char temp[32];
	iserror = 1;
	pid_t a;
	a=pidreturn(midvalue);
	kill(a, SIGKILL);
}

int pidreturn(char *process)
{
	FILE *src;
	DIR *dir;
	struct dirent *entry;
	struct stat fileStat;

	int fd;
	int pid;
	int size;
	char cmdLine[256];
	char tempPath[256];

	dir = opendir("/proc");

	while ((entry = readdir(dir)) != NULL) {
		char proc[256];
		strcpy(proc,"/proc/");
		proc[6] = 0;
		strcat(proc,entry->d_name);
		if(lstat(proc, &fileStat)==-1)
			printf("lstat error\n");

		if(!S_ISDIR(fileStat.st_mode))
			continue;

		pid = atoi(proc+6);
		if (pid <= 0)
			continue;

		strcpy(tempPath,proc);
		strcat(tempPath, "/cmdline");
		src = fopen(tempPath, "r");

		memset(cmdLine, 0, sizeof(cmdLine));
		fgets(cmdLine, 256,src);
		fclose(src);

		if(strcmp(cmdLine, process)==0){
			return pid;
			break;
		}
		else
			continue;

	}
}

