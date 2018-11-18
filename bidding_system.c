#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <fcntl.h>

pid_t pid;
int serial_number[3] = {0}; // 0:normal   1:member   2:VIP
int fd_log;
FILE *fp, *fp_from;
char to_log_zero[25], to_log_one[25], to_log_two[25];
int output[10000][3] = {{0}};
static int out_times = 0;

void make_nonblocking(int fd) {
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	
	flags |= O_NONBLOCK;
	
	fcntl(fd, F_SETFL, flags);
}

static void usr_handler_one(int signo) {
	
	struct timespec test;
	
	
	serial_number[1]++;
	/*receive SIGUSR1 寫進log*/
	output[out_times][0] = 0; output[out_times][1] = 1; output[out_times][2] = serial_number[1]; out_times++;
//	sprintf(to_log_one, "receive 1 %d\n", serial_number[1]);
//	fputs(to_log_one, fp);
//	fflush(fp);
	
	/*nanosleep 0.5秒*/
	test.tv_sec = 0;
	test.tv_nsec = 10000000;
	
	for(long long i = 1 ; i <= 50 ; i++) {
		nanosleep(&test, NULL);
	}
	
	/*發送signo給customer*/
	kill(pid, SIGUSR1);
	
	/*finish SIGUSR1 寫進log*/
	output[out_times][0] = 1; output[out_times][1] = 1; output[out_times][2] = serial_number[1]; out_times++;
//	sprintf(to_log_one, "finish 1 %d\n", serial_number[1]);
//	fputs(to_log_one, fp);
//	fflush(fp);
	
	return;
}

static void usr_handler_two(int signo) {
	struct timespec test;
	
	/*debug
	struct tms tmsstart, tmsend;
	clock_t start, end;
	static long clktck = 0;
	clktck = sysconf(_SC_CLK_TCK);*/
	
	
	serial_number[2]++;
	/*receive SIGUSR2 寫進log*/
	output[out_times][0] = 0; output[out_times][1] = 2; output[out_times][2] = serial_number[2]; out_times++;
//	sprintf(to_log_two, "receive 2 %d\n", serial_number[2]);		

//	start = times(&tmsstart);
	
//	fputs(to_log_two, fp);
//	fflush(fp);
//	write(fd_log, to_log_two, sizeof(to_log_two));
	
//	end = times(&tmsend);
//	fprintf(stderr, "time slice[2] = %7.2f\n", (end - start)/(double)clktck);

	/*nanosleep 0.2秒*/
	test.tv_sec = 0;
	test.tv_nsec = 200000000;
	nanosleep(&test, NULL);

	/*發送signo給customer*/
	kill(pid, SIGUSR2);
	
	/*finish SIGUSR2 寫進log*/
	output[out_times][0] = 1; output[out_times][1] = 2; output[out_times][2] = serial_number[2]; out_times++;
//	sprintf(to_log_two, "finish 2 %d\n", serial_number[2]);
//	fputs(to_log_two, fp);
//	fflush(fp);
	
	return;
}

void write_log() {
	
	for(int i = 0 ; i < out_times ; i++) {
		if(output[i][0] == 0) {
			switch(output[i][1]) {
				case 0:
					fprintf(fp, "receive 0 %d\n", output[i][2]);
					fflush(fp);
					break;
				case 1:
					fprintf(fp, "receive 1 %d\n", output[i][2]);
					fflush(fp);
					break;
				case 2:
					fprintf(fp, "receive 2 %d\n", output[i][2]);
					fflush(fp);
					break;
			}
		}
		else if(output[i][0] == 1) {
			switch(output[i][1]) {
				case 0:
					fprintf(fp, "finish 0 %d\n", output[i][2]);
					fflush(fp);
					break;
				case 1:
					fprintf(fp, "finish 1 %d\n", output[i][2]);
					fflush(fp);
					break;
				case 2:
					fprintf(fp, "finish 2 %d\n", output[i][2]);
					fflush(fp);
					break;
			}
		}
	}
	
	return;
}

int main(int argc, char *argv[]) {
	
	int from_cus[2];
	struct sigaction act_one, act_two;
	act_two.sa_handler = usr_handler_two;
	act_two.sa_flags = 0;

	sigemptyset(&act_two.sa_mask);
	sigaddset(&act_two.sa_mask, SIGUSR1);
	sigaddset(&act_two.sa_mask, SIGUSR2);	
	sigaction(SIGUSR2, &act_two, NULL);
	
	act_one.sa_handler = usr_handler_one;
	sigemptyset(&act_one.sa_mask);
	sigaddset(&act_one.sa_mask, SIGUSR1);
	sigaction(SIGUSR1, &act_one, NULL);
	
	pipe(from_cus);
	make_nonblocking(from_cus[0]);
	fd_log = open("bidding_system_log", O_WRONLY | O_CREAT | O_APPEND, 0700); // 開log檔
	fp = fdopen(fd_log, "w");
	
	
	long long j;
	int read_end;
	int wait_end;
//	int now_times = 0;
	
	while(1) {
		
		if((pid = fork()) == 0) { //customer
			close(from_cus[0]);
			dup2(from_cus[1], 1);
			execl("customer", "customer", argv[1], NULL);
		}
		
//		int read_end = read(from_cus[0], buf_from, 10);
		
		fp_from = fdopen(from_cus[0], "r");

		int i = 0;
		
			char buf_from[100];
			
			fgets(buf_from, 10, fp_from);
			fflush(fp_from);
			
			while(!feof(fp_from)) {
				wait_end = waitpid(pid, NULL, WNOHANG);
			
				if(wait_end != 0) {
					break;
				}
				
				if(strcmp(buf_from, "ordinary\n") == 0) {
					
					serial_number[0]++;
					/*寫receive資料進log*/
				
					output[out_times][0] = 0; output[out_times][1] = 0; output[out_times][2] = serial_number[0]; out_times++;
					
//					sprintf(to_log_zero, "receive 0 %d\n", serial_number[0]);
//					fputs(to_log_zero, fp);
//					fflush(fp);
					/*nanosleep 1 sec*/
					struct timespec test, rem;
					test.tv_sec = 0;
					test.tv_nsec = 10000000;
					for(j = 1 ; j <= 100 ; j++){
						nanosleep(&test, NULL);
					}
					kill(pid, SIGINT);
				
					/*寫finish資料進log*/
					output[out_times][0] = 1; output[out_times][1] = 0; output[out_times][2] = serial_number[0]; out_times++;
				}
				sprintf(buf_from, "No\n");
				fgets(buf_from, 10, fp_from);
				fflush(fp_from);
			}
			
		
		write_log();
//		fprintf(stderr, "I'm out!\n");
		sprintf(to_log_zero, "%s\n", "terminate");
		write(fd_log, to_log_zero, sizeof(to_log_zero));
		break;
	}
	
	fclose(fp);
	fclose(fp_from);
	if(wait_end == 0) {
		wait(NULL);
	}
	
	return 0;
	
}
