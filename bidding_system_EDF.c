#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <string.h>

#define SIGUSR3 SIGWINCH

int serial_number[4] = {0};
int output[10000][3] = {{0}};
int out_times = 0;
FILE *fp_cus, *fp_log;
int fd_cus[2];
int fd_log;
pid_t pid;
int r_start = 0, r_end = 0;
int finish[10000] = {0};

/*算時間用*/
struct tms tmsstart, tmsend;
clock_t start = 0, end = 0;
static long clktck = 0;

typedef struct order {
	int code;
	double dead_line;
	int remain;
} Order;

Order handle[10000];
int handle_times = 0;

void make_nonblocking(int fd) {
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	
	flags |= O_NONBLOCK;
	
	fcntl(fd, F_SETFL, flags);
}

void run() {
	
	if(r_end == 0) {
		return;
	}
	
	/*
	for(int i =  0; i < r_end ; i++) {
		fprintf(stderr, "Up Up Up\n");
		fprintf(stderr, "i = %d\n", i);
		fprintf(stderr, "It's code = %d\n", handle[i].code);
		fprintf(stderr, "It's dead line = %lf\n", handle[i].dead_line);
		fprintf(stderr, "It's remain = %d\n", handle[i].remain);
	}*/
	
	struct timespec timespec;
	timespec.tv_sec = 0;
	timespec.tv_nsec = 10000000;
	int remain_now = handle[r_start].remain;
//	fprintf(stderr, "r_start[up] = %d\n", r_start);
//	fprintf(stderr, "remain_now = %d\n", remain_now);
//	fprintf(stderr, "now_code = %d\n", handle[r_start].code);
	int code_now = handle[r_start].code;
	
		if(handle[r_start].code == 1) {
			if(remain_now == 50) {
				serial_number[1]++;
				output[out_times][0] = 0;	output[out_times][1] = 1;	output[out_times][2] = serial_number[1];	out_times++;
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR1);
				output[out_times][0] = 1;	output[out_times][1] = 1;	output[out_times][2] = serial_number[1];	out_times++;
			}
			else {
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR1);
				output[out_times][0] = 1;	output[out_times][1] = 1;	output[out_times][2] = serial_number[1];	out_times++;
			}
		}
		else if(handle[r_start].code == 2) {
			if(remain_now == 100) {
				serial_number[2]++;
				output[out_times][0] = 0;	output[out_times][1] = 2;	output[out_times][2] = serial_number[2];	out_times++;
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR2);
				output[out_times][0] = 1;	output[out_times][1] = 2;	output[out_times][2] = serial_number[2];	out_times++;
			}
			else {
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR2);
				output[out_times][0] = 1;	output[out_times][1] = 2;	output[out_times][2] = serial_number[2];	out_times++;
			}
		}
		else if(handle[r_start].code == 3) {
			if(handle[r_start].remain == 20) {
				serial_number[3]++;
				output[out_times][0] = 0;	output[out_times][1] = 3;	output[out_times][2] = serial_number[3];	out_times++;
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR3);
				output[out_times][0] = 1;	output[out_times][1] = 3;	output[out_times][2] = serial_number[3];	out_times++;
			}
			else {
				for(int j = 0 ; j < remain_now ; j++) {
					nanosleep(&timespec, NULL);
					if(code_now != handle[r_start].code) {
						return;
					}
					handle[r_start].remain = remain_now-j;
				}
				kill(pid, SIGUSR3);
				output[out_times][0] = 1;	output[out_times][1] = 3;	output[out_times][2] = serial_number[3];	out_times++;
			}
		}
//		fprintf(stderr, "Outside!\n");
		finish[r_start] = 1;
		r_start++;
		handle_times--;
	
	return;
}

int compare(const void *a, const void *b) {
	double deadline1 = ((Order*) a) -> dead_line; 
	double deadline2 = ((Order*) b) -> dead_line;
	
	return (deadline1 > deadline2) ? 1 : -1;
}

void clean(int signo) {
	double deadline;
	end = times(&tmsend);
	
	if(signo == SIGUSR1) {
		handle[r_end].dead_line = 2 + (end - start)/(double)clktck;
//		fprintf(stderr, "dead line = %lf, code = 1\n", handle[r_end].dead_line);
		handle[r_end].code = 1;
		handle[r_end].remain = 50;
		handle_times++;
		r_end++;
	}
	else if(signo == SIGUSR2) {
		handle[r_end].dead_line = 3 + (end - start)/(double)clktck;
//		fprintf(stderr, "dead line = %lf, code = 2\n", handle[r_end].dead_line);
		handle[r_end].code = 2;
		handle[r_end].remain = 100;	
		handle_times++;
		r_end++;
	}
	else if(signo == SIGUSR3) {
		handle[r_end].dead_line = 0.3 + (end - start)/(double)clktck;
//		fprintf(stderr, "dead line = %lf, code = 3\n", handle[r_end].dead_line);
		handle[r_end].code = 3;
		handle[r_end].remain = 20;
		handle_times++;
		r_end++;
	}
	
	qsort(&handle[r_start], r_end - r_start, sizeof(Order), compare);
	
	/*
	for(int i =  0; i < r_end ; i++) {
		fprintf(stderr, "Down Down Down\n");
		fprintf(stderr, "i = %d\n", i);
		fprintf(stderr, "It's code = %d\n", handle[i].code);
		fprintf(stderr, "It's dead line = %lf\n", handle[i].dead_line);
		fprintf(stderr, "It's remain = %d\n", handle[i].remain);
	}*/
	
//	fprintf(stderr, "SigNo = %d\n", signo);
//	fprintf(stderr, "r_start = %d\n", r_start);
//	fprintf(stderr, "r_end = %d\n", r_end);
	
	return;
}

void write_log() {
	
	for(int i = 0 ; i < out_times ; i++) {
		if(output[i][0] == 0) {
			switch(output[i][1]) {
				case 1:
					fprintf(fp_log, "receive 0 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 2:
					fprintf(fp_log, "receive 1 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 3:
					fprintf(fp_log, "receive 2 %d\n", output[i][2]);
					fflush(fp_log);
					break;
			}
		}
		else if(output[i][0] == 1) {
			switch(output[i][1]) {
				case 1:
					fprintf(fp_log, "finish 0 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 2:
					fprintf(fp_log, "finish 1 %d\n", output[i][2]);
					fflush(fp_log);
					break;
				case 3:
					fprintf(fp_log, "finish 2 %d\n", output[i][2]);
					fflush(fp_log);
					break;
			}
		}
	}
	return;
}

int main(int argc, char *argv[]) {
	
	clktck = sysconf(_SC_CLK_TCK);
	start = times(&tmsstart); //開始計時
	
	struct timespec timespec;
	timespec.tv_sec = 0;
	timespec.tv_nsec = 10000000;
	
	pipe(fd_cus);
	fp_cus = fdopen(fd_cus[0], "r");
	char buf_from[100];
	fd_log = open("bidding_system_log", O_WRONLY | O_CREAT | O_APPEND, 0700);
	fp_log = fdopen(fd_log, "w");
	
	/*註冊signal*/
	struct sigaction act;
	
	act.sa_handler = clean;
	
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);
	sigaction(SIGUSR2, &act, NULL);
	sigaction(SIGUSR3, &act, NULL);
	
	if((pid= fork()) == 0) {
		close(fd_cus[0]);
		dup2(fd_cus[1], 1);
		execl("customer_EDF", "customer_EDF", argv[1], NULL);
	}
	else {
		fgets(buf_from, 11, fp_cus);
		fflush(fp_cus);
		
		while(strcmp(buf_from, "terminate\n") != 0) {
			while(handle_times == 0) {
				nanosleep(&timespec, NULL);
			}
			
			while(handle_times > 0) {
				run();
			}
			
			fgets(buf_from, 11, fp_cus);
			fflush(fp_cus);
		}
		
		write_log();
		
		fprintf(fp_log, "terminate\n");
		fflush(fp_log);
	}
	
	wait(NULL);
	
	return 0;
}