#include "ergasia3.h"

// Global metablites
Product catalog[MAX_PRODUCTS]; // Pinakas me ta proionta tou catalog
SharedData *shared_data;       // Shared memory gia na koinopoiountai ta dedomena metaksi ton diergasion

// Arxikopoiisi tou catalog me ta proionta
void init_catalog() {
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        snprintf(catalog[i].description, sizeof(catalog[i].description), "Product %d", i + 1); // Perigrafi proiontos
        catalog[i].price = (i + 1) * 10.0; // Timi proiontos
        catalog[i].item_count = 2;         // Arithmos diathesimwn antitypwn
        catalog[i].requests = 0;           // Arithmos aitimatwn gia to proion
        catalog[i].sold = 0;               // Arithmos poulimenwn antitypwn
    }
}

// Diacheirisi mias paraggelias apo ton pelati
void process_order(int client_socket, int client_id) {
    Order order;
    ssize_t bytes_read = 0;
    size_t total_bytes_read = 0;

    // Diavazei tin paraggelia apo ton pelati
    while (total_bytes_read < sizeof(Order)) {
        bytes_read = read(client_socket, (char *)&order + total_bytes_read, sizeof(Order) - total_bytes_read);
        if (bytes_read <= 0) {
            perror("Error reading order");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        total_bytes_read += bytes_read;
    }

    // Enhmerwnei ton synoliko arithmo paraggelion sto shared memory
    shared_data->total_orders++;

    // Enhmerwnei ta aitimata gia to proion
    catalog[order.product_id - 1].requests++;

    char response[100];

    // Elegxei an yparxei arketo stock gia to proion
    if (catalog[order.product_id - 1].item_count >= order.quantity) {
        // Enhmerwnei to stock kai ta poulimena antitypa
        catalog[order.product_id - 1].item_count -= order.quantity;
        catalog[order.product_id - 1].sold += order.quantity;

        // Ypologizei to synoliko kostos kai to prosthetei sto revenue
        float cost = catalog[order.product_id - 1].price * order.quantity;
        shared_data->total_revenue += cost;

        // Enhmerwnei tis epityxeis paraggelies
        shared_data->successful_orders++;

        // Etimazei to minima epityxias
        snprintf(response, sizeof(response), "Order Successful! Total cost: %.2f\n", cost);
    } else {
        // Enhmerwnei tis apotyxeis paraggelies
        shared_data->failed_orders_count++;

        // Apothikeyei tin apotyxiameni paraggelia
        if (shared_data->failed_order_count < MAX_FAILED_ORDERS) {
            shared_data->failed_orders[shared_data->failed_order_count].client_id = client_id;
            shared_data->failed_orders[shared_data->failed_order_count].product_id = order.product_id;
            shared_data->failed_orders[shared_data->failed_order_count].quantity = order.quantity;
            shared_data->failed_order_count++;
        }

        // Etimazei to minima apotyxias
        snprintf(response, sizeof(response), "Order Failed! Not enough stock.\n");
    }

    // Stelnei tin apantisi ston pelati
    if (write(client_socket, response, strlen(response) + 1) == -1) {
        perror("Error writing response");
    }
}

// Ektypwnei tin apografi ton pwliseon
void print_report() {
    printf("\n--- Eshop Report ---\n");
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        printf("Product: %s\n", catalog[i].description);
        printf("  Requests: %d\n", catalog[i].requests);
        printf("  Sold: %d\n", catalog[i].sold);
        printf("  Remaining: %d\n", catalog[i].item_count);
    }
    printf("\n--- Unserved Clients ---\n");
    for (int i = 0; i < shared_data->failed_order_count; i++) {
        printf("Client %d failed to buy %d of Product %d\n",
               shared_data->failed_orders[i].client_id,
               shared_data->failed_orders[i].quantity,
               shared_data->failed_orders[i].product_id);
    }
    printf("\n--- Summary ---\n");
    printf("Total Orders: %d\n", shared_data->total_orders);
    printf("Successful Orders: %d\n", shared_data->successful_orders);
    printf("Failed Orders: %d\n", shared_data->failed_orders_count);
    printf("Total Revenue: %.2f\n", shared_data->total_revenue);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_id = 0;

    // Arxikopoiisi tou catalog
    init_catalog();

    // Dimiourgia shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Arxikopoiisi ton dedomenon sto shared memory
    memset(shared_data, 0, sizeof(SharedData));

    // Dimiourgia socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind to socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Akouei gia syndeseis
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT);

    // Diacheirisi syndeseon apo pelates
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        client_id++;
        if (fork() == 0) { // Dimiourgia paidiou diergasias
            close(server_fd);
            for (int j = 0; j < 10; j++) {
                process_order(client_fd, client_id); // Diacheirisi paraggelias
                sleep(1);
            }
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    // Perimenei na teleiosoun oi paidies diergasies
    while (wait(NULL) > 0);

    // Ektypwnei tin teliki apografi
    print_report();

    // Apallassei kai diagrafei to shared memory
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);

    // Kleisei tou socket tou server
    close(server_fd);
    return 0;
}