#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PRODUCTS 20
#define MAX_CLIENTS 5
#define MAX_FAILED_ORDERS 100
#define PORT 8080
#define SERVER_IP "127.0.0.1"

//Domi gia ta proionta
typedef struct {
    char description[50]; 
    float price;          // Timh
    int item_count;       // Arithmos temaxiwn
    int requests;         // Zitoumena proionta
    int sold;             // Pwliseis
} Product;

// Domi gia paraggelies
typedef struct {
    int product_id; // ID proiontos
    int quantity;   // Temaxia
} Order;

// Domi gia apotuximena
typedef struct {
    int client_id;   // ID pelati sta failed
    int product_id;  // ID proiontos sta failed
    int quantity;    // Temaxia sta failed
} FailedOrder;

//Domi gia tin ektupwsi dedomenwn
typedef struct {
    int total_orders;
    int successful_orders;
    int failed_orders_count;
    float total_revenue;
    FailedOrder failed_orders[MAX_FAILED_ORDERS];
    int failed_order_count;
} SharedData;

//Prototupa sunartisewn
void init_catalog(); // Arxikopoiisi
void process_order(int client_socket, int client_id); //Pragmatopoisi paraggelias
void print_report(); //Synartisi ektupwsis