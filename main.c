#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <linux/watchdog.h>

void register_sig_handler();
void sigint_handler(int sig);
void run_watchdog_loop(int fd);

int sighup_recvd;

int main(int argc, char **argv)
{
	int fd;
	int timeout, requested_timeout;
		int status, boot_status;
	char c;

	if (argc > 1) {
		requested_timeout = strtol(argv[1], NULL, 0);

		if (requested_timeout == 0 && errno != 0) {
			printf("Invalid timeout arg: %s\n", argv[1]);
			exit(1);
		}

		/* these limits are arbitrary */
		if (requested_timeout < 5 || requested_timeout > 300) {
			printf("Invalid timeout arg: %s\n", argv[1]);
			exit(1);
		}
	}
	else {
		requested_timeout = 0;
	}

	register_sig_handler();

	printf("pid: %d\n", getpid());

	fd = open("/dev/watchdog", O_RDWR);

	if (fd < 0) {
		perror("open");
		exit(1);
	}

	if (ioctl(fd, WDIOC_GETSTATUS, &status) >= 0)
		printf("status: 0x%08X\n", status);
	else
		perror("ioctl(WDIOC_GETSTATUS)");

	if (ioctl(fd, WDIOC_GETBOOTSTATUS, &boot_status) >= 0)
		printf("boot status: 0x%08X\n", boot_status);
	else
		perror("ioctl(WDIOC_GETBOOTSTATUS)");

	if (ioctl(fd, WDIOC_GETTIMEOUT, &timeout) < 0) {
		perror("ioctl(WDIOC_GETTIMEOUT)");
		exit(1);
	}

	printf("current timeout: %d\n", timeout);

	if (requested_timeout != 0 && requested_timeout != timeout) {
		printf("Setting requested timeout: %d\n", requested_timeout);

		if (ioctl(fd, WDIOC_SETTIMEOUT, &requested_timeout) < 0) {
			perror("ioctl(WDIOC_SETTIMEOUT)");
			exit(1);
		}
	}

	run_watchdog_loop(fd);

	/* close gracefully */
	c = 'V';

	if (write(fd, &c, 1) != 1)
		perror("write");

	close(fd);

	return 0;
}

void run_watchdog_loop(int fd)
{
	int timeleft;

	while (!sighup_recvd) {
		if (ioctl(fd, WDIOC_GETTIMELEFT, &timeleft) < 0) {
			perror("ioctl(WDIOC_GETTIMELEFT)");
			exit(1);
		}

		printf("timeleft: %d\n", timeleft);

		if (timeleft < 10) {
			if (ioctl(fd, WDIOC_KEEPALIVE, 0) < 0) {
				perror("ioctl(WDIOC_KEEPALIVE)");
				exit(1);
			}
		}

		sleep(5);
	}
}

void register_sig_handler()
{
	struct sigaction sia;

	bzero(&sia, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGHUP, &sia, NULL) < 0) {
		perror("sigaction(SIGHUP)");
		exit(1);
	}
}

void sigint_handler(int sig)
{
	sighup_recvd = 1;
}

