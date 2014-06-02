
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")  /* WinSockʹ�õĿ⺯�� */

/* ���峣�� */
#define FTP_DEF_PORT        21     /* ���ӵ�ȱʡ�˿� */
#define HTTP_DEF_PORT       80     /* ���ӵ�ȱʡ�˿� */
#define UFTP_DEF_PORT     1025 /* ȱʡ�˿� */
#define BUF_SIZE      1024     /* �������Ĵ�С */
#define FILENAME_LEN   256     /* �ļ������� */

/**
web
*/
/* �����ļ����Ͷ�Ӧ�� Content-Type */
struct doc_type
{
    char *suffix; /* �ļ���׺ */
    char *type;   /* Content-Type */
};

struct doc_type file_type[] =
{
    {"html",    "text/html"  },
    {"gif",     "image/gif"  },
    {"jpeg",    "image/jpeg" },
    { NULL,      NULL        }
};

char *http_res_hdr_tmpl = "HTTP/1.1 200 OK\r\nServer: Huiyong's Server <0.1>\r\n"
    "Accept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: close\r\n"
    "Content-Type: %s\r\n\r\n";


/**************************************************************************
 *
 * ��������: �����ļ���׺���Ҷ�Ӧ�� Content-Type.
 *
 * ����˵��: [IN] suffix, �ļ�����׺;
 *
 * �� �� ֵ: �ɹ������ļ���Ӧ�� Content-Type, ʧ�ܷ��� NULL.
 *
 **************************************************************************/
char *http_get_type_by_suffix(const char *suffix)
{
    struct doc_type *type;

    for (type = file_type; type->suffix; type++)
    {
        if (strcmp(type->suffix, suffix) == 0)
            return type->type;
    }

    return NULL;
}

/**************************************************************************
 *
 * ��������: ����������, �õ��ļ��������׺. �����и�ʽ:
 *           [GET http://www.baidu.com:8080/index.html HTTP/1.1]
 *
 * ����˵��: [IN]  buf, �ַ���ָ������;
 *           [IN]  buflen, buf �ĳ���;
 *           [OUT] file_name, �ļ���;
 *           [OUT] suffix, �ļ�����׺;
 *
 * �� �� ֵ: void.
 *
 **************************************************************************/
void http_parse_request_cmd(char *buf, int buflen, char *file_name, char *suffix)
{
    int length = 0;
    char *begin, *end, *bias;

    /* ���� URL �Ŀ�ʼλ�� */
    begin = strchr(buf, ' ');//�����ַ���s���״γ����ַ�c��λ��
    begin += 1;

    /* ���� URL �Ľ���λ�� */
    end = strchr(begin, ' ');
    *end = 0;

    bias = strrchr(begin, '/');//��str���Ҳ࿪ʼ�����ַ�c�״γ��ֵ�λ��
    length = end - bias;

    /* �ҵ��ļ����Ŀ�ʼλ�� */
    if ((*bias == '/') || (*bias == '\\'))
    {
        bias++;
        length--;
    }

    /* �õ��ļ��� */
    if (length > 0)
    {
        memcpy(file_name, bias, length);
        file_name[length] = 0;

        begin = strchr(file_name, '.');
        if (begin)
            strcpy(suffix, begin + 1);
    }
}


/**************************************************************************
 *
 * ��������: ��ͻ��˷��� HTTP ��Ӧ.
 *
 * ����˵��: [IN]  buf, �ַ���ָ������;
 *           [IN]  buf_len, buf �ĳ���;
 *
 * �� �� ֵ: �ɹ����ط�0, ʧ�ܷ���0.
 *
 **************************************************************************/
int http_send_response(SOCKET soc, char *buf, int buf_len)
{
    int read_len, file_len, hdr_len, send_len;
    char *type;
    char read_buf[BUF_SIZE];
    char http_header[BUF_SIZE];
    char file_name[FILENAME_LEN] = "index.html", suffix[16] = "html";
    FILE *res_file;

    /* �õ��ļ����ͺ�׺ */
    http_parse_request_cmd(buf, buf_len, file_name, suffix);

    res_file = fopen(file_name, "rb+"); /* �ö����Ƹ�ʽ���ļ� */
    if (res_file == NULL)
    {
        printf("[Web] The file [%s] is not existed\n", file_name);
        return 0;
    }

    fseek(res_file, 0, SEEK_END);
    file_len = ftell(res_file);
    fseek(res_file, 0, SEEK_SET);

    type = http_get_type_by_suffix(suffix); /* �ļ���Ӧ�� Content-Type */
    if (type == NULL)
    {
        printf("[Web] There is not the related content type\n");
        return 0;
    }

    /* ���� HTTP �ײ��������� */
    hdr_len = sprintf(http_header, http_res_hdr_tmpl, file_len, type);
    send_len = send(soc, http_header, hdr_len, 0);

    if (send_len == SOCKET_ERROR)
    {
        fclose(res_file);
        printf("[Web] Fail to send, error = %d\n", WSAGetLastError());
        return 0;
    }

    do /* �����ļ�, HTTP ����Ϣ�� */
    {
        read_len = fread(read_buf, sizeof(char), BUF_SIZE, res_file);

        if (read_len > 0)
        {
            send_len = send(soc, read_buf, read_len, 0);
            file_len -= read_len;
        }
    } while ((read_len > 0) && (file_len > 0));

    fclose(res_file);

    return 1;
}


/**
FTP
*/
void ftp_get(SOCKET soc, char* filename){

    FILE *file;
    file = fopen(filename, "rb+");
    if(file == NULL){
        printf("[FTP] The file [%s] is not existed\n", filename);
        exit(1);
    }

    int file_len;
    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    int read_len;
    char read_buf[BUF_SIZE];
    do /* �����ļ��ļ�*/
    {
        read_len = fread(read_buf, sizeof(char), BUF_SIZE, file);

        if (read_len > 0)
        {
            send(soc, read_buf, read_len, 0);
            file_len -= read_len;
        }
    } while ((read_len > 0) && (file_len > 0));

    fclose(file);

}

void ftp_put(SOCKET soc, char *file_name){

    FILE *file_ftp;
    file_ftp = fopen(file_name, "w+");
    if(file_ftp == NULL){
        printf("[FTP] The file [%s] is not existed\n", file_name);
        exit(1);
    }

    int result = 0;
    char data_buf[BUF_SIZE];
    do /* ������Ӧ�����浽�ļ��� */
    {
        result = recv(soc, data_buf, BUF_SIZE, 0);
        if (result > 0)
        {
            fwrite(data_buf, sizeof(char), result, file_ftp);

            /* ����Ļ����� */
            data_buf[result] = 0;
            printf("%s", data_buf);
        }
    } while(result > 0);

    fclose(file_ftp);

}
/**

*/
int ftp_send_response(SOCKET soc, char *buf, int buf_len){

    buf[buf_len] = 0;
    printf("The Server received: '%s' cmd from client \n", buf);
    //get filename
    char file_name[FILENAME_LEN];
    strcpy(file_name, buf+4);
    //�ļ�����
    if (strncmp(buf,"get",3)==0)  ftp_get(soc, file_name);
    //�ļ��ϴ�
    else if (strncmp(buf,"put",3)==0)  ftp_put(soc, file_name);
    else printf("the commod is not found\n");
    return 0;
}

void uftp_get(SOCKET soc, char* filename, struct sockaddr_in * send_addr){

    FILE *file;
    file = fopen(filename, "rb+");
    if(file == NULL){
        printf("[UFTP] The file [%s] is not existed\n", filename);
        exit(1);
    }

    int file_len;
    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    int read_len;
    char read_buf[BUF_SIZE];
    do /* �����ļ��ļ�*/
    {
        read_len = fread(read_buf, sizeof(char), BUF_SIZE, file);

        if (read_len > 0)
        {
            sendto(soc, read_buf, read_len, 0,
               (struct sockaddr *)send_addr, sizeof(struct sockaddr));
            file_len -= read_len;
        }
    } while ((read_len > 0) && (file_len > 0));
    sendto(soc, read_buf, 0, 0,
               (struct sockaddr *)send_addr, sizeof(struct sockaddr));

    fclose(file);

}

void uftp_put(SOCKET soc, char *file_name, struct sockaddr_in * recv_addr){

    FILE *file_ftp;
    file_ftp = fopen(file_name, "w+");
    if(file_ftp == NULL){
        printf("[FTP] The file [%s] is not existed\n", file_name);
        exit(1);
    }

    int result = 0;

    char data_buf[BUF_SIZE]={0};
    int addr_len = sizeof(struct sockaddr);
 /*
    char file_len[BUFSIZ]={0};
    result = recvfrom(soc, data_buf, BUF_SIZE, 0,
                              (struct sockaddr *)recv_addr, &addr_len);
    sscanf(data_buf,"%s",file_len);
    printf("%s",file_len);*/

    do /* ������Ӧ�����浽�ļ��� */
    {
       // printf("1");
        result = recvfrom(soc, data_buf, BUF_SIZE, 0,
                              (struct sockaddr *)recv_addr, &addr_len);
        if(result == 0) break;

       // printf("3");
        if (result > 0)
        {
            fwrite(data_buf, sizeof(char), result, file_ftp);

            /* ����Ļ����� */
            data_buf[result] = 0;
            printf("%s", data_buf);
        }
       // printf("2");
    } while(result > 0);
    //printf("fsdfsdf\n");
    fclose(file_ftp);

}
/**

*/
int uftp_send_response(SOCKET soc, char *buf, int buf_len, struct sockaddr_in* send_addr){
    //get filename
    char file_name[FILENAME_LEN];
    strcpy(file_name, buf+4);
    //�ļ�����
    if (strncmp(buf,"get",3)==0)  uftp_get(soc, file_name, send_addr);
    //�ļ��ϴ�
    else if (strncmp(buf,"put",3)==0)  uftp_put(soc, file_name, send_addr);
    else printf("the commod is not found\n");
    return 0;
}


int main(int argc, char **argv){
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2,0), &wsa_data); /* ��ʼ�� WinSock ��Դ */
    //web
    SOCKET	web_srv_soc = 0;
    unsigned short web_port = HTTP_DEF_PORT;
    web_srv_soc = socket(AF_INET, SOCK_STREAM, 0); /* ���� socket */
    if (web_srv_soc == INVALID_SOCKET)
    {
        printf("[Web] socket() Fails, error = %d\n", WSAGetLastError());
        return -1;
    }
    struct sockaddr_in web_serv_addr;   /* ��������ַ  */
    web_serv_addr.sin_family = AF_INET;
    web_serv_addr.sin_port = htons(web_port);
    web_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int result;
    result = bind(web_srv_soc, (struct sockaddr *) &web_serv_addr, sizeof(web_serv_addr));
    if (result == SOCKET_ERROR) /* ��ʧ�� */
    {
        closesocket(web_srv_soc);
        printf("[Web Server] Fail to bind, error = %d\n", WSAGetLastError());
        return -1;
    }
    result = listen(web_srv_soc, SOMAXCONN);
    printf("[Web] The server is running ... ...\n");

    //FTP
    SOCKET	ftp_srv_soc = 0;
    unsigned short ftp_port = FTP_DEF_PORT;
    ftp_srv_soc = socket(AF_INET, SOCK_STREAM, 0); /* ���� socket */
    if (ftp_srv_soc == INVALID_SOCKET)
    {
        printf("[FTP] socket() Fails, error = %d\n", WSAGetLastError());
        return -1;
    }
    struct sockaddr_in ftp_serv_addr;   /* ��������ַ  */
    ftp_serv_addr.sin_family = AF_INET;
    ftp_serv_addr.sin_port = htons(ftp_port);
    ftp_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    result = bind(ftp_srv_soc, (struct sockaddr *) &ftp_serv_addr, sizeof(ftp_serv_addr));
    if (result == SOCKET_ERROR) /* ��ʧ�� */
    {
        closesocket(ftp_srv_soc);
        printf("[FTP Server] Fail to bind, error = %d\n", WSAGetLastError());
        return -1;
    }
    result = listen(ftp_srv_soc, SOMAXCONN);
    printf("[FTP] The server is running ... ...\n");

    //UDP FTP
    SOCKET  uconn_sock = 0; /* socket ��� */
    uconn_sock = socket(AF_INET, SOCK_DGRAM, 0); /* ���� socket */

    unsigned short port = UFTP_DEF_PORT;
    /* socket�ı��ص�ַ */
    struct sockaddr_in loc_addr;
    loc_addr.sin_family = AF_INET;
    loc_addr.sin_port = htons(port);
    loc_addr.sin_addr.s_addr = INADDR_ANY;

    bind(uconn_sock, (struct sockaddr *)&loc_addr, sizeof(struct sockaddr));
    printf("[UFTP] The server is running ... ...\n");


    fd_set afds;
    FD_ZERO(&afds);
    FD_SET(web_srv_soc, &afds);
    FD_SET(ftp_srv_soc, &afds);
    FD_SET(uconn_sock, &afds);

    fd_set rfds;
    SOCKET web_conn_soc;
    SOCKET ftp_conn_soc;
    while(1){
        FD_ZERO(&rfds);
        memcpy(&rfds, &afds, sizeof(rfds));
        if(select(FD_SETSIZE, &rfds, (fd_set*)0, (fd_set*)0,
                  (struct timeval *)0)== SOCKET_ERROR)
            printf("select error\n");
        if(FD_ISSET(web_srv_soc, &rfds)){
            struct sockaddr_in conn_addr;   /* �ͻ��˵�ַ  */
            int conn_addr_len = sizeof(conn_addr);
            web_conn_soc = accept(web_srv_soc, (struct sockaddr*)&conn_addr,&conn_addr_len);
            if(web_conn_soc == INVALID_SOCKET){
                printf("[Web] Fail to accept, error = %d\n", WSAGetLastError());
                break;
            }
            printf("[Web] Accepted address:[%s], port:[%d]\n",
            inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));
            char recv_buf[BUF_SIZE];
            int recv_len;
            recv_len = recv(web_conn_soc, recv_buf, BUF_SIZE, 0);
            if (recv_len == SOCKET_ERROR) /* ����ʧ�� */
            {
                closesocket(web_conn_soc);
                printf("[Web] Fail to recv, error = %d\n", WSAGetLastError());
                break;
            }

            recv_buf[recv_len] = 0;

            /* ��ͻ��˷�����Ӧ���� */
            http_send_response(web_conn_soc, recv_buf, recv_len);
            closesocket(web_conn_soc);
        }
        if(FD_ISSET(ftp_srv_soc, &rfds)){
            struct sockaddr_in conn_addr;   /* �ͻ��˵�ַ  */
            int conn_addr_len = sizeof(conn_addr);
            ftp_conn_soc = accept(ftp_srv_soc, (struct sockaddr*)&conn_addr,&conn_addr_len);
            if(ftp_conn_soc == INVALID_SOCKET){
                printf("[FTP] Fail to accept, error = %d\n", WSAGetLastError());
                break;
            }
            printf("[FTP] Accepted address:[%s], port:[%d]\n",
            inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));
            char recv_buf[BUF_SIZE];
            int recv_len;
            recv_len = recv(ftp_conn_soc, recv_buf, BUF_SIZE, 0);
            if (recv_len == SOCKET_ERROR) /* ����ʧ�� */
            {
                closesocket(ftp_conn_soc);
                printf("[FTP] Fail to recv, error = %d\n", WSAGetLastError());
                break;
            }

            recv_buf[recv_len] = 0;

            /* ��ͻ��˷�����Ӧ���� */
            ftp_send_response(ftp_conn_soc, recv_buf, recv_len);
            closesocket(ftp_conn_soc);
        }
        if(FD_ISSET(uconn_sock, &rfds)){
            char recv_buf[BUF_SIZE];
            int recv_len;
            struct sockaddr_in   t_addr; /* socket��Զ�˵�ַ */
            int addr_len = sizeof(struct sockaddr_in);
            recv_len = recvfrom(uconn_sock, recv_buf, BUF_SIZE, 0,
                              (struct sockaddr *)&t_addr, &addr_len);
            if (recv_len == SOCKET_ERROR)
            {
                printf("[UFTP Server] Recv error %d\r\n", WSAGetLastError());
                break;
            }
            recv_buf[recv_len] = 0;
            printf("[UFTP Server] Receive data: \"%s\", from %s\r\n",
            recv_buf, inet_ntoa(t_addr.sin_addr));
            uftp_send_response(uconn_sock, recv_buf, recv_len, &t_addr);
        }

    }

    closesocket(web_srv_soc);
    closesocket(ftp_srv_soc);
    closesocket(uconn_sock);
    WSACleanup();
    printf("[web] The server is stopped.\n");
    printf("[FTP] The server is stopped.\n");
    printf("[UFTP] The server is stopped.\n");
    return 0;
}
