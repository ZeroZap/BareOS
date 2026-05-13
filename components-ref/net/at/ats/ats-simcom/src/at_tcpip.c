/**
  ******************************************************************************
  * @file    demo_tcpip.c
  * @author  SIMCom OpenSDK Team
  * @brief   Source file of tcpip stack operation.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 SIMCom Wireless.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

#include "simcom_os.h"
#include "simcom_tcpip.h"
#include "simcom_tcpip_old.h"
#include "scfw_socket.h"
#include "scfw_netdb.h"
#include "scfw_inet.h"
#include "simcom_common.h"
#include "simcom_debug.h"

#include "uvr.h"


#define SC_TCPIP_TEST_STACK_SIZE (1024*4)
#define SC_TCPIP_TEST_TASK_PRO 80
#define SC_MAX_STRING_LEN  64
#define SC_REMOTE_PORT_MIN               0
#define SC_REMOTE_PORT_MAX               65535



INT32 at_tcpip_pdp_id = 1;
int tcpip_server_localport= 0;
int tcpip_server_fd = 0;
struct SCipInfo local_ip_info = {TCPIP_PDP_INVALID, 0, {0}};
struct sockaddr g_local_addr;

static sMsgQRef gClientMsgQueue = NULL;
static at_msgq_t g_tcpip_client_msgq = NULL;
static sTaskRef tcpip_client_task_ref;

static sMsgQRef gServerMsgQueue = NULL;
static at_msgq_t g_tcpip_server_msgq = NULL;
static sTaskRef tcpip_server_task_ref;

static sMutexRef sockMutexRef = NULL;
int g_tcpip_client_sockfd = -1;


/**
  * @brief  Get the ip address of the module and convert it into a struct sockaddr structure
  * @param  type
  *            AF_INET： Get ipv4 address
  *            AF_INET6：Get ipv6 address
  *         local_addr struct to hold addresses
  * @note
  * @retval 0 is sucess,other is fail
  */
int at_tcpip_get_ipaddr(int type,struct sockaddr *local_addr)
{
	struct sockaddr_in ipv4sa;
	struct sockaddr_in6 ipv6sa;

    struct in_addr addr1;
    int isIpv6 = 0;
    int ret = -1;
    int result = -1;
    struct SCipInfo ipinfo;

    if(type == AF_INET6)
        isIpv6 = 1;

    ret = sAPI_TcpipGetSocketPdpAddr(1,1,&ipinfo);
    if(ret != SC_TCPIP_SUCCESS)
        goto exit;

    if(ipinfo.type == TCPIP_PDP_IPV4)
    {
        addr1.s_addr = ipinfo.ip4;
        at_log_d("ipinfo.type[%d]",ipinfo.type);

        if(isIpv6 == 1)
        {
            at_log_d("can't get ipv6 addr");
            goto exit;
        }

        at_log_d("\r\nPDPCTX type: IPV4,\r\nADDR: %s\r\n",inet_ntoa(addr1));
        memset(&ipv4sa,0,sizeof(ipv4sa));
        ipv4sa.sin_family = AF_INET;

        memcpy(&ipv4sa.sin_addr,&ipinfo.ip4,sizeof(ipv4sa.sin_addr));
        memcpy(local_addr,&ipv4sa,sizeof(ipv4sa));
    }
    else if(ipinfo.type == TCPIP_PDP_IPV6)
    {
        if(isIpv6 == 0)
        {
            at_log_d("can't get ipv4 addr");
            goto exit;
        }
        char dststr[100] = {0};
        inet_ntop(AF_INET6, ipinfo.ip6, dststr, sizeof(dststr));
        at_log_d("\r\nPDPCTX type: IPV6,\r\nADDR: [%s]\r\n", dststr);
        memset(&ipv6sa,0,sizeof(ipv6sa));
        ipv6sa.sin6_family = AF_INET6;

        memcpy(&ipv6sa.sin6_addr,&ipinfo.ip6,sizeof(ipv6sa.sin6_addr));
        memcpy(local_addr,&ipv6sa,sizeof(ipv6sa));

    }
    else if(TCPIP_PDP_IPV4V6 == ipinfo.type)
    {

        char dststr[100] = {0};
        inet_ntop(AF_INET6, ipinfo.ip6, dststr, sizeof(dststr));
        addr1.s_addr = ipinfo.ip4;
        at_log_d("\r\nPDPCTX type: IPV4V6,\r\nADDR: %s\r\nADDR: [%s]\r\n", inet_ntoa(addr1),dststr);
        if(isIpv6 == 1)
        {
            memset(&ipv6sa,0,sizeof(ipv6sa));
            ipv6sa.sin6_family = AF_INET6;
            memcpy(&ipv6sa.sin6_addr,&ipinfo.ip6,sizeof(ipv6sa.sin6_addr));
            memcpy(local_addr,&ipv6sa,sizeof(ipv6sa));
        }
        else
        {
            memset(&ipv4sa,0,sizeof(ipv4sa));
            ipv4sa.sin_family = AF_INET;
            memcpy(&ipv4sa.sin_addr,&ipinfo.ip4,sizeof(ipv4sa.sin_addr));
            memcpy(local_addr,&ipv4sa,sizeof(ipv4sa));
        }

    }

    result = 0;


exit:

    return result;
}

/**
  * @brief  Convert struct sockaddr to string
  * @param
  * @note
  * @retval 0 is sucess,other is fail
  */
static int at_tcpip_address_is_valid(const struct sockaddr *sa, socklen_t salen,
                  char *str, UINT16 *port)
{
    switch (sa->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in  *sin = (struct sockaddr_in *) sa;

            if (inet_ntop(AF_INET, &sin->sin_addr, str, 128) == NULL)
                return -1;

            *port = ntohs(sin->sin_port);
            return 0;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

            if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, 128) == NULL)
                return (-1);

            *port = ntohs(sin6->sin6_port);
            return 0;
        }
    }

    return -1;
}

/**
  * @brief  initiate a connection on a socket
  * @param
  * @note
  * @retval 0 is sucess,other is fail
  */
static int at_tcpip_connect(int* socketfd,int cid, const char *host, unsigned short port)
{
    #define PORT_MAX_LEN   6
    int ret = -1;
    int fd = -1;
    char portstr[PORT_MAX_LEN] = {0};
    struct addrinfo hints;
    struct addrinfo *addr_list = NULL, *rp = NULL;
    if (socketfd == NULL || host == NULL)
        return -1;

    at_log_d("host[%s] port[%d]",host,port);
    snprintf(portstr,sizeof(portstr),"%d",port);
    memset(&hints, 0x0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family   = AF_UNSPEC/*AF_INET*/;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, portstr, &hints, &addr_list) != 0)
    {
        at_log_d("getaddrinfo error");
        return -1;
    }

    for (rp = addr_list; rp != NULL; rp = rp->ai_next){
        if((fd = socket(rp->ai_family, rp->ai_socktype, 0)) < 0)
        {
            continue;
        }
        if((ret = connect(fd, rp->ai_addr, rp->ai_addrlen)) == 0)
        {
            *socketfd = fd;
            at_log_d("connect server sucess");
            break;
        }
        close(fd);
    }

    freeaddrinfo(addr_list);
    return ret;
}

/**
  * @brief  send a message on a socket
  * @param
  * @note
  * @retval
  */
static int at_tcpip_sendto(int fd, char *data,  int len, int flag, const struct sockaddr *destcAddr)
{
    int bytes = 0;
    int index = 0;

    while (len)
    {
        if (destcAddr == NULL)
        {
            bytes = send(fd, data + index, len, flag);
        }
        else
        {
            bytes = sendto(fd, data + index, len, flag, destcAddr, sizeof(struct sockaddr));
        }
        if (bytes < 0)
        {
            return -1;
        }
        else
        {
            len = len - bytes;
            index = index + bytes;
        }
    }

    return index;
}

int at_tcp_send(char *data, int len)
{
    int sockfd = 0;
    int ret=0;
    int send_bytes;
    int index;
    sockfd = at_tcpip_get_client_sockfd();

    if (sockfd < 0)
    {
        ret = -1;
        goto out;
    }

    while (len) {
        send_bytes = send(sockfd, data+index, len, 0);
        if (send_bytes <0) {
            return -2;
        } else {
            len -= send_bytes;
            index += send_bytes;
        }
    }
    ret = index;
out:
    return ret;
}

void at_tcpip_set_client_sockfd(int sockfd)
{
    sAPI_MutexLock(sockMutexRef,SC_SUSPEND);
    g_tcpip_client_sockfd = sockfd;
    sAPI_MutexUnLock(sockMutexRef);
}


int at_tcpip_get_client_sockfd(void)
{
    int sockfd = -1;
    sAPI_MutexLock(sockMutexRef,SC_SUSPEND);
    sockfd = g_tcpip_client_sockfd;
    sAPI_MutexUnLock(sockMutexRef);
    return sockfd;

}

/**
  * @brief  tcp or udp recv task
  * @param
  * @note
  * @retval
  */
static void at_tcpip_client_recv(int fd)
{
    fd_set master, read_fds;
    int fdmax = 0;
    int ret = -1;
    struct timeval tv;
    int socket_errno = 0;
    socklen_t addr_len;
    struct sockaddr addr;
    int i = 0;
    char ipstr[128] = {0};
    UINT16 port = 0;
    at_response_t response;
    if(fd < 0)
    {
        return;
    }
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(fd, &master);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    while(1)
    {
        read_fds = master;
        fdmax = fd + 1;
        ret = select(fdmax, &read_fds, NULL, NULL,&tv);
        if(ret > 0)
        {
            for(i = 0;i < fd + 1;i++)
            {
				if (!FD_ISSET(i, &read_fds))
				{
					continue;
				}
				if (i == fd)
				{
					memset(response.buf,0,sizeof(response.buf));
                    addr_len = sizeof(struct sockaddr);
                    memset(&addr,0,sizeof(addr));
					ret = recvfrom(fd,response.buf,sizeof(response.buf),0,&addr,&addr_len);
					if(ret > 0){
                        memset(ipstr,0,sizeof(ipstr));
                        if(at_tcpip_address_is_valid(&addr,addr_len,ipstr,&port) != 0)
                        {
                            at_log_i("\r\n converts  the  network  address fail\r\n");
                        }
                        at_log_i("recv ipstr[%s:%d] size[%d] [%s]",ipstr,port,ret,response.buf);
                        response.len = ret;
                        at_send_response(&response);
					}else{
						socket_errno = lwip_getsockerrno(fd);

						if(socket_errno != EAGAIN /*&& errno != EWOULDBLOCK*/)
						{
							at_log_i("recv fail errno[%d]",socket_errno);
							goto exit;
						}
					}
				}
			}
        }
        else if(ret == 0)
        {
            at_log_i("select timeout");
            continue;
        }
        else
        {
            at_log_i("select fail");
            goto exit;

        }
    }
exit:
    close(fd);
    at_tcpip_set_client_sockfd(-1);
    return;
}


static void at_tcpip_server_recv(int fd)
{
    while(1)
    {


    }

}


static void at_tcpip_client_process(void *arg)
{
    SIM_MSG_T msg ={0,0,0,NULL};
    SC_STATUS status;
    int sockfd = 0;
    at_log_i("start at_tcpip_client_process");
    while(1)
    {
        status = sAPI_MsgQRecv(gClientMsgQueue,&msg,SC_SUSPEND);
        at_log_i("receive msgq....");
        if(status == SC_SUCCESS)
        {
            sockfd = msg.arg1;
            at_tcpip_client_recv(sockfd);
        } else {
            at_log_e("sAPI_MsgQRecv failed!!!");
        }
    }

}


static void at_tcpip_server_process(void *arg)
{
   SIM_MSG_T msg ={0,0,0,NULL};
    SC_STATUS status;
    int sockfd = -1;
    while(1)
    {
        status = sAPI_MsgQRecv(gServerMsgQueue,&msg,SC_SUSPEND);
        if(status == SC_SUCCESS)
        {
            sockfd = msg.arg1;
            at_tcpip_server_recv(sockfd);
        }
    }
}


int at_cmd_tcpip_send(int sockfd, const char *remoteIP, unsigned short port, void *data, int date_len)
{
    int socket_errno = -1;
    char portstr[PORT_MAX_LEN] = {0};
    struct addrinfo hints;
    struct addrinfo *addr_list = NULL;

    int ret = -1;
    if(sockfd < 0){
        at_log_i("udp not't open [%d]",sockfd);
        return -1;
    }

    snprintf(portstr,sizeof(portstr),"%d",port);
    memset(&hints, 0x0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family   = AF_UNSPEC/*AF_INET*/;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(remoteIP, portstr, &hints, &addr_list) != 0)
    {
        at_log_i("getaddrinfo error");
        return -1;
    }

    ret = at_tcpip_send(sockfd,data,date_len,0,addr_list->ai_addr);
    freeaddrinfo(addr_list);
    if(ret == date_len)
    {
        at_log_i("send sucess");
    }
    else
    {
        socket_errno = lwip_getsockerrno(sockfd);
        at_log_i("send fail socket_errno[%d]",socket_errno);
        if(socket_errno != EAGAIN /*&& errno != EWOULDBLOCK*/)
        {
            ret = 0;
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}


void at_tcpip_task_init(void)
{
    static int inited = 0;
    if(inited == 0)
    {
        sAPI_MutexCreate(&sockMutexRef,SC_FIFO);

        sAPI_MsgQCreate(&gClientMsgQueue, "srvqueue", sizeof(SIM_MSG_T), 1, SC_FIFO);
        sAPI_TaskCreate(&tcpip_client_task_ref,NULL,4096,90,"client1",at_tcpip_client_process,NULL);

        sAPI_MsgQCreate(&gServerMsgQueue, "srvqueue", sizeof(SIM_MSG_T), 1, SC_FIFO);
        sAPI_TaskCreate(&tcpip_server_task_ref,NULL,4096,90,"server",at_tcpip_server_process,NULL);
        inited = 1;
    }

}


int at_tcpip_open(char *server_ipstr, uint16_t server_port)
{
    int ret = -1;
    char local_ip_str[128] = {0};
    uint16_t local_port = 0;
    int sockfd = -1;
    SIM_MSG_T msg;
    if (AT_STATUS_OK == sAPI_TcpipPdpActive(at_tcpip_pdp_id, 1)) {
        if (at_tcpip_get_ipaddr(AF_INET, &g_local_addr) == 0) {
            at_log_i("check address is valid");
            memset(local_ip_str, 0, sizeof(local_ip_str));
            if (at_tcpip_address_is_valid(&g_local_addr, sizeof(g_local_addr), local_ip_str,&local_port) == 0) {
                at_log_i("at tcpip pdp active ok");
            }
        }
    }

    if (at_tcpip_get_client_sockfd() >=0 ) {
        at_log_i("socket is busy");
        return -1;
    }

    at_log_i("start to connect");
    ret = at_tcpip_connect(&sockfd, at_tcpip_pdp_id,server_ipstr, server_port);

    if (ret == 0) {
        at_log_i("sockfd:%d, at_tcpip_connect [%s:%d] success", server_ipstr, server_port);
        at_tcpip_set_client_sockfd(sockfd);
    }

    at_sleep(100);
    memset(&msg,0,sizeof(msg));
    msg.arg1 = sockfd;
    ret = sAPI_MsgQSend(gClientMsgQueue,&msg);
    if(ret != SC_SUCCESS)
    {
        at_log_i("tcp recv thread busy status[%d]",ret);
        close(sockfd);
        at_tcpip_set_client_sockfd(-1);
        sockfd = -1;
    }

    return 0;
}