#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include <limits.h>
#include <syscall.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/uio.h>

 #define KCOV_INIT_TRACE			_IOR('c', 1, unsigned long)
#define KCOV_ENABLE			_IO('c', 100)
#define KCOV_DISABLE			_IO('c', 101)
#define COVER_SIZE			(64<<10)

#define KCOV_TRACE_PC  0
#define KCOV_TRACE_CMP 1

struct poc_lkm_size {
	char dummy[128];
};

#define POC_LKM_IOCTL_MAGIC ('P')
#define POC_LKM_CMD_0 _IOWR(POC_LKM_IOCTL_MAGIC, 0, struct poc_lkm_size)
#define POC_LKM_CMD_1 _IOWR(POC_LKM_IOCTL_MAGIC, 1, struct poc_lkm_size)
#define POC_LKM_CMD_2 _IOWR(POC_LKM_IOCTL_MAGIC, 2, struct poc_lkm_size)

int poc_fd;
int kcov_fd;
unsigned long *cover;

void init_poc_lkm(void)
{
	poc_fd = open("/proc/poc_lkm", O_RDWR);
	if (poc_fd < 0) {
		printf("[-] open /proc/poc_lkm failed\n");
		exit(0);
	}
}
void close_poc_lkm(void)
{
	close(poc_fd);
}

void init_kcov(void)
{
	kcov_fd = open("/sys/kernel/debug/kcov", O_RDWR);
	if (kcov_fd == -1) {
		perror("open");
		exit(1);
	}

	/* Setup trace mode and trace size. */
	if (ioctl(kcov_fd, KCOV_INIT_TRACE, COVER_SIZE)) {
		perror("ioctl");
		exit(1);
	}

	/* Mmap buffer shared between kernel- and user-space. */
	cover = (unsigned long*)mmap(NULL, COVER_SIZE * sizeof(unsigned long), PROT_READ | PROT_WRITE, MAP_SHARED, kcov_fd, 0);
	if ((void*)cover == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
}
void close_kcov(void)
{
	/* Free resources. */
	if (munmap(cover, COVER_SIZE * sizeof(unsigned long))) {
		perror("munmap");
		exit(1);
	}
	if (close(kcov_fd)) {
		perror("close");
		exit(1);
	}
}

void start_kcov(void)
{
	/* Enable coverage collection on the current thread. */
	if (ioctl(kcov_fd, KCOV_ENABLE, KCOV_TRACE_PC)) {
		perror("ioctl");
		exit(1);
	}

	/* Reset coverage from the tail of the ioctl() call. */
	__atomic_store_n(&cover[0], 0, __ATOMIC_RELAXED);
}
void stop_kcov(void)
{
	unsigned long n;

	/* Read number of PCs collected. */
	n = __atomic_load_n(&cover[0], __ATOMIC_RELAXED);
	printf("Coverage (The number of PCs) : %ld\n", n);

	/* Disable coverage collection for the current thread. After this call
	 * coverage can be enabled for a different thread.
	 */
	if (ioctl(kcov_fd, KCOV_DISABLE, 0)) {
		perror("ioctl");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	init_poc_lkm();
	init_kcov();

	// coverage start
	start_kcov();
	ioctl(poc_fd, POC_LKM_CMD_0, 0x10);
	stop_kcov();

	start_kcov();
	ioctl(poc_fd, POC_LKM_CMD_1, 0x10);
	stop_kcov();

	start_kcov();
	ioctl(poc_fd, POC_LKM_CMD_2, 0x10);
	stop_kcov();
	// coverage end

	close_kcov();
	close_poc_lkm();
}
