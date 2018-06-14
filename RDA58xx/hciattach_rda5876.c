/*******************************************************************************
 *
 *  Copyright (C) 2017-2018 xxxxx Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*****************************************************************************
**                                                                           
**  Name:          hciattach_rda5876.c
**
**  
******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>


#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTGETPROTO		_IOR('U', 201, int)
#define HCIUARTGETDEVICE	_IOR('U', 202, int)
#define HCIUARTSETFLAGS		_IOW('U', 203, int)
#define HCIUARTGETFLAGS		_IOR('U', 204, int)

#define HCI_UART_H4	0
#define HCI_UART_BCSP	1
#define HCI_UART_3WIRE	2
#define HCI_UART_H4DS	3
#define HCI_UART_LL	4
#define HCI_UART_ATH3K  5

#define HCI_UART_RAW_DEVICE	0


struct uart_t {
	char *type;
	int  m_id;
	int  p_id;
	int  proto;
	int  init_speed;
	int  speed;
	int  flags;
	int  pm;
	char *bdaddr;
	int  (*init) (int fd, struct uart_t *u, struct termios *ti);
	int  (*post) (int fd, struct uart_t *u, struct termios *ti);
};

#define FLOW_CTL	0x0001
#define ENABLE_PM	1
#define DISABLE_PM	0

static volatile sig_atomic_t __io_canceled = 0;
static int bcsp_max_retries = 10;/*unused*/

static void sig_hup(int sig)
{
}

static void sig_term(int sig)
{
	__io_canceled = 1;
}

static void sig_alarm(int sig)
{
	fprintf(stderr, "Initialization timed out.\n");
	exit(1);
}

#ifndef N_HCI
#define N_HCI	15
#endif

#ifdef ppoll
#undef ppoll
#endif
#define ppoll compat_ppoll

static inline int compat_ppoll(struct pollfd *fds, nfds_t nfds,
		const struct timespec *timeout, const sigset_t *sigmask)
{
	if (timeout == NULL)
		return poll(fds, nfds, -1);
	else if (timeout->tv_sec == 0)
		return poll(fds, nfds, 500);
	else
		return poll(fds, nfds, timeout->tv_sec * 1000);
}



static int uart_speed(int s)
{
	switch (s) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	default:
		return B57600;
	}
}

int set_speed(int fd, struct termios *ti, int speed)
{
	if (cfsetospeed(ti, uart_speed(speed)) < 0)
		return -errno;

	if (cfsetispeed(ti, uart_speed(speed)) < 0)
		return -errno;

	if (tcsetattr(fd, TCSANOW, ti) < 0)
		return -errno;

	return 0;
}

#if 0
/*
 * Read an HCI event from the given file descriptor.
 */
int read_hci_event(int fd, unsigned char* buf, int size)
{
	int remain, r;
	int count = 0;

	if (size <= 0)
		return -1;

	/* The first byte identifies the packet type. For HCI event packets, it
	 * should be 0x04, so we read until we get to the 0x04. */
	while (1) {
		r = read(fd, buf, 1);
		if (r <= 0)
			return -1;
		if (buf[0] == 0x04)
			break;
	}
	count++;

	/* The next two bytes are the event code and parameter total length. */
	while (count < 3) {
		r = read(fd, buf + count, 3 - count);
		if (r <= 0)
			return -1;
		count += r;
	}

	/* Now we read the parameters. */
	if (buf[2] < (size - 3))
		remain = buf[2];
	else
		remain = size - 3;

	while ((count - 3) < remain) {
		r = read(fd, buf + count, remain - (count - 3));
		if (r <= 0)
			return -1;
		count += r;
	}

	return count;
}

static int read_check(int fd, void *buf, int count)
{
	int res;

	do {
		res = read(fd, buf, count);
		if (res != -1) {
			buf += res;
			count -= res;
		}
	} while (count && (errno == 0 || errno == EINTR));

	if (count)
		return -1;

	return 0;
}

#endif

//Add by RDA android  ,rda initialization.
//setup uart flow control, if your uart hardware fifo less than 480 bytes.
static int rda_setup_flow_ctl(int fd, struct uart_t *u, struct termios *ti)
{
        unsigned int i, num_send;

        unsigned char rda_flow_ctl_10[][14] =
        {
                {0x01,0x02,0xfd,0x0a,0x00,0x01,0x44,0x00,0x20,0x40,0x3c,0x00,0x00,0x00},
                {0x01,0x02,0xfd,0x0a,0x00,0x01,0x10,0x00,0x00,0x50,0x22,0x01,0x00,0x00},// flow control
        };

        if (u->flags & FLOW_CTL) {
                /*Setup flow control */
                for (i = 0; i < sizeof(rda_flow_ctl_10)/sizeof(rda_flow_ctl_10[0]); i++) {
                        num_send = write(fd, rda_flow_ctl_10[i], sizeof(rda_flow_ctl_10[i]));
                        if (num_send != sizeof(rda_flow_ctl_10[i])) {
                                perror("");
                                printf("num_send = %d (%d)\n", num_send, sizeof(rda_flow_ctl_10[i]));
                                return -1;
                        }
                        usleep(5000);
                }
        }

        usleep(50000);

        return 0;
}

extern int RDABT_core_Intialization(int fd);
static int rda_init(int fd, struct uart_t *u, struct termios *ti)
{
        unsigned int i, num_send;
        unsigned char rda_baud_rate_10[][14] =
        {
                {0x01,0x02,0xfd,0x0a,0x00,0x01,0x60,0x00,0x00,0x80,0x00,0xc2,0x01,0x00},// 115200
                {0x01,0x02,0xFD,0x0A,0x00,0x01,0x40,0x00,0x00,0x80,0x00,0x01,0x00,0x00},//PSKEY: modify flag
        };

        RDABT_core_Intialization(fd);


#if 1
        if (u->speed != 115200) {
                //{0x01,0x02,0xfd,0x0a,0x00,0x01,0x60,0x00,0x00,0x80,0x80,0x25,0x00,0x00},// 9600
                rda_baud_rate_10[0][10] = (unsigned char)u->speed;
                rda_baud_rate_10[0][11] = (unsigned char)(u->speed >> 8);
                rda_baud_rate_10[0][12] = (unsigned char)(u->speed >> 16);
                rda_baud_rate_10[0][13] = (unsigned char)(u->speed >> 24);
                /* Modify Baud Rate */
                for (i = 0; i < sizeof(rda_baud_rate_10)/sizeof(rda_baud_rate_10[0]); i++) {
                        num_send = write(fd, rda_baud_rate_10[i], sizeof(rda_baud_rate_10[i]));
                        if (num_send != sizeof(rda_baud_rate_10[i])) {
                                perror("");
                                printf("num_send = %d (%d)\n", num_send, sizeof(rda_baud_rate_10[i]));
                                return -1;
                        }
                        usleep(3000);
                }
        }
#endif

        return 0;
}
//End by RDA android

struct uart_t uart[] = {
	{ "rda",	0x0000, 0x0000, HCI_UART_H4,   115200, 921600, FLOW_CTL, DISABLE_PM, NULL, rda_init },
	{ NULL, 0 }
};

static struct uart_t * get_by_id(int m_id, int p_id)
{
	int i;
	for (i = 0; uart[i].type; i++) {
		if (uart[i].m_id == m_id && uart[i].p_id == p_id)
			return &uart[i];
	}
	return NULL;
}

static struct uart_t * get_by_type(char *type)
{
	int i;
	for (i = 0; uart[i].type; i++) {
		if (!strcmp(uart[i].type, type))
			return &uart[i];
	}
	return NULL;
}

/* Initialize UART driver */
static int init_uart(char *dev, struct uart_t *u, int send_break, int raw)
{
	struct termios ti;
	int fd, i;
	unsigned long flags = 0;

	if (raw)
		flags |= 1 << HCI_UART_RAW_DEVICE;

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("Can't open serial port");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	if (u->flags & FLOW_CTL)
		ti.c_cflag |= CRTSCTS;
	else
		ti.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Can't set port settings");
		return -1;
	}

	/* Set initial baudrate */
	if (set_speed(fd, &ti, u->init_speed) < 0) {
		perror("Can't set initial baud rate");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (send_break) {
		tcsendbreak(fd, 0);
		usleep(500000);
	}

	if (u->init && u->init(fd, u, &ti) < 0)
		return -1;

	tcflush(fd, TCIOFLUSH);

	/* Set actual baudrate */
	if (set_speed(fd, &ti, u->speed) < 0) {
		perror("Can't set baud rate");
		return -1;
	}

// Add by RDA android, if you open uart FLOW_CTL, let it TRUE, FALSE default.
        if (u->flags & FLOW_CTL) {
                ti.c_cflag |= CRTSCTS;
                rda_setup_flow_ctl(fd, u, &ti);
        }
//End by RDA android

	/* Set TTY to N_HCI line discipline */
	i = N_HCI;
	if (ioctl(fd, TIOCSETD, &i) < 0) {
		perror("Can't set line discipline");
		return -1;
	}

	if (flags && ioctl(fd, HCIUARTSETFLAGS, flags) < 0) {
		perror("Can't set UART flags");
		return -1;
	}

	if (ioctl(fd, HCIUARTSETPROTO, u->proto) < 0) {
		perror("Can't set device");
		return -1;
	}

	if (u->post && u->post(fd, u, &ti) < 0)
		return -1;

	return fd;
}

static void usage(void)
{
	printf("hciattach - HCI UART driver initialization utility\n");
	printf("Usage:\n");
	printf("\thciattach [-n] [-p] [-b] [-r] [-t timeout] [-s initial_speed] <tty> <type | id> [speed] [flow|noflow] [bdaddr]\n");
	printf("\thciattach -l\n");
}

int main(int argc, char * argv[]){
	struct uart_t *u = NULL;
	int detach, printpid, raw, opt, i, n, ld, err;
	int to = 10;
	int init_speed = 0;
	int send_break = 0;
	pid_t pid;
	struct sigaction sa;
	struct pollfd p;
	sigset_t sigs;
	char dev[PATH_MAX];

	detach = 1;
	printpid = 0;
	raw = 0;

	while ((opt=getopt(argc, argv, "bnpt:s:lr")) != EOF) {
		switch(opt) {
		case 'b':
			send_break = 1;
			break;

		case 'n':
			detach = 0;
			break;

		case 'p':
			printpid = 1;
			break;

		case 't':
			to = atoi(optarg);
			break;

		case 's':
			init_speed = atoi(optarg);
			break;

		case 'l':
			for (i = 0; uart[i].type; i++) {
				printf("%-10s0x%04x,0x%04x\n", uart[i].type,
							uart[i].m_id, uart[i].p_id);
			}
			exit(0);

		case 'r':
			raw = 1;
			break;

		default:
			usage();
			exit(1);
		}
	}

	n = argc - optind;
	if (n < 2) {
		usage();
		exit(1);
	}

	for (n = 0; optind < argc; n++, optind++) {
		char *opt;

		opt = argv[optind];

		switch(n) {
		case 0:
			dev[0] = 0;
			if (!strchr(opt, '/'))
				strcpy(dev, "/dev/");
			strcat(dev, opt);
			break;

		case 1:
			if (strchr(argv[optind], ',')) {
				int m_id, p_id;
				sscanf(argv[optind], "%x,%x", &m_id, &p_id);
				u = get_by_id(m_id, p_id);
			} else {
				u = get_by_type(opt);
			}

			if (!u) {
				fprintf(stderr, "Unknown device type or id\n");
				exit(1);
			}

			break;

		case 2:
			u->speed = atoi(argv[optind]);
			break;

		case 3:
			if (!strcmp("flow", argv[optind]))
				u->flags |=  FLOW_CTL;
			else
				u->flags &= ~FLOW_CTL;
			break;

		case 4:
			if (!strcmp("sleep", argv[optind]))
				u->pm = ENABLE_PM;
			else
				u->pm = DISABLE_PM;
			break;

		case 5:
			u->bdaddr = argv[optind];
			break;
		}
	}

	if (!u) {
		fprintf(stderr, "Unknown device type or id\n");
		exit(1);
	}

	/* If user specified a initial speed, use that instead of
	   the hardware's default */
	if (init_speed)
		u->init_speed = init_speed;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	/* 10 seconds should be enough for initialization */
	alarm(to);
	bcsp_max_retries = to;

	n = init_uart(dev, u, send_break, raw);
	if (n < 0) {
		perror("Can't initialize device");
		exit(1);
	}

	printf("Device setup complete\n");

	alarm(0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	sa.sa_handler = sig_hup;
	sigaction(SIGHUP, &sa, NULL);

	if (detach) {
		if ((pid = fork())) {
			if (printpid)
				printf("%d\n", pid);
			return 0;
		}

		for (i = 0; i < 20; i++)
			if (i != n)
				close(i);
	}

	p.fd = n;
	p.events = POLLERR | POLLHUP;

	sigfillset(&sigs);
	sigdelset(&sigs, SIGCHLD);
	sigdelset(&sigs, SIGPIPE);
	sigdelset(&sigs, SIGTERM);
	sigdelset(&sigs, SIGINT);
	sigdelset(&sigs, SIGHUP);

	while (!__io_canceled) {
		p.revents = 0;
		err = ppoll(&p, 1, NULL, &sigs);
		if (err < 0 && errno == EINTR)
			continue;
		if (err)
			break;
	}

	/* Restore TTY line discipline */
	ld = N_TTY;
	if (ioctl(n, TIOCSETD, &ld) < 0) {
		perror("Can't restore line discipline");
		exit(1);
	}

	return 0;
}
