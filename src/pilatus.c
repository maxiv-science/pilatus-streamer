#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <zmq.h>
#include "tiff.h"

#define BUFFER_SIZE 1024
#define EVENT_SIZE sizeof(struct inotify_event)
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct
{
    char last_file[256];
    char recent_file[256];
    void* context;
    void* push_socket;
    void* monitor_socket;
    int scan_numer;
} Pilatus;

void pilatus_init(Pilatus* pilatus)
{
    pilatus->last_file[0] = '\0';
    pilatus->scan_numer = 0;
    pilatus->context = zmq_ctx_new();
    pilatus->push_socket = zmq_socket(pilatus->context, ZMQ_PUSH);
    int rc = zmq_bind(pilatus->push_socket, "tcp://*:9999");
    if (rc != 0) {
        printf("zmq_bind for push socket failed\n");
    }
    pilatus->monitor_socket = zmq_socket(pilatus->context, ZMQ_REP);
    rc = zmq_bind(pilatus->monitor_socket, "tcp://*:9998");
    if (rc != 0) {
        printf("zmq_bind for monitor socket failed\n");
    }
}

int connect_camserver()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create camserver socket");
        return -1;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(41234);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Error connecting to camserver socket\n");
        return -1;
    }
    return sock;
}

int start_server()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create server socket");
        return -1;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);
    if( bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
         printf("Error binding to server socket: %s\n", strerror(errno));
    }
    listen(sock, 3);
    return sock;
}

int check_monitor(void* monitor_socket)
{
    zmq_pollitem_t items[] = {
        {monitor_socket, 0, ZMQ_POLLIN, 0}
    };
    zmq_poll(items, 1, 0);
    if (items[0].revents & ZMQ_POLLIN) {
        char msg [256];
        zmq_recv(monitor_socket, msg, 255, 0);
        printf("monitor msg: %s\n", msg);
        return 1;
    }
    return 0;
}

int get_frame_number(const char* filename)
{
    int frame_number;
    char* tmp = strrchr(filename, '_');
    // if nimages=1 no frame number is appended to filename
    if (tmp == NULL) {
        frame_number = 0;
    }
    else {
        sscanf(tmp, "_%05d.tif", &frame_number);
    }
    return frame_number;
}

void handle_request(char buffer[], int nb, int camserver_sock, Pilatus* pilatus)
{
    printf("Request: %s\n", buffer);
    if ((strncasecmp(buffer, "Exposure", 8) == 0) ||
        (strncasecmp(buffer, "ExtMtrigger", 11) == 0) ||
        (strncasecmp(buffer, "ExtEnable", 9) == 0) ||
        (strncasecmp(buffer, "Exttrigger", 10) == 0)) {
        printf("Arm detector request\n");
        char cmd[64];
        char save_path[256];
        if (sscanf(buffer, "%s %s", cmd, save_path) != 2) {
            printf("Not saving\n");
            save_path[0] = '\0';
        }
        nb = snprintf(buffer, BUFFER_SIZE-1, 
                      "%s scan%d.tif", cmd, pilatus->scan_numer);
        // for null terminator added by snprintf
        nb += 1;
        pilatus->scan_numer++;
        
        char msg[1024];
        int length = snprintf(msg, 1023, 
                              "{\"htype\": \"header\","
                               "\"filename\": \"%s\"}", save_path);
        printf("Msg:%s\n", msg);
        zmq_send(pilatus->push_socket, msg, length, 0);
    }
    int bw = write(camserver_sock, buffer, nb);
    printf("req write %d\n", bw);
}

void end_of_exposure(Pilatus* pilatus)
{
    const char* msg = "{\"htype\": \"series_end\"}";
    printf("Msg:%s\n", msg);
    zmq_send(pilatus->push_socket, msg, strlen(msg), 0);
    pilatus->last_file[0] = '\0';
}

void handle_respone(char buffer[], int nb, Pilatus* pilatus)
{
    char* rest = NULL;
    char* token;
    for (token = strtok_r(buffer, "\x18", &rest);
         token != NULL;
         token = strtok_r(NULL, "\x18", &rest)) {   
        printf("token:%s\n", token);
        if (strncmp(token, "7", 1) == 0) {
            printf("Acquisition finished\n");
            char status[16];
            char path[256];
            sscanf(token, "%*d %s %s", status, path);
            
            if (strncmp(status, "OK", 2) == 0) {
                char* filename = strrchr(path, '/');
                strcpy(pilatus->last_file, filename+1);
                printf("status: %s\nlast file: %s\n", status, pilatus->last_file);
                if (strcmp(pilatus->last_file, pilatus->recent_file) == 0) {
                    end_of_exposure(pilatus);
                }
            }
            // Error in aquisition, send end of stream message
            else {
                end_of_exposure(pilatus);
            }
        }
    }
}

void handle_file(char buffer[], int nb, Pilatus* pilatus, const char* base_folder)
{
    int i = 0;
    while (i < nb) {
        struct inotify_event* event = (struct inotify_event*) &buffer[i];
        if (event->len) {
            //printf("New file: %s\n", event->name);
            strcpy(pilatus->recent_file, event->name);
            int frame_number = get_frame_number(event->name);
            //printf("frame number %d\n", frame_number);
            
            char full_path[512];
            snprintf(full_path, 512, "%s/%s", base_folder, event->name);
            FILE* fp = fopen(full_path, "rb");
            if (!fp) {
                printf("Could not open file %s\n", full_path);
            }
            
            TifInfo info;
            parse_tif(fp, &info);
            zmq_msg_t msg;
            zmq_msg_init_size(&msg, info.strip_byte_counts);
            read_tif_image(fp, &info, zmq_msg_data(&msg));
            
            fclose(fp);
            int rc = remove(full_path);
            if (rc == -1) {
                printf("Error could not delete file %s\n", full_path);
            }
            
            float exposure_time = 0.0;
            char* tmp = strstr(info.description, "Exposure_time");
            if (tmp != NULL) {
                sscanf(tmp + 13, "%f", &exposure_time);
            }
            
            char header[1024];
            int length = snprintf(header, 1024, 
                                  "{\"htype\": \"image\","
                                  "\"frame\": %d,"
                                  "\"shape\": [%d,%d],"
                                  "\"type\": \"int32\","
                                  "\"exposure_time\": %f}",
                                  frame_number, info.height, info.width, exposure_time);
            // send json header
            zmq_send(pilatus->push_socket, header, length, ZMQ_SNDMORE);
            
            // send copy of image to monitoring socket
            // FIXME send exposure time as well to monitoring socket
            if (check_monitor(pilatus->monitor_socket)) {
                zmq_msg_t copy;
                zmq_msg_init(&copy);
                zmq_msg_copy(&copy, &msg);
                zmq_send(pilatus->monitor_socket, header, length, ZMQ_SNDMORE);
                if (zmq_sendmsg(pilatus->monitor_socket, &copy, 0) == -1) {
                    printf("Error in sending message to monitor socket\n");
                }
            }
            
            // send binary blob
            zmq_sendmsg(pilatus->push_socket, &msg, 0);
            
            if (strcmp(event->name, pilatus->last_file) == 0) {
                end_of_exposure(pilatus);
            }
        }
        i += EVENT_SIZE + event->len;
    }
}

typedef struct
{
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];
} Notify;

void notify_init(Notify* notify, const char* folder, uint32_t mask)
{
    notify->fd = inotify_init();
    if (notify->fd < 0) {
        printf("Error in inotify_init\n");
    }
    notify->wd = inotify_add_watch(notify->fd, folder, mask);
    if (notify->wd == -1) {
        printf("Error watching folder %s\n%s\n", folder, strerror(errno));
        exit(-1);
    }
}

void notify_close(Notify* notify)
{
    inotify_rm_watch(notify->fd, notify->wd);
    close(notify->fd);
}

int main()
{
    const char* base_folder = "/lima_data";
    
    Pilatus pilatus;
    pilatus_init(&pilatus);
    
    int server_sock = start_server();
    int camserver_sock = connect_camserver();
    int client_sock = 0;
    
    Notify notify;
    notify_init(&notify, base_folder, IN_MOVED_TO);
    
    fd_set set;
    char buffer[BUFFER_SIZE];
    while (1) {
        FD_ZERO(&set);
        
        FD_SET(notify.fd, &set);
        int maxfd = notify.fd;
        
        FD_SET(camserver_sock, &set);
        maxfd = fmax(camserver_sock, maxfd);
        
        FD_SET(server_sock, &set);
        maxfd = fmax(server_sock, maxfd);
        
        if (client_sock > 0) {
            FD_SET(client_sock, &set);
            maxfd = fmax(client_sock, maxfd);
        }
        
        select(maxfd + 1, &set, NULL, NULL, NULL);
        
        // new request from client
        if (FD_ISSET(client_sock, &set)) {
            int nb = read(client_sock, buffer, BUFFER_SIZE);
            // client disconnected
            if (nb == 0) {
                printf("client disconnected\n");
                close(client_sock);
                client_sock = 0;
            }
            else {
                handle_request(buffer, nb, camserver_sock, &pilatus);
            }
        }
        
        // new connection from client
        else if (FD_ISSET(server_sock, &set)) {
            client_sock = accept(server_sock, (struct sockaddr*)NULL, NULL);
            if (client_sock == -1) {
                printf("Error accepting connection\n");
            }
            printf("New connection\n");
        }
        
        // new response from camserver
        else if (FD_ISSET(camserver_sock, &set)) {
            int nb = read(camserver_sock, buffer, BUFFER_SIZE);
            write(client_sock, buffer, nb);
            handle_respone(buffer, nb, &pilatus);
            bzero(buffer, BUFFER_SIZE);
        }
        
        // new data file
        else if (FD_ISSET(notify.fd, &set)) {
            int nb = read(notify.fd, notify.buffer, EVENT_BUF_LEN);
            handle_file(notify.buffer, nb, &pilatus, base_folder);
        }
    }
    
    notify_close(&notify);
    return 0;
}
