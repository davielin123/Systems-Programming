#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>

int fd, fd_log;
FILE *fp, *fp_log, *fp_out;
int serial_number[3] = {0};
pid_t ppid; //parent process id
static jmp_buf env_alrm;
int send_num = 0, finish_num = 0;
int output[10000][3]; // 0:
int log_times = 0;
int ordinary_num = 0;


void one_handler(int signo) {
	finish_num++;
	output[log_times][0] = 1;	output[log_times][1] = 1;	output[log_times][2] = serial_number[1];	log_times++;
	int remain = alarm(0); 
//	fprintf(stderr, "get SIGUSR1\n");
	return;
}

void two_handler(int signo) {
	finish_num++;
	output[log_times][0] = 1;	output[log_times][1] = 2;	output[log_times][2] = serial_number[2];	log_times++;
//	fprintf(stderr, "get SIGUSR2\n");
	return;
}

void int_handler(int signo) {
	ordinary_num++;
	finish_num++;
	output[log_times][0] = 1;	output[log_times][1] = 0;	output[log_times][2] = ordinary_num;	log_times++;
//	fprintf(stderr, "get SIGINT\n");
	return;
}

void write_log() {
	
	for(int i = 0 ; i < log_times ; i++){
//		fprintf(stderr, "i = %d\n", i);
		if(output[i][0] == 0) {
			switch(output[i][1]) {
				case 0:
					fprintf(fp_log, "send 0 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 1:
					fprintf(fp_log, "send 1 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 2:
					fprintf(fp_log, "send 2 %d\n", output[i][2]);
					fflush(fp_log);
					break;
			}
		}
		else if(output[i][0] == 1) {
			switch(output[i][1]) {
				case 0:
					fprintf(fp_log, "finish 0 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 1:
					fprintf(fp_log, "finish 1 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 2:
					fprintf(fp_log, "finish 2 %d\n", output[i][2]);
					fflush(fp_log);
					break;
			}
		}
		else if(output[i][0] == 2) {
			fprintf(fp_log, "timeout 1 %d\n", output[i][2]);
			fflush(fp_log);
		}
	}
	return;
}

void alarm_handler(int signo) {
//	fprintf(stderr, "I'm in!\n");

	output[log_times][0] = 2;	output[log_times][1] = 1;	output[log_times][2] = serial_number[1];	log_times++;
	write_log();
	close(1);
	
	exit(0);
}


int main(int argc, char *argv[]) {
	
	/*算時間用*/
	struct tms tmsstart, tmsend;
	clock_t start, end;
	static long clktck = 0;
	clktck = sysconf(_SC_CLK_TCK);
	start = times(&tmsstart); //開始計時
	
	/*nanosleep用 每次0.01 sec*/
	struct timespec test;
	test.tv_sec = 0;
	test.tv_nsec = 10000000;
	
	ppid = getppid();
	char buf_from[100];
	fd = open(argv[1], O_RDONLY, 0700);
	fd_log = open("customer_log", O_WRONLY | O_CREAT | O_APPEND, 0700);
	fp = fdopen(fd, "r");
	fp_log = fdopen(fd_log, "w");
	fp_out = fdopen(1, "w");
	int read_end;
	int now_code;
	double send_time;
	
	/*註冊 SIGALRM*/
	struct sigaction act;
	act.sa_handler = alarm_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGUSR1);
	sigaddset(&act.sa_mask, SIGUSR2);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);
	
	/*註冊SIGUSR1 SIGUSR2, SIGINT*/
	struct sigaction act_one, act_two, act_int;
	
	act_one.sa_handler = one_handler;
	sigemptyset(&act_one.sa_mask);
	act_one.sa_flags = 0;
	sigaction(SIGUSR1, &act_one, NULL);
	
	act_two.sa_handler = two_handler;
	sigemptyset(&act_two.sa_mask);
	act_two.sa_flags = 0;
	sigaction(SIGUSR2, &act_two, NULL);
	
	act_int.sa_handler = int_handler;
	sigemptyset(&act_int.sa_mask);
	act_int.sa_flags = 0;
	sigaction(SIGINT, &act_int, NULL);
	
	while(1) {
		
		fgets(buf_from, 100, fp);
		fflush(fp);
		if(feof(fp)) {
			while(send_num > finish_num) {
				sleep(1);
			}
			break;
		}
//		fprintf(stderr, "buf_from = %s", buf_from);
		sscanf(buf_from, "%d %lf", &now_code, &send_time);
//		fprintf(stderr, "now_code = %d, send_time = %f\n", now_code, send_time);
		
		end = times(&tmsend);
		
//		fprintf(stderr, "out[up] = %f\n", (end - start)/(double)clktck);
		
		/*睡到秒數到*/
		while((end - start)/(double)clktck < send_time) {
			nanosleep(&test, NULL);
			end = times(&tmsend);
//			fprintf(stderr, "out = %f\n", (end - start)/(double)clktck);
		}
		
		
		switch(now_code) {
			case 0:
				fprintf(fp_out, "ordinary\n");
				fflush(fp_out);
//				fprintf(stderr, "Send ordinary\n");
				serial_number[0]++;
				send_num++;
				output[log_times][0] = 0;	output[log_times][1] = 0;	output[log_times][2] = serial_number[0];	log_times++;
				break;
			case 1:
				kill(ppid, SIGUSR1); //送出signal
//				fprintf(stderr, "Send SIGUSR1\n");
				alarm(1); //設鬧鐘
				serial_number[1]++;
//				fprintf(stderr, "in 1, serial_number = %d\n", serial_number[1]);
				send_num++;
				output[log_times][0] = 0;	output[log_times][1] = 1;	output[log_times][2] = serial_number[1];	log_times++;
				break;
			case 2:
				kill(ppid, SIGUSR2);
//				fprintf(stderr, "Send SIGUSR2\n");
				serial_number[2]++;
//				fprintf(stderr, "in 2, serial_number = %d\n", serial_number[2]);
				send_num++;
				output[log_times][0] = 0;	output[log_times][1] = 2;	output[log_times][2] = serial_number[2];	log_times++;
				break;
		}
		
	}
	
	close(1);
	
	write_log();
	
	return 0;
	
}