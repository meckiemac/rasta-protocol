#include <rasta_new.h>
#include <scip.h>
#include <scils.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <rmemory.h>

/** added secsys code start **/
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "pointdrv.h"

#define IL_OUT_FIFO "/run/interlocking.sock"
/** added secsys code end **/

#define CONFIG_PATH_S "../../../rasta_server.cfg"
#define CONFIG_PATH_C "../../../rasta_client1.cfg"

#define ID_S 0x61
#define ID_C 0x62

#define SCI_NAME_S "S"
#define SCI_NAME_C "C"

scip_t * scip;

/** added secsys code start **/
pthread_mutex_t mutex;  // Mutex for synchronization
pthread_cond_t cond;    // Condition variable for signaling
int target_location = 0;        // Shared variable for command
scip_t *tmp_p;
char *tmp_sender;

/**
  * @brief Signal handler for not read controller pipe
  * @param 
  */
void sig_pipe_handler(int s) {
    printf("Security Controller not listening\n");
}

/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void* thread_func(void* arg) {
    //int thread_id = *(int*)arg; // Get thread ID from argument
    //printf("Thread %d started\n", thread_id);

    scip_point_location scip_location = POINT_NO_TARGET_LOCATION;

    while (1) {
        pthread_mutex_lock(&mutex); // Acquire the mutex

        // Sleep until signaled or command is available
        while (target_location == 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("->  Point change thread received command: %d\n", target_location);
        /* move point here !*/
        switch(target_location)
        {
            case 1: /* move to left */
            {
                pointdrv_move(DIRECTION_LEFT, 1);
                break;
            }
            case 2: /* move to right */
            {
                pointdrv_move(DIRECTION_RIGHT, 1);
                break;
            }
            default:
            {
                /* Error Message? */
                break;
            }
        }
        printf("->  Checking point location ...\n");
        sleep(1);
        switch(pointdrv_get_postion())
        {
            /* left position */
            case POSITION_LEFT:
            {
                scip_location = POINT_LOCATION_LEFT;
                break;
            }
            /* right position */
            case POSITION_RIGHT:
            {
                scip_location = POINT_LOCATION_RIGHT;
                break;
            }
            case POSITION_UNKNOWN:
            {
                scip_location = POINT_NO_TARGET_LOCATION;
                break;
            }
            /* no position */
            default: 
            {
                scip_location = POINT_NO_TARGET_LOCATION;
                break;
            }
        }
        printf("->  Sending back location status %d...\n", scip_location);
        sci_return_code code = scip_send_location_status(tmp_p, tmp_sender, scip_location);
        if (code == SUCCESS){
            printf("->  Sent location status\n");
        }
        else
        {
            printf("Something went wrong, error code 0x%02X was returned!\n", code);
        }
        tmp_p = NULL;
        tmp_sender = NULL;
        target_location = 0; // Reset command after processing

        pthread_mutex_unlock(&mutex); // Release the mutex
    }

    return NULL;
}

/** 
  * @brief Exports the interlocking state and commands to the security system 
  * @param dev  Export the state for the appropriate device (1 byte)
  * @param state The interlocking state to export. l/r for move to left or right and x/y feedback of x=l/y=r position received
  */
void send_cntrl_state(char dev, char state)
{
    int fd;
    char * fifo = IL_OUT_FIFO;
    fd = open(fifo, O_WRONLY | O_NONBLOCK);

    if(fd<0)
    {
        printf("->  Can't write interlocking status %c to pipe. No listener.\n", state);
        return;
    }

    printf("Writing interlocking status %c to Pipe ...\n", state);
    char wr_req[3] = {dev, state, '\0'};
    write(fd, wr_req, 2);

    close(fd);    
}

 /** added secsys code end **/

void printHelpAndExit(void){
    printf("Invalid Arguments!\n use 's' to start in server mode and 'c' client mode.\n");
    exit(1);
}

void onReceive(struct rasta_notification_result *result){
    rastaApplicationMessage message = sr_get_received_data(result->handle, &result->connection);
    scip_on_rasta_receive(scip, message);
}

void onHandshakeComplete(struct rasta_notification_result *result){
    if (result->connection.my_id == ID_C){

        printf("Sending change location command...\n");
        sci_return_code code = scip_send_change_location(scip, SCI_NAME_S, POINT_LOCATION_CHANGE_TO_RIGHT);
        if (code == SUCCESS){
            printf("Sent change location command to server\n");
        } else{
            printf("Something went wrong, error code 0x%02X was returned!\n", code);
        }
    }
}

void onChangeLocation(scip_t * p, char * sender, scip_point_target_location location){
    printf("Received location change to 0x%02X from %s\n", location, sci_get_name_string(sender));

    /** added secsys code start **/
    pthread_mutex_lock(&mutex); // Acquire the mutex
    if(location == POINT_LOCATION_CHANGE_TO_LEFT)
    {
       target_location = 1; 
    }
    else if (location == POINT_LOCATION_CHANGE_TO_RIGHT)
    {
        target_location = 2;
    }
    tmp_p = p;
    tmp_sender = sender;

    pthread_cond_signal(&cond); // Signal the thread
    pthread_mutex_unlock(&mutex); // Release the mutex
    /** added secsys code end **/

    /* disabled original code */
    //printf("Sending back location status...\n");
    //sci_return_code code = scip_send_location_status(p, sender, scip_location);
    //if (code == SUCCESS){
    //    printf("Sent location status\n");
    //} else{
    //    printf("Something went wrong, error code 0x%02X was returned!\n", code);
    //}
}

void onLocationStatus(scip_t * p, char * sender, scip_point_location location){
    printf("Received location status from %s. Point is at position 0x%02X.\n",sci_get_name_string(sender), location);
    switch(location)
    {
        case POINT_LOCATION_LEFT:
        {
            send_cntrl_state('a', 'x');
            break;
        }
        case POINT_LOCATION_RIGHT:
        {
            send_cntrl_state('a', 'y');
            break;
        }
        case POINT_NO_TARGET_LOCATION:
        case POINT_BUMPED:
        {
            send_cntrl_state('a', 'z');
            break;
        }
        default:
        {
            send_cntrl_state('a', 'z');
            break;
        }
    }
}

int main(int argc, char *argv[]){

    if (argc != 2) printHelpAndExit();

    struct rasta_handle h;

    struct RastaIPData toServer[2];

#ifdef EXAMPLE_IP_OVERRIDE
    char *server1IP = getenv("SERVER_CH1");
    char *server2IP = getenv("SERVER_CH2");
    if (server1IP == NULL || server2IP == NULL)
    {
        printf("Error: no SERVER_CH1 or SERVER_CH2 environment variable specified\n");
        return 1;
    }
    strcpy(toServer[0].ip, server1IP);
    strcpy(toServer[1].ip, server2IP);
#else
    strcpy(toServer[0].ip, "10.0.0.100");
    strcpy(toServer[1].ip, "10.0.0.101");
#endif
    toServer[0].port = 8888;
    toServer[1].port = 8889;

    /** added secsys code start **/
    struct point_drv_cfg pdrv_cfg;

    pthread_t tid;
    if (strcmp(argv[1], "c") == 0)
    {
        /* client: add pipe singal handler and create fifo */
        signal(SIGPIPE, sig_pipe_handler);
        char * fifo = IL_OUT_FIFO;
        mkfifo(fifo, 0666);
    }
    else if (strcmp(argv[1], "s") == 0)
    {
        pdrv_cfg.drv_pin_1 = RPI_V2_GPIO_P1_11;   /* GPIO 17 */
        pdrv_cfg.drv_pin_2 = RPI_V2_GPIO_P1_12;   /* GPIO 18 */
        pdrv_cfg.drv_pin_3 = RPI_V2_GPIO_P1_13;   /* GPIO 27 */
        pdrv_cfg.drv_pin_4 = RPI_V2_GPIO_P1_15;   /* GPIO 22 */
        pdrv_cfg.pos_pin_1 = RPI_V2_GPIO_P1_18;   /* GPIO 24 Left Stop */
        pdrv_cfg.pos_pin_2 = RPI_V2_GPIO_P1_16;   /* GPIO 23 Right Stop */
        pdrv_cfg.half_steps = 1;                  /*  smother run w/ half steps */
        if(!pointdrv_init(pdrv_cfg))
        {
            exit(1);
        }
        /* server: Thread for non-blocking point move  */
        pthread_mutex_init(&mutex, NULL); 
        pthread_cond_init(&cond, NULL); 
        int thread_id = 1;
        pthread_create(&tid, NULL, thread_func, &thread_id);
    }
    /** added secsys code end **/


    printf("Server at %s:%d and %s:%d\n", toServer[0].ip, toServer[0].port, toServer[1].ip, toServer[1].port);

    if (strcmp(argv[1], "s") == 0) {
        printf("->   R (ID = 0x%lX)\n", (unsigned long)ID_S);

        getchar();
        sr_init_handle(&h, CONFIG_PATH_S);
        h.notifications.on_receive = onReceive;
        h.notifications.on_handshake_complete = onHandshakeComplete;
        scip = scip_init(&h, SCI_NAME_S);
        scip->notifications.on_change_location_received = onChangeLocation;
    }
    else if (strcmp(argv[1], "c") == 0) {
        printf("->   S1 (ID = 0x%lX)\n", (unsigned long)ID_C);

        sr_init_handle(&h, CONFIG_PATH_C);
        h.notifications.on_receive = onReceive;
        h.notifications.on_handshake_complete = onHandshakeComplete;
        printf("->   Press Enter to connect\n");
        getchar();
        sr_connect(&h,ID_S,toServer);

        scip = scip_init(&h, SCI_NAME_C);
        scip->notifications.on_location_status_received = onLocationStatus;

        scip_register_sci_name(scip, SCI_NAME_S, ID_S);
    }

    /** added secsys code start **/
    if (strcmp(argv[1], "s") == 0)
    {
        getchar();
    }
    else
    {
        int stop = 0;
        while(!stop)
        {
            char m = getchar();
            switch(m) /* move point as client */
            {
                case 'l':
                {
                    printf("->   Moving left\n");
                    sci_return_code code = scip_send_change_location(scip, SCI_NAME_S, POINT_LOCATION_CHANGE_TO_LEFT);
                    if (code == SUCCESS){
                        send_cntrl_state('a', 'l');
                    } else
                    {
                        printf("Something went wrong, error code 0x%02X was returned!\n", code);
                    }
                    break;
                }
                case 'r':
                {
                    printf("->   Moving right\n");
                    sci_return_code code = scip_send_change_location(scip, SCI_NAME_S, POINT_LOCATION_CHANGE_TO_RIGHT);
                    if (code == SUCCESS){
                        send_cntrl_state('a', 'r');
                    } else
                    {
                        printf("Something went wrong, error code 0x%02X was returned!\n", code);
                    }
                    break;
                }
                case 'q':
                {
                    stop = 1;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    pthread_join(tid, NULL);

    pthread_cond_destroy(&cond);   
    pthread_mutex_destroy(&mutex);
    /** added secsys code end **/

    scip_cleanup(scip);
    sr_cleanup(&h);

    return 0;
}
