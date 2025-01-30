#include "ergasia3.h"

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char response[100];

    srand(time(NULL) ^ getpid());

    // Dhmiourgia kai elegxos socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Orismos tou server kai elegxos
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Sundesi ston server kai elegxos
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    // Epanalipsi gia 10 paraggelies gia kathe pellati
    for (int i = 0; i < 10; i++) {
        Order order;
        order.product_id = rand() % MAX_PRODUCTS + 1;
        order.quantity = 1;

        printf("Client: Attempting to order Product %d\n", order.product_id);

        if (write(client_fd, &order, sizeof(Order)) == -1) { //Elegxos gia ta pipes kai an isxuei ektupwnw katallhlo minima
            perror("Error sending order");
            break;
        }

        ssize_t bytes_read = read(client_fd, response, sizeof(response));
        if (bytes_read <= 0) { //An ta bytes einai <=0 den exoume labei apantisi kai ektupwnoume katallhlo minima 
            perror("Error reading response");
            break;
        }

        printf("Client: Ordered Product %d -> %s\n", order.product_id, response);
        sleep(1);
    }

    // Close the socket
    close(client_fd);
    return 0;
}