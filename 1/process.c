/*
 * Copyright (c) 2019 Sanggu Han
 */

#include "process.h"
#include "globals.h"

#define QUEUE_PERMS 0644
#define BUFF_SIZE 64

/**
 * input_process - get input from board, then pass key to main process
 */
void input_process()
{
	/* Variables for loop counter */
	int i, j;

	/* Variables for switch input (BACK, PROG, ...) */
	struct input_event ev[BUFF_SIZE];
	int fd, rd, value, flags, size = sizeof (struct input_event);
	char *device = "/dev/input/event0";

	/* Variables for switch input (SW1-9) */
	int swfd, swflag, pressed;
	unsigned char push_sw_buff[9];
	unsigned char pushed[9];
	int buff_size = sizeof(push_sw_buff);
	char *sw = "/dev/fpga_push_switch";

	/* Variables for message queue */
        key_t key;
        int msgqid;

        /* get message queue id for input_process <-> main_process */
        key = ftok(__FILE__, 'Z');
        msgqid = msgget(key, QUEUE_PERMS | IPC_CREAT);
        if (msgqid == -1) {
                perror("(I) Error occurred while get message queue");
        }

	if((fd = open (device, O_RDONLY)) == -1) {
		printf("%s is not a valid device\n", device);
	}
	/* for non-blocking read() */
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);


	if((swfd = open(sw, O_RDWR)) == -1) {
		printf("%s is not a valid device\n", device);
	}

	while (1){
		/* set pressed as release state */
		pressed = KEY_RELEASE;
		/* initialize switch state tracker */
		memset(pushed, 0, sizeof(pushed));

		do {
			swflag = 0;
			read(swfd, &push_sw_buff, buff_size);
			for (i = 0; i < 9; i++) {
				swflag |= push_sw_buff[i];
				pushed[i] |= push_sw_buff[i];
			}
			pressed = swflag ? KEY_PRESS : pressed;
			usleep(10000);
		} while (swflag); /* read input until switch is released */

		if (pressed) {
			enqueue_message(msgqid, (long)INPUT, "Switch");
		}

		if ((rd = read(fd, ev, size * BUFF_SIZE)) < size) {
			continue;
		}

		value = ev[0].value;

		if (value == KEY_RELEASE) {
			switch(ev[0].code) {
			case KEY_PROG:
				printf("PROG\n");
				break;
			case KEY_BACK:
				printf("BACK\n");
				break;
			case KEY_VOLUMEUP:
				printf("VOLUME UP\n");
				break;
			case KEY_VOLUMEDOWN:
				printf("VOLUME DOWN\n");
				break;
			default:
				break;
			}
			enqueue_message(msgqid, (long)INPUT, "Key");
		}
		usleep(10000);
	}

	close(fd);
	close(swfd);
}

int enqueue_message(int qid, long mtype, char *msg) {
        msg_t message;
        message.mtype = mtype;
        strcpy(message.msg, msg); 
        if (msgsnd(qid, &message, sizeof(msg_t)-sizeof(long), 0) == -1) {
                perror("Error occurred msgsnd()");
        }
	return 0;
};

/**
 * output_process - get results from main process, then print to board
 */
void output_process() {
        key_t key;
        int msgqid;

        /* get message queue id for output_process <-> main_process */
        key = ftok(__FILE__, 'Z');
        msgqid = msgget(key, QUEUE_PERMS | IPC_CREAT);
        if (msgqid == -1) {
                perror("(O) Error occurred while get message queue");
        }

        do {
                msg_t message;
                if (msgrcv(msgqid, &message, sizeof(msg_t)-sizeof(long), OUTPUT, 0) == -1) {
                        perror("(O) Error occurred msgrcv()");
                }
                printf("(O) got %s\n", message.msg);

                /* TODO: print result properly in here */
        } while(1);
};
