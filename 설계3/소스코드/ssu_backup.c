#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

char backupdir[] = "backupdir";
char logfile[] = "logfile";
char backuplist[] = "backuplist";
char currentworking[1024];
char backuppath[1024];

typedef struct _Backup{
	char path[256];
	int period;
	char option[4][16];
	int next;
	pthread_t tid;
}Backup;

int backupcnt;

FILE *fp_log;
FILE *fp_list;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void command_add(char *filename, char *period, int argc, char *argv[]);
void command_remove(char *arg);
void command_compare(char *compare1, char *compare2);
void command_recover(char *path, char *opt);
void *add_thread(void *arg);
void searchdir(Backup backup);

int main(int argc, char *argv[])
{
	struct stat st;
	char backup_path[1024];
	char command[1024];
	char commandname[16];
	char commandtemp[1024];
	char option[16];
	char compare1[512];
	char compare2[512];
	char removefile[256];
	char removeopt[256];
	char period[256];
	char *toktemp;
	int argcnt=0;
	int head;
	int offset;
	int position;
	Backup backuptemp;

	backupcnt = 0;
	getcwd(currentworking, 1024);

	//인자가 2개 이상
	if(argc > 2) {
		fprintf(stderr, "usage : ./ssu_backup <PATH NAME>\n");
		exit(1);
	}


	//경로인자가 안주어졌을때, 현재폴더에 저장
	else if (argc == 1) {
		mkdir(backupdir, 0766);
		chdir(backupdir);
		getcwd(backuppath, 1024);
	}

	//경로인자가 주어졌을때
	else if (argc == 2) {

		//경로를 찾을 수 없는 경우
		if(lstat(argv[1], &st) == -1) {
			fprintf(stderr, "NO DIECTORY...usage: ./ssu_bakcup <PATH NAME>\n");
			exit(1);
		}
		//폴더가 아닌경우
		if(S_ISDIR(st.st_mode) != 1) {
			fprintf(stderr, "NOT DIRECTORY...usage: ./ssu_backup <PATH NAME>\n");
			exit(1);
		}
		//접근권한이 없는 경우
		if((st.st_mode & S_IRWXU) == 0) {
			fprintf(stderr, "NO PERMISSION...usage: ./ssu_backup <PATH NAME>\n");
			exit(1);
		}
		chdir(argv[1]);

		mkdir(backupdir, 0766);
		chdir(backupdir);
		getcwd(backuppath, 1024);
	}
	//백업리스트 파일을 오픈
	//링크드리스트 구현위해 헤드 -1로 설정
	fp_list = fopen(backuplist, "w+");
	head = -1;
	fwrite(&head, 4, 1, fp_list);

	//로그파일을 오픈
	fp_log = fopen(logfile, "w+"); 


	//프롬프트 반복 시작
	while(1) {
		printf("20142406> ");
		fgets(command, 1024, stdin);

		//엔터만 입력할때
		if(strcmp(command, "\n") == 0)
			continue;

		//뛰어쓰기, 개행으로 몇개의 단어가 있는지 계산
		strcpy(commandtemp, command);
		strtok(commandtemp, " '\n'");
		argcnt=1;
		while(1) {
			if(strtok(NULL, " '\n'") == NULL) 
				break;
			argcnt++;
		}


		strcpy(commandtemp, command);
		strcpy(commandname, strtok(commandtemp, " '\n'"));

		//명령어 add
		if(strcmp(commandname, "add") == 0) {
			char *argv_add[argcnt];
			int i;
			for(i=0; i<argcnt; i++) {
				argv_add[i] = (char *)malloc(512);
			}

			if(argcnt < 3) {
				fprintf(stderr, "not enough arg\n");
				continue;
			}

			strcpy(argv_add[0], commandname);
			for(int i = 1; i < argcnt; i++) {
				strcpy(argv_add[i], strtok(NULL, " '\n'"));
			}
			command_add(argv_add[1], argv_add[2], argcnt, argv_add);

			sleep(1);
			for(i=0; i<argcnt; i++) {
				free(argv_add[i]);
			}
		}	
		//명령어 remove
		else if(strcmp(commandname, "remove") == 0) {
			if(argcnt != 2) {
				fprintf(stderr, "wrong arg\n");
				continue;
			}
			command_remove(strtok(NULL, " '\n'"));
		}
		//명령어 compare
		else if(strcmp(commandname, "compare") == 0) {
			if(argcnt != 3) {
				fprintf(stderr, "wrong arg\n");
				continue;
			}
			strcpy(compare1, strtok(NULL, " '\n'"));
			strcpy(compare2, strtok(NULL, " '\n'"));
			command_compare(compare1, compare2);
		}
		//명령어 recover
		else if(strcmp(commandname, "recover") == 0) {
			if((argcnt == 3) || (argcnt == 1)) {
				fprintf(stderr, "wrong arg\n");
				continue;
			}

			if(argcnt == 2) 
				command_recover(strtok(NULL, " '\n'"), NULL);
			else if(argcnt == 4) {
				strcpy(removefile, strtok(NULL, " '\n'"));
				if(strcmp(strtok(NULL, " '\n'"), "-n") != 0) {
					fprintf(stderr, "wrong option\n");
					continue;
				}
				strcpy(removeopt, strtok(NULL, " '\n'"));
				command_recover(removefile, removeopt);
			}

		}
		else if(strcmp(commandname, "list") == 0) {
			rewind(fp_list);
			fread(&head, 4, 1, fp_list);
			fseek(fp_list, 0, SEEK_END);
			position = ftell(fp_list);
			//백업리스트에 존재하는지 확인과정
			offset = head;
			while(1) {
				if(offset == -1)
					break;
				else {
					//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
					fseek(fp_list, offset, SEEK_SET);
					fread(&backuptemp, sizeof(Backup), 1, fp_list);
					printf("%s, %d", backuptemp.path, backuptemp.period);
					for(int i=0; i<4; i++) {
						if(strcmp(backuptemp.option[i], "") != 0) {
							if(i == 0)
								printf(", -m ");
							else if(i == 1)
								printf(", -n:%s", backuptemp.option[i]);
							else if(i == 2)
								printf(", -t:%s", backuptemp.option[i]);
							else if(i == 3)
								printf(", -d ");
						}
					}
				}
				printf("\n");
				offset = backuptemp.next;
			}
		}
		//ls명령어
		else if(strcmp(commandname, "ls") == 0) {
			chdir(currentworking);
			system(command);
		}
		//vi명령어
		else if((strcmp(commandname, "vi") && strcmp(commandname, "vim")) == 0) {
			chdir(currentworking);
			system(command);
		}
		//exit명령
		else if(strcmp(commandname, "exit") == 0) {
			exit(0);
		}

		else {
			fprintf(stderr, "wrong command\n");
			continue;
		}

	}


	fclose(fp_list);
	fclose(fp_log);
}

void command_add(char *filename, char *period, int argc, char *argv[])
{
	int i;
	int opt;
	int head;
	int offset;
	long position;
	Backup backup;
	Backup backuptemp;
	char file[256];
	char filepath[256];
	char option[64];
	char *temp;
	temp = (char *)malloc(1024);
	char *org;
	pthread_t tid;
	struct stat statbuf;

	org = temp;
	optind = 0;

	//file에 경로를 제외한 파일이름 저장하는과정
	strcpy(temp, filename);
	strcpy(temp, strtok(temp, "/"));
	while(temp != NULL) {
		strcpy(file, temp);
		temp = strtok(NULL, "/");
	}

	//filename에서 파일이름을 제외하여 filepath에 저장
	//ex) filename: ./home/dohyun/abc.txt -> ./home/dohyun/ 
	strcpy(filepath, filename);
	temp = strstr(filepath, file);
	memset(temp, 0, strlen(file));

	temp = org;
	//상대경로를 절대경로로 바꾸는 과정
	//filepath : /home/dohyun
	//filename : /home/dohyun/abc.txt
	chdir(currentworking);

	if(strcmp(filepath, "") && strcmp(filepath, "./") == 0){
		chdir(currentworking);
	}
	else {
		chdir(filepath);
	}
	getcwd(filepath, 256);
	strcpy(filename, filepath);
	strcat(filename, "/");
	strcat(filename, file);

	//period
	//정수가 아닐때
	if ((strstr(period, ".") != NULL)) {
		fprintf(stderr, "not integer\n");
	}

	//period자리에 숫자가 아닌 옵션 등이 들어왔을때
	if ((strstr(period, "-") != NULL)) {
		fprintf(stderr, "wrong period\n");
		return ;
	}
	//5~10사이의 값이 아닐때
	if ((atoi(period) <= 10 && atoi(period)>=5) != 1) {
		fprintf(stderr, "wrong period(5<=peiord<=10)\n");
		return ;
	}
	
	
	//backup구조체에 정보 저장
	strcpy(backup.path, filename);
	backup.period = atoi(period);

	for(i = 0; i < 4; i++) 
		strcpy(backup.option[i], "");

	if ((stat(filename, &statbuf)) < 0) {
		fprintf(stderr, "file not exits\n");
		free(temp);
		return;
	}

	//옵션이 없는경우
	if(argc == 3) {	
		if((statbuf.st_mode &S_IFMT) != S_IFREG) {
			fprintf(stderr, "filename is not reg\n");
			free(temp);
			return ;
		}	

		rewind(fp_list);
		fread(&head, 4, 1, fp_list);
		fseek(fp_list, 0, SEEK_END);
		position = ftell(fp_list);
		//백업리스트에 존재하는지 확인과정
		offset = head;
		while(1) {
			if(offset == -1)
				break;
			else {
				//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
				fseek(fp_list, offset, SEEK_SET);
				fread(&backuptemp, sizeof(Backup), 1, fp_list);
				if(strcmp(backuptemp.path, filename) == 0) {
					fprintf(stderr, "backup list already exist\n");
					free(temp);
					return ;
				}
				offset = backuptemp.next;
			}
		}


		if(pthread_create(&tid, NULL, add_thread, (void *)&backup) != 0) {
			fprintf(stderr, "pthread_create error\n");
			exit(1);
		}

		//링크드리스트를 이용하여 백업리스트에 추가
		//ex)head ->15->34에 50추가하면
		//head->50->15->34
		backup.tid = tid;
		backup.next = head;  
		head = position;

		//헤드에 다음 리스트 위치 기록
		rewind(fp_list);
		fwrite(&head, 4, 1, fp_list);

		//파일끝에 새리스트 기록
		fseek(fp_list, 0, SEEK_END);
		fwrite(&backup, sizeof(Backup), 1, fp_list);

	}

	else if(argc > 3) {

		while((opt = getopt(argc, argv, "mn:dt:")) != -1) {

			switch(opt)  
			{  
				case 'm': 
					//"on"을 구조체 변수중 option[0]에 넣어줌
					//option[1]~option[3]은 빈칸
					strcpy(backup.option[0], "on");
					break;  
				case 'n': 
					if(atoi(optarg) == 0) {
						fprintf(stderr, "Not number\n");
						return ;
					}
					if(strstr(optarg, ".") != NULL) {
						fprintf(stderr, "NUMBER is not integer\n");
						return ;
					}
					if(((atoi(optarg)>=1) && (atoi(optarg)<=100)) != 1) {
						fprintf(stderr, "wrong NUMBER(1<=num<=100)\n");
						return ;
					}
					sprintf(option, "%s", optarg);
					strcpy(backup.option[1], option);
					break; 
				case 't': 
					if(atoi(optarg) == 0) {
						fprintf(stderr, "Not number\n");
						return ;
					}
					if(strstr(optarg, ".") != NULL) {
						fprintf(stderr, "NUMBER is not integer\n");
						return ;
					}
					if(((atoi(optarg)>=60) && (atoi(optarg)<=1200)) != 1) {
						fprintf(stderr, "wrong NUMBER(1<=num<=100)\n");
						return ;
					}
					sprintf(option, "%s", optarg);
					strcpy(backup.option[2], option);

					break; 
				case 'd': 
					if((statbuf.st_mode & S_IFMT) != S_IFDIR) {
						fprintf(stderr, "d option needs dir\n");
						free(temp);
						return ;
					}
					strcpy(backup.option[3], "on");
					break; 
				case '?':
					printf("wrong flag\n");
					free(temp);
					return;
			} 
		}
		//d옵션이 있을때
		if(strcmp(backup.option[3], "") != 0) {
			searchdir(backup);

		}
		//d옵션 없을때
		else {
			if((statbuf.st_mode &S_IFMT) != S_IFREG) {
				fprintf(stderr, "filename is not reg\n");
				free(temp);
				return ;
			}

			rewind(fp_list);
			fread(&head, 4, 1, fp_list);
			fseek(fp_list, 0, SEEK_END);
			position = ftell(fp_list);

			//백업리스트에 존재하는지 확인과정
			offset = head;
			while(1) {
				if(offset == -1)
					break;
				else {
					//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
					fseek(fp_list, offset, SEEK_SET);
					fread(&backuptemp, sizeof(Backup), 1, fp_list);
					if(strcmp(backuptemp.path, filename) == 0) {
						fprintf(stderr, "backup list already exist\n");
						free(temp);
						return ;
					}
					offset = backuptemp.next;
				}
			}		



			//링크드리스트를 이용
			//ex)head ->15->34에 50추가하면
			//head->50->15->34
			backup.next = head;  
			head = position;

			//헤드에 다음 리스트 위치 기록
			rewind(fp_list);
			fwrite(&head, 4, 1, fp_list);

			//파일끝에 새리스트 기록
			fseek(fp_list, 0, SEEK_END);
			fwrite(&backup, sizeof(Backup), 1, fp_list);


			if(pthread_create(&tid, NULL, add_thread, (void *)&backup) != 0) {
				fprintf(stderr, "pthread_create error\n");
				exit(1);
			}

		}
	}

	free(temp);
}

void *add_thread(void *arg) 
{
	struct dirent **list;
	struct stat statorg;
	struct stat statbuf;
	Backup info;
	char pathname[512];
	char day[7];
	char clock[7];
	char clocktmp[7];
	char tmp[3];
	char line[1024];
	char timestd[17];
	char file[256];
	char optpath[512];
	char *temp = (char *)malloc(256);
	char *ptr;
	int subsec1;
	int subsec2;
	int sub;
	int filenum;
	int ncount=0;
	int cnt=0;
	int i;
	int total, hour, min, sec;
	int on;

	FILE *fp1, *fp2;
	DIR *dirp;

	time_t timer;
	struct tm *t;


	info = *((Backup *)arg);

	strcpy(pathname, info.path);
	fp1 = fopen(pathname, "r");

	strcpy(temp, info.path);
	strcpy(temp, strtok(temp, "/"));
	while(temp != NULL) {
		strcpy(file, temp);
		temp = strtok(NULL, "/");
	}

	//m 옵션 주어졌을때
	if(strcmp(info.option[0], "") != 0) {
		if(stat(info.path, &statorg) < 0) {
			fprintf(stderr, "Stat error\n");
			exit(1);
		}
	}


	//t 옵션시 주어진 시간을 시간,분,초로 고치는 과정
	if(info.option[2] != "") {
		total = atoi(info.option[2]);
		hour = total/3600;
		total = total - hour*3600;
		min = total/60;
		total = total - min*60;
		sec = total;


		sprintf(tmp, "%d", hour);
		if((hour/10) == 0) {
			strcpy(clocktmp, "0");
			strcat(clocktmp, tmp);
		}
		else
			strcpy(clocktmp, tmp);

		sprintf(tmp, "%d", min);
		if((min/10) == 0) {
			strcat(clocktmp, "0");
			strcat(clocktmp, tmp);
		}
		else
			strcat(clocktmp, tmp);

		sprintf(tmp, "%d", sec);
		if((sec/10) == 0) {
			strcat(clocktmp, "0");
			strcat(clocktmp, tmp);
		}
		else
			strcat(clocktmp, tmp);

	}

	//added 문구 저장
	timer = time(NULL);
	t = localtime(&timer);

	sprintf(tmp, "%d", (t->tm_year-100));
	if(((t->tm_year-100)/10) == 0) {
		strcpy(day, "0");
		strcat(day, tmp);
	}
	else
		strcpy(day, tmp);

	sprintf(tmp, "%d", (t->tm_mon+1));
	if(((t->tm_mon+1)/10) == 0) {
		strcat(day, "0");
		strcat(day, tmp);
	}
	else
		strcat(day, tmp);

	sprintf(tmp, "%d", (t->tm_mday));
	if(((t->tm_mday)/10) == 0) {
		strcat(day, "0");
		strcat(day, tmp);
	}
	else
		strcat(day, tmp);

	sprintf(tmp, "%d", (t->tm_hour));
	if(((t->tm_hour)/10) == 0) {
		strcpy(clock, "0");
		strcat(clock, tmp);
	}
	else
		strcpy(clock, tmp);

	sprintf(tmp, "%d", (t->tm_min));
	if(((t->tm_min)/10) == 0) {
		strcat(clock, "0");
		strcat(clock, tmp);
	}
	else
		strcat(clock, tmp);

	sprintf(tmp, "%d", (t->tm_sec));
	if(((t->tm_sec)/10) == 0) {
		strcat(clock, "0");
		strcat(clock, tmp);
	}
	else
		strcat(clock, tmp);

	strcpy(pathname, backuppath);
	strcat(pathname, "/");
	strcat(pathname, file);
	strcat(pathname, "_");
	strcat(pathname, day);
	strcat(pathname, clock);

	strcpy(line, "[");
	strcat(line, day);
	strcat(line, " ");
	strcat(line, clock);
	strcat(line, "] ");
	strcat(line, info.path);
	strcat(line, " added\n");
	fwrite(line, strlen(line), 1, fp_log);
	fflush(fp_log);
	sleep(info.period);



	while(1) {

		if(strcmp(info.option[0], "") == 0) {
			on = 1;
		}
		else {
			if(stat(info.path, &statbuf) < 0) {
				fprintf(stderr, "stat error\n");
				exit(1);
			}

			if(statorg.st_mtime == statbuf.st_mtime)
				on = 0;
			else {
				on = 1;
				statorg.st_mtime = statbuf.st_mtime;
			}
		}

		if(on) {

			timer = time(NULL);
			t = localtime(&timer);

			sprintf(tmp, "%d", (t->tm_year-100));
			if(((t->tm_year-100)/10) == 0) {
				strcpy(day, "0");
				strcat(day, tmp);
			}
			else
				strcpy(day, tmp);

			sprintf(tmp, "%d", (t->tm_mon+1));
			if(((t->tm_mon+1)/10) == 0) {
				strcat(day, "0");
				strcat(day, tmp);
			}
			else
				strcat(day, tmp);

			sprintf(tmp, "%d", (t->tm_mday));
			if(((t->tm_mday)/10) == 0) {
				strcat(day, "0");
				strcat(day, tmp);
			}
			else
				strcat(day, tmp);

			sprintf(tmp, "%d", (t->tm_hour));
			if(((t->tm_hour)/10) == 0) {
				strcpy(clock, "0");
				strcat(clock, tmp);
			}
			else
				strcpy(clock, tmp);

			sprintf(tmp, "%d", (t->tm_min));
			if(((t->tm_min)/10) == 0) {
				strcat(clock, "0");
				strcat(clock, tmp);
			}
			else
				strcat(clock, tmp);

			sprintf(tmp, "%d", (t->tm_sec));
			if(((t->tm_sec)/10) == 0) {
				strcat(clock, "0");
				strcat(clock, tmp);
			}
			else
				strcat(clock, tmp);

			strcpy(pathname, backuppath);
			strcat(pathname, "/");
			strcat(pathname, file);
			strcat(pathname, "_");
			strcat(pathname, day);
			strcat(pathname, clock);

			chdir(backuppath);
			fp2 = fopen(pathname, "w");


			while(1) {
				fgets(line, 1024, fp1);
				if(feof(fp1))
					break;
				fputs(line, fp2);
			}


			strcpy(line, "[");
			strcat(line, day);
			strcat(line, " ");
			strcat(line, clock);
			strcat(line, "] ");
			strcat(line, pathname);
			strcat(line, " generated\n");
			fwrite(line, strlen(line), 1, fp_log);
			fflush(fp_log);

			//n옵션 있을때
			//n옵션에 값이 있고, 그값이 파일갯수와 같을때
			//백업디렉토리에 있는 모든파일을 오름차순(시간순) 정렬 후
			//백업할파일의 이름이 들어가 있는 파일중 가장 오래된것을 삭제
			if((strcmp(info.option[1], "") != 0)) {
				filenum = scandir(backuppath, &list, NULL, alphasort);
				for(i=0; i<filenum; i++)
					if(strstr(list[i]->d_name, file) != NULL) 
						ncount++;

				if(ncount > atoi(info.option[1])) {
					for(i=0; i<filenum; i++) {
						if(strstr(list[i]->d_name, file) != NULL) {
							strcpy(optpath, backuppath);
							strcat(optpath, "/");
							strcat(optpath, list[i]->d_name);
							unlink(optpath);
							break;
						}
					}
				}
				ncount=0;
			}

			//t옵션 있을때
			if(strcmp(info.option[2], "") != 0) {
				filenum = scandir(backuppath, &list, NULL, alphasort);
				for(i=0; i<filenum; i++) {
					if(strstr(list[i]->d_name, file) != NULL) {
						strcpy(optpath, list[i]->d_name);
						ptr = strrchr(optpath, '_');
						ptr += 7;

						strcpy(timestd, clock);

						//hhmmss로 돼있는것을 초단위로 바꿔주는과정
						//ex)141026 = 14*3600 + 10*60 + 26
						subsec1 = atoi(timestd)/10000*3600;
						subsec1 = subsec1 + (atoi(timestd) - atoi(timestd)/10000*10000)/100*60;
						subsec1 = subsec1 + atoi(timestd)%100;

						subsec2 = atoi(ptr)/10000*3600;
						subsec2 = subsec2 + (atoi(ptr) - atoi(ptr)/10000*10000)/100*60;
						subsec2 = subsec2 + atoi(ptr)%100;

						sub = subsec1-subsec2;
						//초의 차가 t 옵션 인자보다 크면 삭제
						if(sub > atoi(clocktmp)) {
							strcpy(optpath, backuppath);
							strcat(optpath, "/");
							strcat(optpath, list[i]->d_name);
							unlink(optpath);
						}
					}
				}
			}

			rewind(fp1);
			fclose(fp2);
		}
		sleep(info.period);
	}

	fclose(fp1);
	free(temp);
}

void searchdir(Backup backup) 
{
	struct dirent *dentry;
	struct stat statbuf;
	pthread_t tid;
	Backup backuptemp;
	Backup info[1024];
	DIR *dirp;
	char filename[1204];
	char cur[1024];
	int infocnt=0;
	int exist;
	int head;
	int offset;
	int position;

	dirp = opendir(backup.path);
	chdir(backup.path);

	while((dentry = readdir(dirp)) != NULL) {
		exist = 0;
		getcwd(cur, 1024);
		strcat(cur, "/");

		if(dentry->d_ino == 0)
			continue;

		memcpy(filename, dentry->d_name, 1024);

		//읽은파일이름이 ., ..일때는 스킵
		if((strcmp(filename, ".") ==0) || strcmp(filename, "..") ==0)
			continue;
		strcat(cur, filename);
		strcpy(backup.path, cur);

		stat(backup.path, &statbuf);

		//읽은파일이 일반파일일때
		//백업리스트에 등록된지 확인후 쓰레드 실행
		if((statbuf.st_mode &S_IFMT) == S_IFREG) {

			rewind(fp_list);
			fread(&head, 4, 1, fp_list);
			fseek(fp_list, 0, SEEK_END);
			position = ftell(fp_list);

			//백업리스트에 존재하는지 확인과정
			offset = head;
			while(1) {
				if(offset == -1)
					break;
				else {
					//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
					fseek(fp_list, offset, SEEK_SET);
					fread(&backuptemp, sizeof(Backup), 1, fp_list);
					if(strcmp(backuptemp.path, backup.path) == 0) {
						fprintf(stderr, "backup list already exist\n");
						exist = 1; //백업리스트에 존재함을 표시
						break;
					}
					offset = backuptemp.next;
				}
			}		

			if(exist == 0) {

				//디렉토리인 backup.path에 현재읽은파일 이름 붙여줌
				//strcat(backup.path, "/");
				//strcat(backup.path, filename);

				info[infocnt] = backup;
				if(pthread_create(&tid, NULL, add_thread, (void *)&info[infocnt]) != 0) {
					fprintf(stderr, "pthread_create error\n");
					exit(1);
				}
				//링크드리스트를 이용
				//ex)head ->15->34에 50추가하면
				//head->50->15->34
				backup.tid = tid;
				backup.next = head;  
				head = position;

				//헤드에 다음 리스트 위치 기록
				rewind(fp_list);
				fwrite(&head, 4, 1, fp_list);

				//파일끝에 새리스트 기록
				fseek(fp_list, 0, SEEK_END);
				fwrite(&backup, sizeof(Backup), 1, fp_list);

				infocnt++;
				//sleep(2);
			}

		}
		//일반파일이 아닌 폴더파일일때
		else {
			//함수를 재귀적으로 수행
			//backuptemp = backup;
			//strcat(backuptemp.path, "/");
			//strcat(backuptemp.path, filename);
			searchdir(backup);
			chdir("..");

		}
	}
}

void command_remove(char *arg)
{
	int head;
	int position;
	int offset;
	int before;
	char day[7];
	char clock[7];
	char clocktmp[7];
	char tmp[3];
	char *temp = (char *)malloc(1024);
	char *org;
	char filename[512];
	char filepath[256];
	char file[256];
	char pathname[1024];
	char line[1024];
	time_t timer;
	struct tm *t;
	Backup backuptemp;

	org = temp;
	strcpy(filename, arg);

	if(strcmp(arg, "-a") != 0) {
		//file에 경로를 제외한 파일이름 저장하는과정
		strcpy(temp, filename);
		strcpy(temp, strtok(temp, "/"));
		while(temp != NULL) {
			strcpy(file, temp);
			temp = strtok(NULL, "/");
		}

		//filename에서 파일이름을 제외하여 filepath에 저장
		//ex) filename: ./home/dohyun/abc.txt -> ./home/dohyun/ 
		strcpy(filepath, filename);
		temp = strstr(filepath, file);
		memset(temp, 0, strlen(file));

		temp = org;
		//상대경로를 절대경로로 바꾸는 과정
		//filepath : /home/dohyun
		//filename : /home/dohyun/abc.txt
		chdir(currentworking);

		if(strcmp(filepath, "") && strcmp(filepath, "./") == 0){
			chdir(currentworking);
		}
		else {
			chdir(filepath);
		}
		getcwd(filepath, 256);
		strcpy(filename, filepath);
		strcat(filename, "/");
		strcat(filename, file);


		rewind(fp_list);
		fread(&head, 4, 1, fp_list);
		fseek(fp_list, 0, SEEK_END);
		position = ftell(fp_list);
		//백업리스트에 존재하는지 확인과정
		offset = head;
		before = 0;
		while(1) {
			if(offset == -1) {
				fprintf(stderr, "no backuplist\n");
				return ;
			}
			else {
				//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
				fseek(fp_list, offset, SEEK_SET);
				fread(&backuptemp, sizeof(Backup), 1, fp_list);
				if(strcmp(backuptemp.path, filename) == 0) {
					pthread_cancel(backuptemp.tid);

					//[000000 000000] <filename>꼴을 저장하기 위함
					timer = time(NULL);
					t = localtime(&timer);

					sprintf(tmp, "%d", (t->tm_year-100));
					if(((t->tm_year-100)/10) == 0) {
						strcpy(day, "0");
						strcat(day, tmp);
					}
					else
						strcpy(day, tmp);

					sprintf(tmp, "%d", (t->tm_mon+1));
					if(((t->tm_mon+1)/10) == 0) {
						strcat(day, "0");
						strcat(day, tmp);
					}
					else
						strcat(day, tmp);

					sprintf(tmp, "%d", (t->tm_mday));
					if(((t->tm_mday)/10) == 0) {
						strcat(day, "0");
						strcat(day, tmp);
					}
					else
						strcat(day, tmp);

					sprintf(tmp, "%d", (t->tm_hour));
					if(((t->tm_hour)/10) == 0) {
						strcpy(clock, "0");
						strcat(clock, tmp);
					}
					else
						strcpy(clock, tmp);

					sprintf(tmp, "%d", (t->tm_min));
					if(((t->tm_min)/10) == 0) {
						strcat(clock, "0");
						strcat(clock, tmp);
					}
					else
						strcat(clock, tmp);

					sprintf(tmp, "%d", (t->tm_sec));
					if(((t->tm_sec)/10) == 0) {
						strcat(clock, "0");
						strcat(clock, tmp);
					}
					else
						strcat(clock, tmp);

					strcpy(line, "[");
					strcat(line, day);
					strcat(line, " ");
					strcat(line, clock);
					strcat(line, "] ");
					strcat(line, backuptemp.path);
					strcat(line, " deleted\n");
					fwrite(line, strlen(line), 1, fp_log);
					fflush(fp_log);


					offset = backuptemp.next;
					//전에 링크된게 head 였다면 head에 다음 구조체를 저장
					if(before == 0) {
						rewind(fp_list);
						fwrite(&offset, 4, 1, fp_list);
					}
					//전의 구조체에 다음구조체의 오프셋을 저장시킴
					else {
						fseek(fp_list, before, SEEK_SET);
						fread(&backuptemp, sizeof(Backup), 1, fp_list);
						backuptemp.next = offset;
						fseek(fp_list, before, SEEK_SET);
						fwrite(&backuptemp, sizeof(Backup), 1, fp_list);
					}
					return ;
				}
				before = offset;
				offset = backuptemp.next;
			}
		}
	}
	else if(strcmp(arg, "-a") == 0) {
		rewind(fp_list);
		fread(&head, 4, 1, fp_list);
		fseek(fp_list, 0, SEEK_END);
		position = ftell(fp_list);
		//백업리스트에 존재하는지 확인과정
		offset = head;
		while(1) {
			if(offset == -1) {
				fseek(fp_list, 0, SEEK_SET);
				fwrite(&offset, 4, 1, fp_list);
				return ;
			}
			else {
				//offset위치로 가서 구조체를 읽고
				//구조체에 저장된 tid를 종료
				//구조체에 저장된 다음 구조체 offset을 구함
				fseek(fp_list, offset, SEEK_SET);
				fread(&backuptemp, sizeof(Backup), 1, fp_list);
				pthread_cancel(backuptemp.tid);
				offset = backuptemp.next;

				//[000000 000000] <filename> deleted 꼴을 만들기 위함
				timer = time(NULL);
				t = localtime(&timer);

				sprintf(tmp, "%d", (t->tm_year-100));
				if(((t->tm_year-100)/10) == 0) {
					strcpy(day, "0");
					strcat(day, tmp);
				}
				else
					strcpy(day, tmp);

				sprintf(tmp, "%d", (t->tm_mon+1));
				if(((t->tm_mon+1)/10) == 0) {
					strcat(day, "0");
					strcat(day, tmp);
				}
				else
					strcat(day, tmp);

				sprintf(tmp, "%d", (t->tm_mday));
				if(((t->tm_mday)/10) == 0) {
					strcat(day, "0");
					strcat(day, tmp);
				}
				else
					strcat(day, tmp);

				sprintf(tmp, "%d", (t->tm_hour));
				if(((t->tm_hour)/10) == 0) {
					strcpy(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcpy(clock, tmp);

				sprintf(tmp, "%d", (t->tm_min));
				if(((t->tm_min)/10) == 0) {
					strcat(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcat(clock, tmp);

				sprintf(tmp, "%d", (t->tm_sec));
				if(((t->tm_sec)/10) == 0) {
					strcat(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcat(clock, tmp);

				strcpy(line, "[");
				strcat(line, day);
				strcat(line, " ");
				strcat(line, clock);
				strcat(line, "] ");
				strcat(line, backuptemp.path);
				strcat(line, " deleted\n");
				fwrite(line, strlen(line), 1, fp_log);
				fflush(fp_log);

			}
		}
	}
}

void command_compare(char *compare1, char *compare2) 
{
	char file1[1024];
	char file2[1024];
	struct stat statbuf1;
	struct stat statbuf2;

	chdir(currentworking);

	if(stat(compare1, &statbuf1) < 0) {
		fprintf(stderr, "file1 is not accessible\n");
		return ;
	}

	if(stat(compare2, &statbuf2) < 0) {
		fprintf(stderr, "file2 is not accessible\n");
		return ;
	}

	if((statbuf1.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "file1 is not reg\n");
		return ;
	}

	if((statbuf2.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "file2 is not reg\n");
		return ;
	}

	//사이즈가같고 수정시간이 같다면 같은 파일이다
	if((statbuf1.st_size ==  statbuf2.st_size) && (statbuf1.st_mtime == statbuf2.st_mtime))
		printf("file1, file2 is same\n");
	else{
		printf("different!\n");
		printf("file1) size:%ld, mtime:%ld\n", statbuf1.st_size, statbuf1.st_mtime);
		printf("file2) size:%ld, mtime:%ld\n", statbuf2.st_size, statbuf2.st_mtime);
	}

}

void command_recover(char *path, char *opt)
{
	struct dirent **list;
	char listtmp[256];
	char file[256];
	char filepath[256];
	char filename[512];
	char filetmp[512];
	char **filelist;
	char line[1024];
	char day[7];
	char clock[7];
	char *temp;
	char tmp[7];
	char num[8];
	temp = (char *)malloc(1024);
	char *org;
	int filenum;
	int listcnt;
	int count;
	int head;
	int position;
	int offset;
	int before;
	struct stat statbuf;
	FILE *fp1, *fp2;
	Backup backuptemp;
	time_t timer;
	struct tm *t;
	org = temp;


	strcpy(filename, path);

	//file에 경로를 제외한 파일이름 저장하는과정
	strcpy(temp, filename);
	strcpy(temp, strtok(temp, "/"));
	while(temp != NULL) {
		strcpy(file, temp);
		temp = strtok(NULL, "/");
	}

	//filename에서 파일이름을 제외하여 filepath에 저장
	//ex) filename: ./home/dohyun/abc.txt -> ./home/dohyun/ 
	strcpy(filepath, filename);
	temp = strstr(filepath, file);
	memset(temp, 0, strlen(file));

	temp = org;
	//상대경로를 절대경로로 바꾸는 과정
	//filepath : /home/dohyun
	//filename : /home/dohyun/abc.txt
	chdir(currentworking);

	if(strcmp(filepath, "") && strcmp(filepath, "./") == 0){
		chdir(currentworking);
	}
	else {
		chdir(filepath);
	}
	getcwd(filepath, 256);
	strcpy(filename, filepath);
	strcat(filename, "/");
	strcat(filename, file);

	if(stat(filename, &statbuf) < 0) {
		fprintf(stderr, "file not exist\n");
		temp = org;
		free(temp);
		return ;
	}

	if(stat(opt, &statbuf) == 0) {
		fprintf(stderr, "newfile already exist\n");
		temp = org;
		free(temp);
		return;
	}



	rewind(fp_list);
	fread(&head, 4, 1, fp_list);
	fseek(fp_list, 0, SEEK_END);
	position = ftell(fp_list);
	//백업리스트에 존재하는지 확인과정
	offset = head;
	before = 0;
	while(1) {
		if(offset == -1) {
			fprintf(stderr, "no backuplist\n");
			temp = org;
			free(temp);
			return ;
		}
		else {
			//저장된 구조체를 읽어 저장된 파일이름과 추가하려는 파일이름이 같은지 비교
			fseek(fp_list, offset, SEEK_SET);
			fread(&backuptemp, sizeof(Backup), 1, fp_list);
			if(strcmp(backuptemp.path, filename) == 0) {
				pthread_cancel(backuptemp.tid);

				//[000000 000000] <filename>꼴을 저장하기 위함
				timer = time(NULL);
				t = localtime(&timer);

				sprintf(tmp, "%d", (t->tm_year-100));
				if(((t->tm_year-100)/10) == 0) {
					strcpy(day, "0");
					strcat(day, tmp);
				}
				else
					strcpy(day, tmp);

				sprintf(tmp, "%d", (t->tm_mon+1));
				if(((t->tm_mon+1)/10) == 0) {
					strcat(day, "0");
					strcat(day, tmp);
				}
				else
					strcat(day, tmp);

				sprintf(tmp, "%d", (t->tm_mday));
				if(((t->tm_mday)/10) == 0) {
					strcat(day, "0");
					strcat(day, tmp);
				}
				else
					strcat(day, tmp);

				sprintf(tmp, "%d", (t->tm_hour));
				if(((t->tm_hour)/10) == 0) {
					strcpy(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcpy(clock, tmp);

				sprintf(tmp, "%d", (t->tm_min));
				if(((t->tm_min)/10) == 0) {
					strcat(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcat(clock, tmp);

				sprintf(tmp, "%d", (t->tm_sec));
				if(((t->tm_sec)/10) == 0) {
					strcat(clock, "0");
					strcat(clock, tmp);
				}
				else
					strcat(clock, tmp);

				strcpy(line, "[");
				strcat(line, day);
				strcat(line, " ");
				strcat(line, clock);
				strcat(line, "] ");
				strcat(line, backuptemp.path);
				strcat(line, " deleted\n");
				fwrite(line, strlen(line), 1, fp_log);
				fflush(fp_log);


				offset = backuptemp.next;
				//전에 링크된게 head 였다면 head에 다음 구조체를 저장
				if(before == 0) {
					rewind(fp_list);
					fwrite(&offset, 4, 1, fp_list);
				}
				//전의 구조체에 다음구조체의 오프셋을 저장시킴
				else {
					fseek(fp_list, before, SEEK_SET);
					fread(&backuptemp, sizeof(Backup), 1, fp_list);
					backuptemp.next = offset;
					fseek(fp_list, before, SEEK_SET);
					fwrite(&backuptemp, sizeof(Backup), 1, fp_list);
				}

				//종료했으므로 백업 수행
				//백업디렉토리안의 파일 오름차순을 읽어들임
				filenum = scandir(backuppath, &list, NULL, alphasort);
				count = 0;
				//백업디렉토리안에 백업할파일의 이름을 가지고 있는 백업파일갯수
				for(int i=0; i<filenum; i++) {
					strcpy(listtmp, list[i]->d_name);
					if(strcmp(strtok(listtmp, "_"), file) == 0) 
						count++;
				}
				filelist = (char **)malloc(sizeof(char *)*count);
				for(int i =0; i < count; i++) {
					filelist[i] = (char *)malloc(512);
				}

				listcnt=0;
				for(int i=0; i<filenum; i++) {
					strcpy(listtmp, list[i]->d_name);
					if(strcmp(strtok(listtmp, "_"), file) == 0)  {
						strcpy(filelist[listcnt], list[i]->d_name);
						listcnt++;
					}
				}

				printf(" 0. exit\n");
				for(int i=0; i<count; i++) {
					chdir(backuppath);
					stat(filelist[i], &statbuf);
					//abc.c_111111000000
					//->111111000000 날짜시간만 구하는 과정
					temp = strrchr(filelist[i], '_');
					temp +=1;
					printf("%2d. %-20s %ldbytes\n", i+1, temp, statbuf.st_size);
				}
				printf("choose file to recover : ");
				fgets(num, 8, stdin);

				chdir(currentworking);

				//exit
				if(atoi(num) == 0)
					return;
				//범위밖의 파일 골랐을때
				if(atoi(num) > count) {
					fprintf(stderr, "wrong number\n");
					return;
				}

				//n옵션이 없을때
				if(opt == NULL) {
					//기존파일 지우고 새로 만듬
					unlink(filename);

					strcpy(filetmp, backuppath);
					strcat(filetmp, "/");
					strcat(filetmp, filelist[atoi(num)-1]);


					fp1 = fopen(filetmp, "r");
					fp2 = fopen(filename, "w");


					while(1) {
						fgets(line, 1024, fp1);
						if(feof(fp1))
							break;
						fputs(line, fp2);
					}				

				}
				//n옵션이 있을때
				else {
					//새파일 생성하여 백업파일의 내용 옮겨적음
					strcpy(filetmp, backuppath);
					strcat(filetmp, "/");
					strcat(filetmp, filelist[atoi(num)-1]);

					fp1 = fopen(filetmp, "r");
					fp2 = fopen(opt, "w");

					while(1) {
						fgets(line, 1024, fp1);
						if(feof(fp1))
							break;
						fputs(line, fp2);
					}	
				}

				printf("Recovery Success!\n");
				temp = org;
				free(temp);

				for(int i = 0; i < count; i++)
					free(filelist[i]);
				free(filelist);
				fclose(fp1);
				fclose(fp2);
				return ;
			}
			before = offset;
			offset = backuptemp.next;
		}
	}

}

