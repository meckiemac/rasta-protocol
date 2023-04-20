#include <rasta_new.h>
#include <scip.h>
#include <scils.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <rmemory.h>

/* Additiona: Location of the python script */
#define POINT_MOVE_SCRIPT "/home/meckie/src/point_move/point_move.py"

#define CONFIG_PATH_S "../../../rasta_server.cfg"
#define CONFIG_PATH_C "../../../rasta_client1.cfg"

#define ID_S 0x61
#define ID_C 0x62

#define SCI_NAME_S "S"
#define SCI_NAME_C "C"

scip_t * scip;

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
    int status;
    char script[] = POINT_MOVE_SCRIPT;
    char s_arg[3] = { 0x20, 0x20, 0x0 };

    scip_point_location scip_location = POINT_NO_TARGET_LOCATION;

    if(location == POINT_LOCATION_CHANGE_TO_RIGHT)
    {
       s_arg[1] = '1';
    }
    else if (location == POINT_LOCATION_CHANGE_TO_LEFT)
    {
        s_arg[1] = '2';
    }

    printf("Running script %s \n", strcat(script,s_arg)); /* debug & remove*/

    status = system(script);

    printf("Got exit code %d \n", (status / 256)); /* debug & remove*/
    //int exitcode = status / 256;

    switch ((status / 256))
    {
        case 5: /* left position */
        {
            scip_location = POINT_LOCATION_LEFT;
            break;
        }
        case 6: /* right position */
        {
            scip_location = POINT_LOCATION_RIGHT;
            break;
        }
        default: /* no position */
        {
            scip_location = POINT_NO_TARGET_LOCATION;
            break;
        }
    }
    /** added secsys code end **/

    printf("Sending back location status...\n");
    sci_return_code code = scip_send_location_status(p, sender, scip_location);
    if (code == SUCCESS){
        printf("Sent location status\n");
    } else{
        printf("Something went wrong, error code 0x%02X was returned!\n", code);
    }
}

void onLocationStatus(scip_t * p, char * sender, scip_point_location location){
    printf("Received location status from %s. Point is at position 0x%02X.\n",sci_get_name_string(sender), location);
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
                    printf("->   Moving Left\n");
                    sci_return_code code = scip_send_change_location(scip, SCI_NAME_S, POINT_LOCATION_CHANGE_TO_RIGHT);
                    break;
                }
                case 'r':
                {
                    printf("->   Moving Right\n");
                    sci_return_code code = scip_send_change_location(scip, SCI_NAME_S, POINT_LOCATION_CHANGE_TO_RIGHT);
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
    /** added secsys code end **/

    scip_cleanup(scip);
    sr_cleanup(&h);

    return 0;
}
