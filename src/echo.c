/******************************************************************************
 *
 * Copyright (C) 2016 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/sys.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lwipopts.h"

#define THREAD_STACKSIZE 1024
#define REMOTE_IP "192.168.1.15"
extern int start = 0;
extern int set = 0;
u16_t echo_port = 1900;


struct header
{
	uint8_t NetworkControl;
	uint8_t DataControl;
	uint8_t Size;
	uint32_t Data;
};

/*********************************************************************************
 *  Queue Handles:
 *
 *  xQueue1 -> 	Transfer image pixel data from
 *  			network input thread to application
 *  			thread.
 *
 *  xQueue2 ->	Communicate control packets.
 *  			Tell application thread what size
 *  			image to expect.
 **********************************************************************************/
QueueHandle_t xQueue1;
QueueHandle_t xQueue2;

void print_echo_app_header() {
	xil_printf("%20s %6d %s\r\n", "echo server", echo_port,
			"$ telnet <board_ip> 1900");

}

/* thread spawned for each connection */

void process_echo_request(void *p) {
	int sd = (int) p;
	int RECV_BUF_SIZE = 1024; //2048
	char recv_buf[RECV_BUF_SIZE];
	int n, nwrote;

	while (1) {
		/* read a max of RECV_BUF_SIZE bytes from socket */
		if ((n = read(sd, recv_buf, RECV_BUF_SIZE)) < 0) {
			xil_printf("%s: error reading from socket %d, closing socket\r\n",
					__FUNCTION__, sd);
			break;
		}

		/* break if the recved message = "quit" */
		if (!strncmp(recv_buf, "quit", 4))
			break;

		/* break if client closed connection */
		if (n <= 0)
			break;

		/* handle request */
		if ((nwrote = write(sd, recv_buf, n)) < 0) {
			xil_printf(
					"%s: ERROR responding to client echo request. received = %d, written = %d\r\n",
					__FUNCTION__, n, nwrote);
			xil_printf("Closing socket %d\r\n", sd);
			break;
		}
	}

	/* close connection */
	close(sd);
	vTaskDelete(NULL);
}

void echo_application_thread() {
	int sock;
	struct sockaddr_in Remote;
	struct sockaddr_in Local;
	int image_length = 0;


	struct header packet;
	if ((sock = lwip_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Socket Create Failed...\n");
		return;
	}

	// Set up Local Connection
	Local.sin_family = AF_INET;
	Local.sin_port = htons(echo_port);
	Local.sin_addr.s_addr = INADDR_ANY;

	if (lwip_bind(sock, (struct sockaddr *) &Local, sizeof(Local)) < 0) {
		printf("Bind Failed...\n");
		return;
	}

	// Set up Remote Connection
	Remote.sin_family = AF_INET;
	Remote.sin_port = htons(echo_port);
	Remote.sin_addr.s_addr = inet_addr(REMOTE_IP);

	xQueue1 = xQueueCreate(128, sizeof(packet.Data));
	xQueue2 = xQueueCreate(2, sizeof(packet.Data));

	while (1) {

		if (recvfrom(sock, (void *)&packet, sizeof(packet), 0,(struct sockaddr * )&Local, sizeof(Local)) > 0) {


			// Network Setup Packet
			if(packet.NetworkControl == 0xFF)
			{
				//printf("----------Network Control Packet----------\n");
				image_length = packet.Size;

				if(image_length != 0)
				{
					printf("Image Size %d\n", packet.Size);
					xQueueSendToBack(xQueue2, &packet.Size, 0);
					xQueueSendToBack(xQueue1, &packet.Data, 0);

				}
				/*****************************************************
				 *  Control Packet: Start Network
				 ****************************************************/
				if (packet.DataControl == 0xFF) {
					printf("----------Network Start Packet----------\n");
					start = 1;
					set = 1;
				}
			}
			// Network Data Packet
			if(packet.NetworkControl == 0x01)
			{
			//	printf("----------Network Data Packet----------\n");
				// Data is a training image
				if (packet.DataControl == 0x01)
				{
					//printf("----------Network Training Packet----------\n");
					xQueueSendToBack(xQueue1, &packet.Data, 0);
				}

				// Data is for real time analysis
				if (packet.DataControl == 0x02)
				{
				//	printf("----------Network Analysis Packet----------\n");
					xQueueSendToBack(xQueue1, packet.Data, 0);
				}
			}




/*
			if (sendto(sock, recv_buf, RECV_BUF_SIZE, 0, (struct sockaddr * )&Remote, sizeof(Remote)) > 0) {
				printf("\n    [+] Data Sent: %s\n", recv_buf);
			} else {
				printf('Error Sending Data\n');
			}
*/
		}

	}
}
void echo_application_thread1() {
	int sock, new_sd;
	struct sockaddr_in address, remote;
	int size;

	if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	address.sin_family = AF_INET;
	address.sin_port = htons(echo_port);
	address.sin_addr.s_addr = INADDR_ANY;

	if (lwip_bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
		return;

	lwip_listen(sock, 0);

	size = sizeof(remote);

	while (1) {

		if ((new_sd = lwip_accept(sock, (struct sockaddr *) &remote,
				(socklen_t *) &size)) > 0) {
			sys_thread_new("echos", process_echo_request, (void*) new_sd,
					THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
		}
	}
}

void print_app_header() {
	xil_printf("\n\r\n\r-----lwIP UDP echo server ------\n\r");
	xil_printf("UDP packets sent to port 7 will be echoed back\n\r");
}

