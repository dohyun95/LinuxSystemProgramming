#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 1024

void mainclass(void);
void otherclass(char *line);
void makestruct(char *temp, char *rline, char *strname, char *type, char method[32]);
void makeprintf(char *rline);
char *replace(char *s, const char *olds, const char *news);
void headersearch(void);


FILE *fp, *fp1, *fp2, *fp_header, *fp_temp, *fp_temp1, *header;

struct class {
	char name[64];
} Class[64];

int classcnt=0;

struct _Object {
	char name[64];
} Object[64];
int objcnt=0;

int main(int argc, char *argv[])
{

	char line[BUFFER_SIZE];
	char *classname_get;
	char *filename;
	char *temp;
	int current;
	int opt;
	int stackbool=0;


	if ((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}



	temp = malloc(BUFFER_SIZE);
	strcpy(temp, argv[1]);

	while(strstr(temp, "/") != NULL) {
		temp = temp +1;
	}

	filename = strtok(temp, ".");

	current = ftell(fp);

	while(1) {
		if((fgets(line, BUFFER_SIZE, fp)) == NULL)
			break;

		//클래스 명이 파일이름과 같을시 -> 메인클래스
		if((temp = strstr(line, "class")) != NULL) {
			temp = temp + strlen("class") +1;
			for(int i=0; i<strlen(temp); i++) {
				if(temp[i] == '{' || temp[i] == ' '|| temp[i] == '\n' || temp[i] ==0)
					break;
				classname_get[i] = temp[i];
				classname_get[i+1] = '\0';
			}

			if(strcmp(filename, classname_get) == 0) {
				if((fp_temp = fopen("tempfile", "w+"))==NULL) {
					fprintf(stderr,"tempfile open error\n");
					exit(1);
				}
				if((fp_header = fopen("header", "r+"))==NULL) {
					fprintf(stderr, "header open error\n");
					exit(1);
				}
				char result[64];
				strcpy(result, filename);
				strcat(result, ".c");
				if((fp1 = fopen(result, "w+"))==NULL) {
					fprintf(stderr, "targefile open error\n");
					exit(1);
				}

				/*****메인클래스 구현******/
				fputs("#include \"ssu_convert.h\"\n", fp_temp);
				mainclass();
				headersearch();
				fclose(fp_temp);
				fclose(fp_header);
				fclose(fp1);
				remove("tempfile");

			}
			else {
				if((fp_temp = fopen("tempfile", "w+"))==NULL) {
					fprintf(stderr,"tempfile open error\n");
					exit(1);
				}
				if((fp_header = fopen("header", "r+"))==NULL) {
					fprintf(stderr, "header open error\n");
					exit(1);
				}
				if((fp1 = fopen("Stack.c", "w+"))==NULL) {
					fprintf(stderr, "Stackfile open error\n");
					exit(1);
				}
				if((header = fopen("ssu_convert.h", "w+"))==NULL) {
					fprintf(stderr, "targefile open error\n");
					exit(1);
				}
				if((fp_temp1 = fopen("tempfile1", "w+")) == NULL) {
					fprintf(stderr,"tempfile open error\n");
					exit(1);
				}

				fputs("#include \"ssu_convert.h\"\n", fp_temp);
				otherclass(line);
				headersearch();
				fclose(fp_temp);
				fclose(fp_header);
				fclose(fp1);
				fclose(header);
				fclose(fp_temp1);
				remove("tempfile");
				remove("tempfile1");
				stackbool=1;
			}
		}
	}
	char result[BUFFER_SIZE];
	char arg[BUFFER_SIZE];
	char mfile[BUFFER_SIZE];
	strcpy(result, filename);
	strcat(result, ".c");
	strcpy(arg, argv[1]);

	printf("%s.c converting is finished!\n", filename);

	if((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	if((fp1 = fopen(result, "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	int cnt=1;
	int size;
	while((opt = getopt(argc, argv, "jcpflr")) != -1) {

		switch(opt) {
			case 'j':
				cnt=1;
				while(1) {
					if(fgets(line, BUFFER_SIZE, fp) == NULL) {
						rewind(fp);
						break;
					}
					printf("%-3d%s", cnt, line);
					cnt++;
				}
				break;
			case 'c':
				cnt=1;
				while(1) {
					if(fgets(line, BUFFER_SIZE, fp1) == NULL) {
						rewind(fp1);
						break;
					}
					printf("%-3d%s", cnt, line);
					cnt++;
				}
				break;
			case 'p':
				break;
			case 'f':
				fseek(fp, 0, SEEK_END);
				size = ftell(fp);
				printf("%s file size is %d bytes\n", arg, size);
				fseek(fp1, 0, SEEK_END);
				size = ftell(fp1);
				printf("%s file size is %d bytes\n", result, size);
				rewind(fp);
				rewind(fp1);
				break;
			case 'l':
				cnt=0;
				while(1) {
					if(fgets(line, BUFFER_SIZE, fp) == NULL) {
						rewind(fp);
						break;
					}
					cnt++;
				}
				printf("%s line number is %d line\n", arg, cnt);
				cnt=0;
				while(1) {
					if(fgets(line, BUFFER_SIZE, fp1) == NULL) {
						rewind(fp);
						break;
					}
					cnt++;
				}
				printf("%s line number is %d line\n", result, cnt);
				break;
			case 'r':
				break;
		}
	}

	strcpy(temp, filename);
	strcat(temp, "_Makefile");
	if((fp2 = fopen(temp, "w+"))==NULL) {
		fprintf(stderr, "makefile open error\n");
		exit(1);
	}

	strcpy(temp, replace(temp, "_Makefile", ".out: "));
	fputs(temp, fp2);
	if(stackbool == 1)
		fputs(" Stack.o ", fp2);
	strcpy(temp, replace(temp, ".out: ", ".o"));
	fputs(temp, fp2);

	fputs("\n	gcc -o ", fp2);
	strcpy(temp, replace(temp, ".o", ".out "));
	fputs(temp, fp2);

	if(stackbool == 1)
		fputs(" Stack.o ",fp2);
	strcpy(temp, replace(temp, ".out ", ".o"));
	fputs(temp, fp2);
	fputs("\n",fp2);
	if(stackbool == 1)
	{
		fputs("Stack.o: Stack.c\n	gcc -c -o Stack.o Stack.c\n", fp2);
	}
	fputs(temp, fp2);
	fputs(": ", fp2);
	strcpy(temp, replace(temp, ".o", ".c"));
	fputs(temp, fp2);
	fputs("\n", fp2);
	fputs("	gcc -c -o ",fp2);
	strcpy(temp, replace(temp, ".c", ".o "));
	fputs(temp, fp2);
	strcpy(temp, replace(temp, ".o ", ".c\n"));
	fputs(temp, fp2);
	fputs("\n", fp2);

	fputs("clean:\n", fp2);
	fputs("	rm -f *.o ", fp2);
	fputs(filename, fp2);
	fputs(".out\n",fp2);

	fclose(fp2);

}

void mainclass(void) 
{
	char scanner[32];
	char *reline = malloc(BUFFER_SIZE);
	char line[BUFFER_SIZE];
	char *temp = malloc(BUFFER_SIZE);
	char *tmp = malloc(BUFFER_SIZE);
	char filedef[64];
	char exception[64][64];
	int exceptioncnt=0;
	int brack_cnt;
	int varcnt=0;
	int loopcnt=0;
	int listcnt=0;
	int delicnt=0;
	struct var{
		char name[64];
		char type[64];
	} Var[64];
	struct objlist{
		char name[64];
		char type[64];
	} Objlist[64];

	strcpy(filedef, "NONE"); 

	while(1) {
		int blank_cnt=0;
		if((fgets(line, BUFFER_SIZE, fp)) == NULL)
			break;
		char noblank[BUFFER_SIZE];
		strcpy(noblank, line);
		strcpy(noblank, strtok(noblank, "	"));

		strcpy(reline, line);
		reline = reline+1; //공백정리위함

		if(strstr(reline, "{") != NULL)
			delicnt++;
		if(strstr(reline, "}") != NULL)
			delicnt--;


		if ((tmp = strstr(reline, "new")) != NULL) {
			tmp = tmp + strlen("new ");
			temp = malloc(BUFFER_SIZE);
			strcpy(temp, tmp);

			if(strstr(tmp, "(") != NULL) {  //클래스일때
				temp = strtok(temp, "(");
				for(int i=0; i<objcnt; i++) {
					if(strcmp(temp, Object[i].name) == 0){ //클래스 리스트에 있는지확인
						char rep[BUFFER_SIZE];
						reline = replace(reline, "new ", "new");
						strcpy(temp, reline);
						temp = strtok(temp, "	 ");
						strcpy(Objlist[listcnt].type, temp);
						temp = strtok(NULL, "	 ");
						strcpy(Objlist[listcnt].name, temp);
						strcpy(rep, "*");
						strcat(rep, Objlist[listcnt].name);

						strcpy(reline, replace(reline, Objlist[listcnt].name, rep)); 
						listcnt++;
					}
				}
			}
		}

		tmp =malloc(BUFFER_SIZE);
		char *name=(char *)malloc(128);
		for(int i=0; i<listcnt; i++)
		{
			strcpy(tmp, Objlist[i].name);
			strcat(tmp, ".");
			if(strstr(reline, tmp) != NULL) {
				char tmp1[64];
				char tmp2[64];
				char tmp3[64];
				char tmp4[64];
				strcpy(tmp1, Objlist[i].name);
				strcpy(tmp2, Objlist[i].name);
				strcat(tmp1, ".");
				strcat(tmp2, "->");
				reline = replace(reline, tmp1, tmp2);
				tmp = strstr(reline, tmp2);
				if((tmp = strstr(tmp, "(")) != NULL) {  //st.aaa()같은경우
					strcpy(tmp1, tmp);
					strcpy(tmp3, reline);
					strcpy(name, strstr(tmp3, "->"));
					name = name + strlen("->");
					strcpy(name, strtok(name, "("));
					strcpy(tmp1, strtok(tmp1, ")"));
					strcpy(tmp2, strstr(reline, Objlist[i].name));
					if(strcmp("(", tmp1) == 0) {
						strcpy(tmp3, name);
						strcat(tmp3, "(");
						strcat(tmp3, Objlist[i].name);

						strcpy(tmp4, name);
						strcat(tmp4, "(");
						reline = replace(reline, tmp4, tmp3);  //st.aaa() -> st.aaa(st)
					}
					else {
						strcpy(tmp3, name);
						strcat(tmp3, "(");
						strcat(tmp3, Objlist[i].name);
						strcat(tmp3, ", ");
						strcpy(tmp4, name);
						strcat(tmp4, "(");		
						reline = replace(reline, tmp4 ,tmp3); //st.aaa(5) -> st.aaa(st, 5)
					}
				}
			}
		}



		//int main 생성
		if((temp = strstr(reline, "public static void main")) != NULL) {
			fputs("int main(int argc, char *argv[])", fp_temp);
			fputs("{\n", fp_temp);
			brack_cnt++;
			if(strstr(temp, "throws") != NULL) {
				strcpy(temp, strstr(temp, "throws"));
				strtok(temp, " ");
				strcpy(temp, strtok(NULL, "{ "));
				strcpy(exception[exceptioncnt], temp);
				exceptioncnt++;
			}

		}
		//scanf 생성
		else if((temp = strstr(reline, "new Scanner(System.in)")) != NULL) {
			char *scan_name;
			scan_name = strtok(reline, "	 ");
			while(scan_name != NULL)
			{
				if(strcmp(scan_name, "Scanner") == 0) {
					scan_name = strtok(NULL, " ");
					strcpy(scanner, scan_name);
					break;
				}
			}
		}

		else if((temp = strstr(reline, "nextInt()")) != NULL) {
			char scantemp[128];
			char *scanvar;
			int cnt=0;
			while(1) {
				if(reline[cnt] == 9)
					cnt++;
				else 
					break;
			}
			for(int i=0; i<cnt; i++) {
				fputs("	",fp_temp);
			}
			strcpy(scantemp, scanner);
			strcat(scantemp, ".nextInt()");
			if(strstr(reline, scantemp) != NULL) {
				scanvar = strtok(reline, "=");
				scanvar = strtok(scanvar, "	");
				fputs("scanf(\"\%d\", &",fp_temp);
				fputs(scanvar,fp_temp);
				fputs(");\n", fp_temp);
			}
		}
		//printf생성
		else if(strstr(reline, "System.out.printf") != NULL) {
			int cnt=0;
			while(1) {
				if(reline[cnt] == 9)
					cnt++;
				else 
					break;
			}
			for(int i=0; i<cnt; i++) {
				fputs("	",fp_temp);
			}

			temp = strstr(reline,"printf");
			fputs(temp, fp_temp);
		}

		//return생성
		else if(strstr(reline, "return") != NULL) {
			//아무일도 하지않음
		}


		//빈줄 구현
		//빈줄중에 \n만으로 이루어진것도있고 \n과 공백,tab
		//으로 이루어진것도 있어서 공백과 Tab을 제거한게 noblank
		else if(strcmp (noblank, "\n") == 0) {
			fputs("\n",fp_temp);
		}

		//파일입출력관련
		else if (strstr(reline, "new File(") != NULL) {
			char temp[BUFFER_SIZE];
			char fname[BUFFER_SIZE];
			char file[BUFFER_SIZE];
			int cnt=0;
			while(1) {
				if(reline[cnt] == 9)
					cnt++;
				else 
					break;
			}
			for(int i=0; i<cnt; i++) {
				fputs("	",fp_temp);
			}


			strcpy(fname, "char *");

			strcpy(temp, reline);
			strcpy(temp, strtok(temp, "="));
			strtok(temp, " 	");
			strcpy(file, strtok(NULL, "  "));

			strcat(fname, file);
			strcat(fname, " = ");

			strcpy(temp, strstr(reline, "new File("));
			strtok(temp, "(");
			strcpy(temp, strtok(NULL, ")"));

			strcat(fname, temp);
			strcat(fname, ";\n");
			strcpy(fname, strtok(fname, "	"));
			fputs(fname,fp_temp);

			for(int i=0; i<exceptioncnt; i++) {
				if(strcmp(exception[i], "IOException") == 0) {
					char errorprt[BUFFER_SIZE];
					strcpy(errorprt, "	if(access(");
					strcat(errorprt, file);
					strcat(errorprt, ", 0) == -1) {\n");
					fputs(errorprt,fp_temp);

					fputs("		fprintf(stderr, \"No file!\\n\");\n", fp_temp);
					fputs("		exit(1);\n", fp_temp);
					fputs("	}", fp_temp);


				}
			}

		}
		else if (strstr(reline, "new FileWriter(") != NULL) {
			char temp[BUFFER_SIZE];
			char file[BUFFER_SIZE];
			char linetemp[BUFFER_SIZE];
			int cnt=0;
			while(1) {
				if(reline[cnt] == 9)
					cnt++;
				else 
					break;
			}
			for(int i=0; i<cnt; i++) {
				fputs("	", fp_temp);	
			}


			strcpy(temp, reline);
			strcpy(temp, strtok(temp, "="));
			strtok(temp, " 	");
			strcpy(temp, strtok(NULL, "	 "));
			strcpy(Var[varcnt].name, temp);
			strcpy(filedef, temp);
			strcat(filedef, ".");


			strcpy(linetemp, "FILE *");
			strcat(linetemp, temp);
			strcat(linetemp, ";");
			fputs(linetemp, fp_temp);
			fputs("\n", fp_temp);

			strcpy(linetemp, reline);
			strcpy(file, strstr(reline, "new FileWriter"));
			strtok(file, "(");
			strtok(NULL, ", ");
			strcpy(file, strtok(NULL, ")"));

			if(strstr(file, "false") != NULL) {
				strcpy(linetemp, replace(linetemp, "new FileWriter", "fopen"));
				strcpy(linetemp, replace(linetemp, "false", "\"w\""));

			}
			else if(strstr(file, "true") != NULL) {
				strcpy(linetemp, replace(linetemp, "new FileWriter", "fopen"));
				strcpy(linetemp, replace(linetemp, "true", "\"a\""));
			}

			strcpy(linetemp, strtok(linetemp, "	"));
			strcpy(linetemp, replace(linetemp, "FileWriter ", ""));
			for(int i=0; i<cnt; i++) {
				fputs("	", fp_temp);
			}


			fputs(linetemp, fp_temp);

			strcpy(temp, reline);
			strcpy(temp, strstr(temp, "FileWriter("));
			strcpy(temp, strtok(temp, "("));

			strcpy(Var[varcnt].type, temp);
			varcnt++;
		}
		//"writer." 을 검색
		else if (strstr(reline, filedef) != NULL){

			int cnt=0;
			while(1) {
				if(reline[cnt] == 9)
					cnt++;
				else 
					break;
			}
			for(int i=0; i<cnt; i++) {
				fputs("	", fp_temp);
			}


			if (strstr(reline, ".write") != NULL) {
				char temp[64];
				char type[64];
				char linetemp[BUFFER_SIZE];
				strcpy(temp, reline);
				strcpy(temp, strtok(temp, "	."));
				for(int i=0; i<varcnt; i++)
					if(strcmp(Var[i].name, temp) == 0)
						strcpy(type, Var[i].type);
				if(strcmp(type, "FileWriter") == 0)
				{
					strcpy(linetemp, strstr(reline, ".write("));
					strtok(linetemp, "(");
					strcpy(linetemp, strtok(NULL, ")"));
					strcpy(type, "fputs(");
					strcat(type, linetemp);
					strcat(type, ", ");
					strcat(type, temp);
					strcat(type, ");\n");
					strcpy(linetemp, type);

				}
				strcpy(linetemp, strtok(linetemp, "	")); 
				fputs(linetemp, fp_temp);
			}
			else if(strstr(reline, ".flush") != NULL) {
				char linetemp[BUFFER_SIZE];
				char temp[BUFFER_SIZE];
				strcpy(temp, filedef);
				strtok(temp, ".");

				strcpy(linetemp, "(");
				strcat(linetemp, temp);
				strcat(linetemp, ")");
				strcpy(temp, linetemp);

				strcpy(linetemp, reline);
				strcpy(linetemp, replace(linetemp, filedef, ""));
				strcpy(linetemp, replace(linetemp, "flush", "fflush"));
				strcpy(linetemp, replace(linetemp, "()", temp));
				strcpy(linetemp, strtok(linetemp, "	"));

				strcpy(linetemp, strtok(linetemp, "	")); 
				fputs(linetemp, fp_temp);

			}
			else if(strstr(reline, ".close") != NULL) {
				char linetemp[BUFFER_SIZE];
				char temp[BUFFER_SIZE];
				strcpy(temp, filedef);
				strtok(temp, ".");

				strcpy(linetemp, "(");
				strcat(linetemp, temp);
				strcat(linetemp, ")");
				strcpy(temp, linetemp);

				strcpy(linetemp, reline);
				strcpy(linetemp, replace(linetemp, filedef, ""));
				strcpy(linetemp, replace(linetemp, "close", "fclose"));
				strcpy(linetemp, replace(linetemp, "()", temp));
				strcpy(linetemp, strtok(linetemp, "	"));

				strcpy(linetemp, strtok(linetemp, "	")); 
				fputs(linetemp, fp_temp);


			}
		}
		else {
			if(delicnt == 0) {
				fputs("	return 0;\n", fp_temp);
				delicnt = -1;
			}
			if(strstr(reline, "null") != NULL)
				strcpy(reline, replace(reline, "null", "NULL"));
			fputs(reline, fp_temp);
		}

	}
}

void otherclass(char *line)
{
	char rline[BUFFER_SIZE];
	char *temp=malloc(BUFFER_SIZE);
	char *strname=(char *)malloc(BUFFER_SIZE);
	char classname_get[BUFFER_SIZE];
	char methodname[32][32];
	int methodcnt=0;
	int loop=0;
	int cnt;
	int linecnt=0;
	long current;

	struct Vartype {
		char name[32];
		char type[32];
	} vartype[32];
	int varcnt=0;

	current = ftell(fp); //클래스를 읽은 위치 기억

	if((temp = strstr(line, "class")) != NULL) {
		temp = temp + strlen("class") +1;
		for(int i=0; i<strlen(temp); i++) {
			if(temp[i] == '{' || temp[i] == ' '|| temp[i] == '\n' || temp[i] ==0)
				break;
			classname_get[i] = temp[i];
			classname_get[i+1] = '\0';
		}
	}

	strcpy(Object[objcnt].name, classname_get);
	objcnt++;

	if(strstr(line, "{") != NULL) {
		loop=1;
	}

	while(1) {
		if(fgets(rline, BUFFER_SIZE, fp) == NULL) {
			break;
		}
		linecnt++;

		if(strstr(rline, "{") != NULL)
			loop++;
		if(strstr(rline, "}") != NULL)
			loop--;
		if(loop == 0) {
			fseek(fp, current, 0);
			break;
		}
	}



	//구조체 만들기
	fputs("typedef ", header);
	strcpy(strname, "struct _");
	strcat(strname, classname_get);
	fputs(strname, header);
	fputs("{\n", header);
	int fdelcnt=0;
	int position;

	while(1)
	{
		if(cnt == linecnt) {

			fputs("}", header);

			fputs(classname_get, header);
			fputs(";\n", header);

			fseek(fp, current, 0);
			cnt = 0;
			break;
		}


		if(fgets(rline, BUFFER_SIZE, fp) == NULL)
			break;

		if(strstr(rline, "}") != NULL) {
			fdelcnt--;
			if(fdelcnt == 0) {
				cnt++;
				continue;
			}
		}

		if(fdelcnt != 0) {
			cnt++;
			continue;
		}
		if(strstr(rline, "{") != NULL) {
			fdelcnt++;
		}


		//static final -> #define
		if(strstr(rline, "static final") != NULL) 
		{
			char *temp = malloc(BUFFER_SIZE);
			char *define = malloc(BUFFER_SIZE);
			strcpy(temp, rline);
			strcpy(define, "#define ");

			strtok(temp, "=");
			temp = strstr(temp, "static");
			strtok(temp, " ");
			strtok(NULL, " ");
			strtok(NULL, " ");
			temp = strtok(NULL, " ");

			strcat(define, temp);

			strcpy(temp, rline);
			temp = strstr(temp, "=");
			temp = temp+1;
			strtok(temp, ";");

			strcat(define, " ");
			strcat(define, temp);
			fputs("	", header);
			fputs(define, header);

		}
		//함수가 아님을 구분
		if(strstr(rline, "(") == NULL) {
			if(strstr(rline, "[]") !=NULL) {
				char *tmp = malloc(BUFFER_SIZE);
				strcpy(tmp, rline);

				temp = strtok(tmp, "[");
				fputs(temp, header);
				fputs("*", header);
				temp = strtok(NULL, "]");

				fputs(temp, header);

				temp = strtok(temp, "; ");

				strcpy(vartype[varcnt].name, temp);

				strcpy(tmp, rline);
				temp = strtok(tmp, "	 ");
				strcpy(vartype[varcnt].type, tmp); 
				varcnt++;
			}
			else if(strstr(rline, "static final") ==0 ){
				char *tmp = malloc(BUFFER_SIZE);
				fputs(rline, header);

				strcpy(tmp, rline);

				temp = strtok(tmp, "\n	 ");
				if(temp == NULL || (strcmp(temp, "}") == 0)){
					cnt++;
					continue; //빈줄일때 다음행으로 넘어감
				}

				strcpy(tmp, rline);

				strtok(tmp, " ");
				strcpy(vartype[varcnt].type, temp);
				temp = strtok(NULL, " ");
				temp = strtok(temp, ";");
				strcpy(vartype[varcnt].name, temp);
				varcnt++;

			}
		}
		//함수일때
		else {
			char *temprline=malloc(BUFFER_SIZE);
			char *tmp;
			strcpy(temprline, rline);
			temp = strtok(temprline, "(");

			if((tmp = strstr(temp, "int ")) != NULL) {

				makestruct(tmp, rline, strname, "int ", methodname[methodcnt]);
				methodcnt++;
			}
			else if((tmp = strstr(temp, "void ")) != NULL) {
				makestruct(tmp, rline, strname, "void ", methodname[methodcnt]);
				methodcnt++;
			}
			else if((tmp = strstr(temp, "char ")) != NULL) {

				makestruct(tmp, rline, strname, "char ", methodname[methodcnt]);
				methodcnt++;
			}

		}
		cnt++;
	}

	//생성자와 메소드 구현
	char constructor[BUFFER_SIZE];
	char nClass[64];
	char nClsptr[64];
	char structvar[64];

	strcpy(nClass, "n");
	strcat(nClass, classname_get);

	strcpy(constructor, classname_get);
	strcat(constructor, "(");

	strcpy(nClsptr, nClass);
	strcat(nClsptr, "->");

	while(1) {
		if(cnt == linecnt) {
			rewind(fp_temp1);
			while(1) {
				if(fgets(rline, BUFFER_SIZE, fp_temp1) == NULL)
					break;
				fputs(rline, fp_temp);
			}


			cnt = 0;
			break;
		}
		cnt++;

		if(fgets(rline, BUFFER_SIZE, fp) == NULL)
			break;

		strcpy(temp, rline);
		temp = strtok(temp, "	 "); //공백,탭을 없앴을때 \n만있는경우.즉, 빈줄
		if(strstr(temp, "\n")) {
			fputs("\n", fp_temp);
		}


		if((strstr(rline, "(") != NULL) && (strstr(rline, constructor) == NULL))//생성자X, 함수O
		{
			int delicnt=0;
			char *tmp = malloc(BUFFER_SIZE);
			char *func = malloc(BUFFER_SIZE);
			char *rlinecut = malloc(BUFFER_SIZE);

			if(strstr(rline, "{"))
				delicnt++;
			if(strstr(rline, "}"))
				delicnt--;

			strcpy(rlinecut, rline);
			strtok(rlinecut, "(");
			strcat(rlinecut, "(");

			if((tmp = strstr(rlinecut, "int ")) != NULL) {
				tmp = strtok(tmp, ")");
				strcpy(func, tmp);
				tmp = strstr(rline, "(");
				tmp = strtok(tmp, ")");

				strcat(func, classname_get);
				strcat(func, " *");
				strcat(func, nClass);

				if(strcmp(tmp, "(") == 0) //매개변수가 없음
				{
					strcat(func, ") {\n");
				}
				else
				{
					tmp = strtok(tmp, "(");
					strcat(func, ", ");
					strcat(func, tmp);
					strcat(func, ") {\n");
				}

				fputs(func, fp_temp);
				while(1) //함수 내용 구현
				{
					if(fgets(rline, BUFFER_SIZE, fp) == NULL)
						break;
					cnt++;
					if(strstr(rline, "}"))
						delicnt--;

					for(int i=0; i<delicnt; i++) {
						fputs("	", fp_temp);
					}
					if(strstr(rline, "{"))
						delicnt++;


					if(delicnt == 0) {
						fputs("}\n", fp_temp);
						break;
					}

					if(strstr(rline, "System.out.printf") != NULL) {
						char *printtmp = malloc(BUFFER_SIZE);
						char *storetmp = malloc(BUFFER_SIZE);
						strcpy(printtmp, rline);
						strtok(printtmp, "\"");
						printtmp = strtok(NULL, "\"");
						tmp = strstr(tmp, "(");
						if(strstr(tmp, "+") != NULL) {
							tmp = strtok(tmp, "+");
							strcpy(storetmp, tmp+1);
							if(strstr(tmp, "\"") == NULL) {
								if(strstr(tmp, "[") != NULL) { //배열변수일때
									tmp = strtok(tmp, "[");
									for(int i=0; i<varcnt;i++) {
										if(strstr(tmp, vartype[i].name) != NULL){
											if(strstr(vartype[i].type, "int") != NULL)
												strcpy(tmp, "(\"\%d");
											else if(strstr(vartype[i].type, "char") !=NULL)
												strcpy(tmp, "(\"\%c");
											else if(strstr(vartype[i].type, "char[]")!=NULL)
												strcpy(tmp, "(\"\%s");
											strcat(tmp, printtmp);
											strcat(tmp, "\"");
											strcat(tmp, ", ");
											strcat(tmp, storetmp);
											strcat(tmp, ");");
											strcpy(printtmp, rline);
											strtok(printtmp, "(");
											strcat(printtmp, tmp);
										}
									}
								}
								else { //변수가 배열이 아닐때
									for(int i=0; i<varcnt;i++) {
										if(strstr(tmp, vartype[i].name) != NULL){
											if(strstr(vartype[i].type, "int") != 0)
												strcpy(tmp, "(\"\%d");
											else if(strstr(vartype[i].type, "char") !=0)
												strcpy(tmp, "(\"\%c");
											else if(strstr(vartype[i].type, "char[]")!=0)
												strcpy(tmp, "(\"\%s");
											strcat(tmp, printtmp);
											strcat(tmp, "\"");
											strcat(tmp, ", ");
											strcat(tmp, storetmp);
											strcat(tmp, ");");
											strcpy(printtmp, rline);
											strtok(printtmp, "(");
											strcat(printtmp, tmp);
										}
									}

								}
								for(int i=0; i<varcnt; i++)
									if(strstr(printtmp, vartype[i].name)!=NULL)
									{
										strcpy(tmp, nClass);
										strcat(tmp, "->");
										strcat(tmp, vartype[i].name);
										strcpy(printtmp, replace(printtmp,vartype[i].name, tmp));
									}

								strcpy(printtmp, replace(printtmp, "System.out.", ""));
								printtmp = strtok(printtmp, "	");
								fputs(printtmp, fp_temp);
								fputs("\n",fp_temp);

							}
							else { //" "+ str 형일때인데 시간날때 구현하기

							}
						}



					}
					else {
						strcpy(temp, rline);
						for(int i=0; i<varcnt; i++)
							if(strstr(temp, vartype[i].name)!=NULL)
							{
								strcpy(tmp, nClass);
								strcat(tmp, "->");
								strcat(tmp, vartype[i].name);
								strcpy(temp, replace(temp,vartype[i].name, tmp));
							}
						temp = strtok(temp, "	");

						fputs(temp, fp_temp);
					}
				}


			}
			else if((tmp = strstr(rlinecut, "void ")) != NULL)
			{
				tmp = strtok(tmp, ")");
				strcpy(func, tmp);
				tmp = strstr(rline, "(");
				tmp = strtok(tmp, ")");

				strcat(func, classname_get);
				strcat(func, " *");
				strcat(func, nClass);

				if(strcmp(tmp, "(") == 0) //매개변수가 없음
				{
					strcat(func, ") {\n");
				}
				else
				{
					tmp = strtok(tmp, "(");
					strcat(func, ", ");
					strcat(func, tmp);
					strcat(func, ") {\n");
				}
				fputs(func, fp_temp);
				while(1) //함수 내용 구현
				{
					if(fgets(rline, BUFFER_SIZE, fp) == NULL)
						break;
					cnt++;

					if(strstr(rline, "}"))
						delicnt--;

					for(int i=0; i<delicnt; i++) {
						fputs("	",fp_temp);
					}

					if(strstr(rline, "{"))
						delicnt++;
					if(delicnt == 0) {
						fputs("}\n",fp_temp);
						break;
					}

					if(strstr(rline, "System.out.printf") != NULL) {
						char *printtmp = malloc(BUFFER_SIZE);
						char *storetmp = malloc(BUFFER_SIZE);
						strcpy(printtmp, rline);
						strtok(printtmp, "\"");
						printtmp = strtok(NULL, "\"");
						tmp = strstr(rline, "(");
						if(strstr(tmp, "+") != NULL) {
							tmp = strtok(tmp, "+");
							strcpy(storetmp, tmp+1);
							if(strstr(tmp, "\"") == NULL) {
								if(strstr(tmp, "[") != NULL) { //배열변수일때
									tmp = strtok(tmp, "[");
									for(int i=0; i<varcnt;i++) {
										if(strstr(tmp, vartype[i].name) != NULL){
											if(strstr(vartype[i].type, "int") != NULL)
												strcpy(tmp, "(\"\%d");
											else if(strstr(vartype[i].type, "char") !=NULL)
												strcpy(tmp, "(\"\%c");
											else if(strstr(vartype[i].type, "char[]")!=NULL)
												strcpy(tmp, "(\"\%s");
											strcat(tmp, printtmp);
											strcat(tmp, "\"");
											strcat(tmp, ", ");
											strcat(tmp, storetmp);
											strcat(tmp, ");");
											strcpy(printtmp, rline);
											strtok(printtmp, "(");
											strcat(printtmp, tmp);
										}
									}
								}
								else { //변수가 배열이 아닐때
									for(int i=0; i<varcnt;i++) {
										if(strstr(tmp, vartype[i].name) != NULL){
											if(strstr(vartype[i].type, "int") != 0)
												strcpy(tmp, "(\"\%d");
											else if(strstr(vartype[i].type, "char") !=0)
												strcpy(tmp, "(\"\%c");
											else if(strstr(vartype[i].type, "char[]")!=0)
												strcpy(tmp, "(\"\%s");
											strcat(tmp, printtmp);
											strcat(tmp, "\"");
											strcat(tmp, ", ");
											strcat(tmp, storetmp);
											strcat(tmp, ");");
											strcpy(printtmp, rline);
											strtok(printtmp, "(");
											strcat(printtmp, tmp);
										}
									}

								}
								for(int i=0; i<varcnt; i++)
									if(strstr(printtmp, vartype[i].name)!=NULL)
									{
										strcpy(tmp, nClass);
										strcat(tmp, "->");
										strcat(tmp, vartype[i].name);
										printtmp = replace(printtmp,vartype[i].name, tmp);
									}

								printtmp = replace(printtmp, "System.out.", "");
								printtmp = strtok(printtmp, "	");
								fputs(printtmp, fp_temp);

							}
						}
						else {
							strcpy(temp, rline);
							for(int i=0; i<varcnt; i++)
								if(strstr(temp, vartype[i].name)!=NULL)
								{
									strcpy(tmp, nClass);
									strcat(tmp, "->");
									strcat(tmp, vartype[i].name);
									temp = replace(temp,vartype[i].name, tmp);
								}

							temp = replace(temp, "System.out.", "");
							temp = strtok(temp, "	");
							fputs(temp, fp_temp);

						}
					}
					else {
						strcpy(temp, rline);
						for(int i=0; i<varcnt; i++)
							if(strstr(temp, vartype[i].name)!=NULL)
							{
								strcpy(tmp, nClass);
								strcat(tmp, "->");
								strcat(tmp, vartype[i].name);
								strcpy(temp, replace(temp,vartype[i].name, tmp));
							}
						temp = strtok(temp, "	");
						fputs(temp, fp_temp);
					}
				}



			}


		}
		else if(strstr(rline, constructor) != NULL) {//생성자있다면
			char *temp = malloc(BUFFER_SIZE);
			strcpy(temp, rline);
			int delicnt=0;

			strcpy(constructor, classname_get);
			strcat(constructor, " *new");
			strcat(constructor, classname_get);
			strcat(constructor, "(");
			//생성자의 매개변수 유무
			temp = strstr(temp, "(");
			temp = strtok(temp,  ")");
			if(strcmp(temp, "(") == 0) //매개변수 X
				strcat(constructor, "void)");
			else {//매개변수 O
				temp = temp+1; //"(" 없애는과정
				strcat(constructor, temp);
				strcat(constructor, ")");
			}
			fputs(constructor, header);
			fputs(";\n", header);
			strcat(constructor, " {");

			fputs(constructor, fp_temp1);
			fputs("\n", fp_temp1);

			//Constructor->C언어 과정에서 생기는 별도의 코드
			strcpy(constructor, "	");
			strcat(constructor, classname_get);
			strcat(constructor, " *");
			strcat(constructor, nClass);
			strcat(constructor, "=(");
			strcat(constructor, classname_get);
			strcat(constructor, " *)");
			strcat(constructor, "malloc(sizeof(");
			strcat(constructor, classname_get);
			strcat(constructor, "));\n");

			fputs(constructor, fp_temp1);

			//새줄시작
			for(int i=0; i<methodcnt; i++) {
				strcpy(constructor, "	");
				strcat(constructor, nClsptr);
				strcat(constructor, methodname[i]);
				strcat(constructor, "=");
				strcat(constructor, methodname[i]);
				strcat(constructor, ";\n");
				fputs(constructor, fp_temp1);
			}


			if(strstr(rline, "{") !=NULL)
				delicnt++;
			if(strstr(rline, "}") != NULL)
				delicnt--;

			//constructor안의 기본내용 정리
			while(1) {
				char *tmp=malloc(BUFFER_SIZE);
				char *type=malloc(BUFFER_SIZE);
				fgets(rline, BUFFER_SIZE, fp);
				if(strstr(rline, "{") != NULL)
					delicnt++;
				if(strstr(rline, "}") != NULL)
					delicnt--;
				if(delicnt == 0) {
					strcpy(temp, "	return ");
					strcat(temp, nClass);
					strcat(temp, ";");

					fputs(temp, fp_temp1);
					fputs("\n}\n", fp_temp1);
					break;
				}

				if(strstr(rline, "new ") != NULL) {
					strcpy(tmp, rline);
					tmp = strtok(tmp, "= 	");
					for(int i=0; i<varcnt; i++) {
						if(strcmp(vartype[i].name, tmp) == 0) {
							if(strstr(vartype[i].type, "[]"))
							{
								tmp = strtok(rline, "[");
								tmp = strtok(NULL, "]");
								strcpy(temp, nClsptr);
								strcat(temp, vartype[i].name);
								strcat(temp, " = ");
								strcat(temp, "malloc(");
								strcat(temp, tmp);
								strcat(temp, ");\n");
							}
						}
					}
					fputs("	", fp_temp1);

					fputs(temp, fp_temp1);

				}
				else {
					for(int i=0; i<varcnt; i++) {
						if(strstr(rline, vartype[i].name) !=0) {
							strcpy(tmp, nClsptr);
							strcat(tmp, vartype[i].name);
							strcpy(temp, replace(rline, vartype[i].name, tmp)); 
						}
					}
					temp = strtok(temp, "	");

					fputs("	", fp_temp1);
					fputs(temp, fp_temp1);
				}
				cnt++;
			}
		} 




	}
}


void makestruct(char *temp, char *rline, char *strname, char *type, char method[32])
{
	char *name=malloc(BUFFER_SIZE);
	char *param;
	temp = temp + strlen(type);
	strcpy(method, temp);
	temp = strtok(temp, "(");

	strcpy(name, type);
	strcat(name, "(*");
	strcat(name, temp);
	strcat(name, ")");

	//함수에 매개변수가 없을때
	temp = strstr(rline, "(");
	param = strtok(temp, ")");
	if(strcmp(param, "(") == 0) {
		strcat(name, "(");
		strcat(name, strname);
		strcat(name, "*);");
	}
	//함수에 매개변수가 있을때
	else {
		char *toktemp = malloc(BUFFER_SIZE);
		temp = temp+1;
		strcpy(toktemp, temp);
		toktemp = strtok(toktemp, " ");
		strcat(name, "(");
		strcat(name, strname);
		strcat(name, "*, ");
		strcat(name, toktemp);
		while(1) {
			if((toktemp = strstr(temp, ",")) == NULL)
				break;
			toktemp = toktemp +1;
			toktemp = strtok(toktemp, " ");
			strcat(name, ", ");
			strcat(name, toktemp);
		}
		strcat(name, ");");
	}
	fputs("	", header);
	fputs(name, header);

}

char *replace(char *s, const char *olds, const char *news)
{
	char *result, *sr;
	size_t i, count = 0;
	size_t oldlen = strlen(olds); if (oldlen < 1) return s;
	size_t newlen = strlen(news);


	if (newlen != oldlen) {
		for (i = 0; s[i] != '\0';) {
			if (memcmp(&s[i], olds, oldlen) == 0)  {
				count++;
				i += oldlen;
			}
			else i++;
		}
	} 
	else i = strlen(s);


	result = (char *) malloc(i + 1 + count * (newlen - oldlen));
	if (result == NULL) 
		return NULL;


	sr = result;
	while (*s) {
		if (memcmp(s, olds, oldlen) == 0) {
			memcpy(sr, news, newlen);
			sr += newlen;
			s  += oldlen;
		} 
		else *sr++ = *s++;
	}
	*sr = '\0';

	return result;
}

void headersearch(void) {
	char *buf = malloc(BUFFER_SIZE);
	char *head = malloc(BUFFER_SIZE);
	char *headtemp = malloc(BUFFER_SIZE);
	char temp[BUFFER_SIZE];
	int headcnt=0;
	int a=1;
	while(1) {
		if(fgets(head, BUFFER_SIZE, fp_header) == NULL)
			break;
		strcpy(temp, head);

		strcpy(temp, strtok(temp, " ")); //ex) open
		strcat(temp, "("); //ex) open(

		rewind(fp_temp);
		while(1) {
			if(fgets(buf, BUFFER_SIZE, fp_temp) == NULL)
				break;
			if(strstr(buf, temp) != NULL) {
				headtemp = malloc(BUFFER_SIZE);
				strcpy(headtemp, head);
				while(1) {
					headtemp = strstr(headtemp, "#");
					if(headtemp == NULL)
						break;
					strcpy(temp, headtemp);
					strcpy(buf, strtok(temp, ">"));
					strcat(buf, ">");

					rewind(fp1);
					for(int i=0; i<headcnt; i++)
					{
						fgets(temp, BUFFER_SIZE, fp1);

						strtok(temp, "'\n'");
						if(strcmp(temp, buf) == 0)
							a=0;
					}
					if(a == 1) {
						fputs(buf, fp1);
						fputs("\n", fp1);
						headcnt++;
					}
					a=1;
					headtemp= headtemp+1;
				}

				break;
			}
		}
	}
	fputs("\n", fp1);
	rewind(fp_header);
	rewind(fp_temp);
	while(1) {
		if(fgets(buf, BUFFER_SIZE, fp_temp) == NULL)
			break;
		fputs(buf, fp1);
	}
}
