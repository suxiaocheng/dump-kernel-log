#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <screen/screen.h>
#include <errno.h>
#include <string.h>
#include <sys/slogcodes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/procfs.h>

#define TAG		"dump-kernel-log"
#define MEM_DEV		"/dev/mem"

#define SLOG_DEBUG(x, ...)	\
	if (debug) {\
		slogf(_SLOGC_TEST, _SLOG_DEBUG1,  "[" TAG ":%s(%d)]DEBG: " x, __func__, __LINE__, ##__VA_ARGS__); \
	}
#define SLOG_INFO(x, ...)       slogf(_SLOGC_TEST, _SLOG_INFO,    "[" TAG ":%s(%d)]INFO: " x, __func__, __LINE__, ##__VA_ARGS__)
#define SLOG_WARNING(x, ...)    slogf(_SLOGC_TEST, _SLOG_WARNING, "[" TAG ":%s(%d)]WARN: " x, __func__, __LINE__, ##__VA_ARGS__)
#define SLOG_ERROR(x, ...)      \
	{ \
		slogf(_SLOGC_TEST, _SLOG_ERROR,   "[" TAG ":%s(%d)]ERR : " x, __func__, __LINE__, ##__VA_ARGS__); \
		fprintf(stderr, "[" TAG ":%s(%d)]ERR : " x "\n", __func__, __LINE__, ##__VA_ARGS__); \
	}\

typedef unsigned int uint32_t;
typedef unsigned int atomic_t;
typedef unsigned char uint8_t;

// copy data struct and macro form linux kernel (fs/pstore/ram_core.c:32)
struct persistent_ram_buffer {
	uint32_t    sig;
	atomic_t    start;
	atomic_t    size;
	uint8_t     data[0];
};

#define PERSISTENT_RAM_SIG (0x43474244) /* DBGC */

int debug = 0;
char *dmesg_file="/var/dmesg.txt";

int ascii2dec(char *ascii)
{
	int value = 0;
	int base = 10;
	int i = 0;

	if ((ascii[0] == '0') && ((ascii[1] = 'x')||(ascii[1] = 'X'))) {
		base = 16;
		i = 2;
	}

	for(; ascii[i] != 0; i++) {
		value = base * value;
		if ((ascii[i] >= '0') && (ascii[i] <= '9')) {
			value += ascii[i] - '0';
		} else if (base == 16) {
			if ((ascii[i] >= 'A') && (ascii[i] <= 'F')) {
				value += ascii[i] - 'A' + 10;
			} else if ((ascii[i] >= 'a') && (ascii[i] <= 'f')) {
				value += ascii[i] - 'a' + 10;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	return value;
}

int save_dmesg_buf(char *buf, int len)
{
	int ret;
	int fd;
	int oflag = O_WRONLY | O_CREAT;
	static int first_write = 1;

	SLOG_DEBUG("save buf, len: %d, first_write: %d", len, first_write);

	if (first_write) {
		oflag |= O_TRUNC;
		first_write = 0;
	} else {
		oflag |= O_APPEND;
	}

	fd = open(dmesg_file, oflag, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		SLOG_ERROR("Create dmesg file fail, errno: %d", errno);
		return -1;
	}

	if (debug) {
		char *header_tag = "save buf...\n";
		write(fd, header_tag, strlen(header_tag));
	}

	ret = write(fd, buf, len);
	if (ret == -1) {
		SLOG_ERROR("Write dmesg file fail");
		goto SAVE_DMESG_QUIT;
	}

SAVE_DMESG_QUIT:
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int address = 0x6bff0000;
	int size = 0x10000 - sizeof(struct persistent_ram_buffer);
	int c;
	char *buf = NULL;
	int fd;

	struct persistent_ram_buffer header;

	while (( c = getopt( argc, argv, "a:d:s:o:" ) ) != -1){
		switch(c) {
			case 'a':
				address = ascii2dec(optarg);
				SLOG_DEBUG("address is 0x%x", address);
				break;
			case 's':
				size = ascii2dec(optarg);
				SLOG_DEBUG("size is 0x%x", size);
				break;
			case 'd':
				debug = ascii2dec(optarg);
				SLOG_DEBUG("debug is %s", debug?"enable":"disable");
				break;
			case 'o':
				dmesg_file = optarg;
				SLOG_DEBUG("output dmesg file: %s", dmesg_file);
				break;
			default:
				SLOG_ERROR("Unknow input opt: %c", c);
				return -1;
		}
	}

	fd = open(MEM_DEV, O_RDONLY);
	if (fd == -1) {
		SLOG_ERROR("Open %s fail, errno: %d", MEM_DEV, errno);
		return -1;
	}

	ret = lseek(fd, address, SEEK_SET);
	if (ret == -1) {
		SLOG_ERROR("Seek to 0x%x fail", address);
		goto QUIT;
	}

	ret = read(fd, &header, sizeof(header));
	if (ret != sizeof(header)) {
		SLOG_ERROR("Read less header bytes (%d) than except (%ld)", ret, sizeof(header));
		goto QUIT;
	}

	if (header.sig != PERSISTENT_RAM_SIG) {
		SLOG_ERROR("Header mismatch, readout %x, except: %x", header.sig, PERSISTENT_RAM_SIG);
		goto QUIT;
	}

	buf = (char *)malloc(size);
	if (buf == NULL) {
		SLOG_ERROR("malloc memory len:0x%x fail", size);
		goto QUIT;
	}

	ret = read(fd, buf, size);
	if (ret != size) {
		SLOG_ERROR("Read data size(%d) not match we except(%u)", ret, size);
		goto QUIT;
	} else {
		SLOG_DEBUG("Found valid header, start to dump dmesg...");
		SLOG_DEBUG("\t start: 0x%x, size: 0x%x", header.start, header.size);
	}

	/* Write the memory to a dump file */
	if (header.size > 0) {
		/* init state, the buffer is not fulfill */
		if (header.size < size) {
			save_dmesg_buf(buf, header.size);
		} else {
			/* verify param is correct or not */
			if (header.size != size) {
				SLOG_ERROR("pstore data size error");
			}
			save_dmesg_buf(&buf[header.start], size - header.start);
			save_dmesg_buf(buf, header.start);
		}
	} else {
		SLOG_DEBUG("Empty log");
	}

QUIT:
	if(buf != NULL) {
		free(buf);
	}
	close(fd);

	return ret;
}

