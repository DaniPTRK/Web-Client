#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "parson.h"
#include "helpers.h"
#include "requests.h"

#define SERVER_ADDR "34.246.184.49"
#define SERVER_PORT 8080
#define JSON_TYPE "application/json"
#define SERVER_LOGOUT "/api/v1/tema/auth/logout"
#define SERVER_ENTRY "/api/v1/tema/library/access"
#define SERVER_BOOKS "/api/v1/tema/library/books"

void resize_string(char *string) {
    /* Elimina endline-ul de la finalul unui string. */
    if(string[strlen(string) - 1] == '\n') {
        string[strlen(string) - 1] = 0;
    }
}

int check(char *cookies, char *token) {
    /* Verifica daca clientul este logat la server.
    Daca acesta este logat, verifica daca acesta are acces la librarie.*/
    if(cookies == NULL) {
        printf("[ERROR] You're not logged in.\n");
        return 0;
    } else if(token == NULL) {
        printf("[ERROR] You haven't accessed the library.\n");
        return 0;
    }

    return 1;
}

int check_num(char *id) {
    /* Verifica daca input-ul primit poate fi transformat intr-un int. */
    if(*id == '\0') {
        return -1;
    }
    for(int i = 0; i < strlen(id); i++) {
        if(id[i] < '0' || id[i] > '9') {
            return -1;
        }
    }
    return atoi(id);
}

int check_for_OK(char **message, char **response, char *err_msg) {
    /* Verifica daca raspunsul primit de la server este OK. 
    In caz contrar, afiseaza un mesaj de eroare clientului. */
    if(strstr(*response, "200 OK") == NULL) {
        printf("%s\n", err_msg);
        free(*message);
        free(*response);
        return 0;
    }

    return 1;
}

void register_user(int sockfd, char *command, char **cookies) {
    /* Procesul de inregistrare/logare al unui user pe server.*/
    char username[BUFLEN], password[BUFLEN];
    printf("username=");
    fgets(username, BUFLEN, stdin);
    printf("password=");
    fgets(password, BUFLEN, stdin);

    /* Se verifica daca input-ul primit respecta restrictiile. */
    if(strchr(username, ' ') != NULL || username[0] == '\n') {
        printf("[ERROR] Username is empty or contains spaces.\n");
    } else if(strchr(password, ' ') != NULL || password[0] == '\n') {
        printf("[ERROR] Password is empty or contains spaces.\n");
    } else {
        /* In cazul in care input-urile sunt valide, construieste
        un JSON Object ce contine username-ul si parola. */
        char *route, *to_send, *message, *response, *status_code, *temp;
        route = malloc(BUFLEN * sizeof(char));
        JSON_Value *val = json_value_init_object();
        JSON_Object *obj = json_value_get_object(val);

        sprintf(route, "/api/v1/tema/auth/%s", command);
        resize_string(username);
        resize_string(password);

        json_object_set_string(obj, "username", username);
        json_object_set_string(obj, "password", password);
        to_send = json_serialize_to_string(val);

        /* Trimite un POST request catre server. */
        message = compute_post_request(SERVER_ADDR, route, JSON_TYPE,
                                            &to_send, 1, NULL, 0, NULL);
        send_to_server(sockfd, message);
        response = receive_from_server(sockfd);

        /* Verifica raspunsul server-ului la request-ul primit. */
        status_code = strstr(response, "400 Bad Request");
        if(status_code != NULL) {
            /* Daca este bad request, identifica motivul pentru care s-a
            obtinut codul 400 si afiseaza user-ului problema. */
            if(strcmp(command, "register") == 0) {
                printf("[ERROR] Username already exists.\n");
            } else {
                status_code = strstr(response, "Credentials are not good");
                if(status_code != NULL) {
                    printf("[ERROR] Credentials are wrong.\n");
                } else {
                    printf("[ERROR] User does not exist.\n");
                }
            }
        } else {
            /* Raspuns pozitiv.*/
            if(strcmp(command, "register") == 0) {
                printf("[SUCCESS] User %s registered!\n", username);
            } else {
                /* Extrage cookie-ul din raspunsul primit la logare. */
                printf("[SUCCESS] You're successfully logged in!\n");

                temp = strstr(response, "connect.sid=");
                char *end_cookie = strstr(temp, ";");
                *end_cookie = 0;
                *cookies = malloc((strlen(temp) + 1)*sizeof(char));
                strcpy(*cookies, temp);
            }
        }

        json_value_free(val);
        free(route);
        free(to_send);
        free(message);
        free(response);
    }
}

void logout_user(int sockfd, char **cookies, char **token) {
    /* Procesul de delogare al unui user pe server.*/
    /* Verifica mai intai daca user-ul este conectat la un cont.*/
    if(cookies == NULL) {
        printf("[ERROR] You're not logged into an account.\n");
        return;
    }
    char *message, *response;

    /* Trimite catre server un request pentru logout. */
    message = compute_get_request(SERVER_ADDR, SERVER_LOGOUT, NULL,
                                                    cookies, 1, *token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    /* Verifica daca raspunsul este OK si elibereaza memoria.*/
    if(check_for_OK(&message, &response, "[ERROR] Couldn't log out.\n") == 0) {
        return;
    }
    printf("[SUCCESS] You've successfully logged out!\n");

    free(*cookies);
    free(*token);
    free(message);
    free(response);
    *cookies = NULL;
    *token = NULL;
}

void enter_user(int sockfd, char *cookies, char **token) {
    /* Procesul de accesare al bibliotecii. */
    char *message, *response, *end_token, *temp;

    /* Trimite o cerere catre server pentru a obtine token-ul de acces. */
    message = compute_get_request(SERVER_ADDR, SERVER_ENTRY, NULL, 
                                                    &cookies, 1, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    /* Verifica raspunsul server-ului. */
    if(check_for_OK(&message, &response, 
                    "[ERROR] Couldn't enter the library.\n") == 0) {
        return;
    }

    /* In cazul unui raspuns pozitiv, extrage token-ul din raspuns. */
    printf("[SUCCESS] You can now access the library!\n");

    temp = strstr(response, "{\"token\":\"");
    temp += strlen("{\"token\":\"");
    end_token = strstr(temp, "\"}");
    *end_token = 0;
    *token = malloc((strlen(temp) + 1)*sizeof(char));
    strcpy(*token, temp);

    free(message);
    free(response);
}

void show_all_books(int sockfd, char *cookies, char *token) {
    /* Afiseaza toate cartile de pe server. */
    char *message, *response, *all_books;

    /* Realizeaza o cerere folosind cookie-ul si token-ul. */
    message = compute_get_request(SERVER_ADDR, SERVER_BOOKS, NULL,
                                                    &cookies, 1, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);


    if(check_for_OK(&message, &response,
                    "[ERROR] You don't have access to the library.\n") == 0) {
        return;
    }

    /* Preia din raspunsul server-ului bucata ce contine un JSON Array.*/
    all_books = strchr(response, '[');

    /* Verifica daca array-ul este gol. */
    if(*(all_books + 1) == ']') {
        printf("[SUCCESS] There are no books inside the library.\n");
    } else {
        /* Afiseaza cartile din biblioteca. */
        printf("[SUCCESS] Books inside the library:\n");
        printf("%s\n", all_books);
    }

    free(message);
    free(response);
}

void checkout_book(int sockfd, char *cookies, char *token, char *command) {
    /* Afiseaza continutul unei carti folosindu-se de ID sau
    sterge cartea ce corespunde ID-ului dat */
    char *message, *response, *route, *temp, *tempend;
    char id[BUFLEN];

    /*Extrage ID-ul.*/
    printf("id=");
    fgets(id, BUFLEN, stdin);
    resize_string(id);
    
    /* Verifica daca ID-ul este de forma numerica.*/
    if(check_num(id) < 0) {
        printf("[ERROR] ID must be an unsigned integer.\n");
    } else {
        /* Construieste ruta folosind ID-ul. */
        route = malloc(BUFLEN * sizeof(char));
        strcpy(route, SERVER_BOOKS);
        strcat(route, "/");
        strcat(route, id);

        /* Daca comanda este get_book, se face o cerere GET.
        Daca comanda este delete_book, se face o cerere DELETE. */
        if(strcmp(command, "get_book") == 0) {
            message = compute_get_request(SERVER_ADDR, route, NULL,
                                                        &cookies, 1, token);
        } else {
            message = compute_delete_request(SERVER_ADDR, route, NULL,
                                                        &cookies, 1, token);
        }

        send_to_server(sockfd, message);
        response = receive_from_server(sockfd);

        if(check_for_OK(&message, &response,
                                "[ERROR] Book couldn't be found.\n") == 0) {
            free(route);
            return;
        }

        if(strcmp(command, "get_book") == 0) {
            /* Afiseaza detaliile despre carte. */
            temp = strstr(response, "{\"id\"");
            tempend = strchr(temp, '}');
            *(tempend + 1) = 0;

            printf("[SUCCESS] Information found!\n");
            printf("%s\n", temp);
        } else {
            /* Afiseaza un mesaj de feedback. */
            printf("[SUCCESS] Book with id %s has been succesfully deleted.\n", id);
        }

        free(route);
        free(message);
        free(response);
    }
}

void new_book(int sockfd, char *cookies, char *token) {
    /* Insereaza o noua carte in biblioteca. */
    char *message, *response, *to_send;
    char title[BUFLEN], author[BUFLEN], genre[BUFLEN], publisher[BUFLEN],
            page_count[BUFLEN];

    /* Extrage informatii despre noua carte. */
    printf("title=");
    fgets(title, BUFLEN, stdin);
    resize_string(title);
    printf("author=");
    fgets(author, BUFLEN, stdin);
    resize_string(author);
    printf("genre=");
    fgets(genre, BUFLEN, stdin);
    resize_string(genre);
    printf("publisher=");
    fgets(publisher, BUFLEN, stdin);
    resize_string(publisher);
    printf("page_count=");
    fgets(page_count, BUFLEN, stdin);
    resize_string(page_count);

    /* Asiguram faptul ca input-ul primit este valid. */
    if((title[0] == '\0') || (author[0] == '\0') || (genre[0] == '\0') ||
            (publisher[0] == '\0') || (check_num(page_count) < 0)) {
        printf("[ERROR] The data provided is invalid.\n");
        return;
    }

    /* Construim JSON Object-ul ce va fi pus in biblioteca.*/
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "title", title);
    json_object_set_string(obj, "author", author);
    json_object_set_string(obj, "genre", genre);
    json_object_set_number(obj, "page_count", check_num(page_count));
    json_object_set_string(obj, "publisher", publisher);

    to_send = json_serialize_to_string(val);

    /* Trimite in server noua carte. */
    message = compute_post_request(SERVER_ADDR, SERVER_BOOKS, JSON_TYPE,
                                                &to_send, 1, NULL, 0, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    /* Verifica daca server-ul a primit cartea. */
    if(strstr(response, "200 OK") == NULL) {
        printf("[ERROR] Couldn't add the book inside the library.\n");
    } else {
        printf("[SUCCESS] Book has been added successfully!\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    free(to_send);
}

int main(int argc, char *argv[])
{
    char *cookies, *token, *command;
    cookies = NULL;
    token = NULL;
    int sockfd;

    while(1) {

        sockfd = open_connection(SERVER_ADDR, SERVER_PORT, AF_INET,
                                                            SOCK_STREAM, 0);

        command = malloc(BUFLEN * sizeof(char));
        fgets(command, BUFLEN, stdin);
        resize_string(command);

        /* Pentru fiecare comanda primita, executa actiunea corespunzatoare.
        Pentru operatiile ce necesita ca utilizatorul sa fie logged in/
        intrat in biblioteca, verifica cookie-urile si token-urile.*/
        if(strcmp(command, "register") == 0 || strcmp(command, "login") == 0) {
            if(cookies == NULL) {
                register_user(sockfd, command, &cookies);
            } else {
                printf("[ERROR] You're already connected with an account.\n");
            }
        } else if(strcmp(command, "enter_library") == 0) {
            if(cookies != NULL) {
                enter_user(sockfd, cookies, &token);
            } else {
                printf("[ERROR] You're not logged in.\n");
            }
        } else if(strcmp(command, "get_books") == 0) {
            if(check(cookies, token)) {
                show_all_books(sockfd, cookies, token);
            }
        } else if(strcmp(command, "get_book") == 0 ||
                        strcmp(command, "delete_book") == 0) {
            if(check(cookies, token)) {
                checkout_book(sockfd, cookies, token, command);
            }
        } else if(strcmp(command, "add_book") == 0) {
            if(check(cookies, token)) {
                new_book(sockfd, cookies, token);
            }
        } else if(strcmp(command, "logout") == 0) {
            logout_user(sockfd, &cookies, &token);
        } else if(strcmp(command, "exit") == 0) {
            close(sockfd);
            free(command);
            free(cookies);
            free(token);
            break;
        } else {
            printf("[ERROR] Command doesn't exist.\n");
        }

        close(sockfd);
        free(command);
    }

    return 0;
}
