#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 65536
#define USERS_FILE "users.txt"
#define VEHICLES_FILE "vehicles.txt"

// Predefined Admin Credentials
#define ADMIN_EMAIL "admin@rental.com"
#define ADMIN_PASS "Admin@123"

typedef struct {
    char name[100];
    char email[100];
    char phone[30];
    char password[100];
    char sec_q[200];
    char sec_a[100];
    int failed_attempts;
} User;

typedef struct {
    char id[20];
    char name[100];
    char type[50];
    double price;
    char status[30]; // "Available" or "Rented"
} Vehicle;

// --- Utility Functions ---

void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

void get_param(const char *body, const char *key, char *out, int max_len) {
    out[0] = '\0';
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *pos = strstr(body, search);
    if (!pos) return;
    if (pos != body && *(pos - 1) != '&') return;

    pos += strlen(search);
    const char *end = strchr(pos, '&');
    int len = end ? (int)(end - pos) : (int)strlen(pos);
    char encoded[512];
    if (len >= sizeof(encoded)) len = sizeof(encoded) - 1;
    strncpy(encoded, pos, len);
    encoded[len] = '\0';
    url_decode(out, encoded);
    out[max_len - 1] = '\0';
}

// --- Validation Functions ---

int validate_email(const char *email) {
    const char *at = strchr(email, '@');
    if (!at || at == email) return 0;
    const char *dot = strchr(at, '.');
    if (!dot || dot == at + 1 || *(dot + 1) == '\0') return 0;
    return 1;
}

int validate_password(const char *pass) {
    if (strlen(pass) < 6) return 0;
    int has_letter = 0, has_digit = 0, has_special = 0;
    for (int i = 0; pass[i]; i++) {
        if (isalpha((unsigned char)pass[i])) has_letter = 1;
        else if (isdigit((unsigned char)pass[i])) has_digit = 1;
        else has_special = 1;
    }
    return has_letter && has_digit && has_special;
}

// --- File Initialization ---

void init_files() {
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp) fclose(fp);

    fp = fopen(VEHICLES_FILE, "r");
    if (!fp) {
        fp = fopen(VEHICLES_FILE, "w");
        if (fp) {
            fprintf(fp, "V101|Toyota Corolla|Sedan|3500.00|Available\n");
            fprintf(fp, "V102|Honda Civic|Sedan|4200.00|Available\n");
            fprintf(fp, "V103|Mitsubishi Pajero|SUV|7500.00|Available\n");
            fprintf(fp, "V104|Yamaha R15|Bike|1500.00|Rented\n");
            fclose(fp);
        }
    } else {
        fclose(fp);
    }
}

// --- User File Operations ---

int email_exists(const char *email) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) return 0;
    char line[600];
    User u;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%99[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

void register_user(const char *body, char *response_json) {
    User u;
    get_param(body, "name", u.name, sizeof(u.name));
    get_param(body, "email", u.email, sizeof(u.email));
    get_param(body, "phone", u.phone, sizeof(u.phone));
    get_param(body, "password", u.password, sizeof(u.password));
    get_param(body, "sec_q", u.sec_q, sizeof(u.sec_q));
    get_param(body, "sec_a", u.sec_a, sizeof(u.sec_a));
    u.failed_attempts = 0;

    if (strlen(u.name) == 0 || strlen(u.phone) == 0 || strlen(u.sec_q) == 0 || strlen(u.sec_a) == 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"All fields are required.\"}");
        return;
    }
    if (!validate_email(u.email)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid email format.\"}");
        return;
    }
    if (strcmp(u.email, ADMIN_EMAIL) == 0 || email_exists(u.email)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Email already registered or restricted.\"}");
        return;
    }
    if (!validate_password(u.password)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Password must be 6+ chars with 1 letter, 1 number & 1 special symbol.\"}");
        return;
    }

    FILE *fp = fopen(USERS_FILE, "a");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error saving user.\"}");
        return;
    }
    fprintf(fp, "%s|%s|%s|%s|%s|%s|0\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a);
    fclose(fp);
    strcpy(response_json, "{\"status\":\"success\",\"message\":\"Registration successful! Please login.\"}");
}

void login_user(const char *body, char *response_json) {
    char email[100], password[100];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "password", password, sizeof(password));

    FILE *fp = fopen(USERS_FILE, "r");
    FILE *temp = fopen("temp_users.txt", "w");
    if (!fp || !temp) {
        if (fp) fclose(fp);
        if (temp) fclose(temp);
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[600];
    User u;
    int found = 0;
    strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid email or password.\"}");

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%99[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                found = 1;
                if (u.failed_attempts >= 5) {
                    snprintf(response_json, BUFFER_SIZE,
                             "{\"status\":\"locked\",\"message\":\"Account locked after 5 failed attempts.\",\"sec_q\":\"%s\"}",
                             u.sec_q);
                } else if (strcmp(u.password, password) == 0) {
                    u.failed_attempts = 0;
                    snprintf(response_json, BUFFER_SIZE,
                             "{\"status\":\"success\",\"user\":{\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\"}}",
                             u.name, u.email, u.phone);
                } else {
                    u.failed_attempts++;
                    if (u.failed_attempts >= 5) {
                        snprintf(response_json, BUFFER_SIZE,
                                 "{\"status\":\"locked\",\"message\":\"5 incorrect attempts! Answer security question to reset password.\",\"sec_q\":\"%s\"}",
                                 u.sec_q);
                    } else {
                        snprintf(response_json, BUFFER_SIZE,
                                 "{\"status\":\"error\",\"message\":\"Incorrect password! Attempts left: %d\"}",
                                 5 - u.failed_attempts);
                    }
                }
            }
            fprintf(temp, "%s|%s|%s|%s|%s|%s|%d\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, u.failed_attempts);
        }
    }

    fclose(fp);
    fclose(temp);
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);

    if (!found) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"No account found with this email.\"}");
    }
}

void reset_password(const char *body, char *response_json) {
    char email[100], sec_a[100], new_pass[100], confirm_pass[100];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "sec_a", sec_a, sizeof(sec_a));
    get_param(body, "new_pass", new_pass, sizeof(new_pass));
    get_param(body, "confirm_pass", confirm_pass, sizeof(confirm_pass));

    if (strcmp(new_pass, confirm_pass) != 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Passwords do not match.\"}");
        return;
    }
    if (!validate_password(new_pass)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"New password must be 6+ chars with 1 letter, 1 number & 1 special symbol.\"}");
        return;
    }

    FILE *fp = fopen(USERS_FILE, "r");
    FILE *temp = fopen("temp_users.txt", "w");
    if (!fp || !temp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error.\"}");
        return;
    }

    char line[600];
    User u;
    int updated = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%99[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                if (strcmp(u.sec_a, sec_a) == 0) {
                    strcpy(u.password, new_pass);
                    u.failed_attempts = 0;
                    updated = 1;
                }
            }
            fprintf(temp, "%s|%s|%s|%s|%s|%s|%d\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, u.failed_attempts);
        }
    }

    fclose(fp);
    fclose(temp);
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);

    if (updated) {
        strcpy(response_json, "{\"status\":\"success\",\"message\":\"Password reset successful! You can now log in.\"}");
    } else {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Incorrect security answer.\"}");
    }
}

void get_all_users(char *response_json) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"success\",\"users\":[]}");
        return;
    }
    strcpy(response_json, "{\"status\":\"success\",\"users\":[");
    char line[600], item[400];
    User u;
    int first = 1;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%99[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (!first) strcat(response_json, ",");
            snprintf(item, sizeof(item), "{\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\"}",
                     u.name, u.email, u.phone);
            strcat(response_json, item);
            first = 0;
        }
    }
    strcat(response_json, "]}");
    fclose(fp);
}

// --- Vehicle File Operations ---

void get_all_vehicles(char *response_json) {
    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"success\",\"vehicles\":[]}");
        return;
    }
    strcpy(response_json, "{\"status\":\"success\",\"vehicles\":[");
    char line[300], item[300];
    Vehicle v;
    int first = 1;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (!first) strcat(response_json, ",");
            snprintf(item, sizeof(item), "{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"price\":%.2f,\"status\":\"%s\"}",
                     v.id, v.name, v.type, v.price, v.status);
            strcat(response_json, item);
            first = 0;
        }
    }
    strcat(response_json, "]}");
    fclose(fp);
}

int vehicle_id_exists(const char *id) {
    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (!fp) return 0;
    char line[300];
    Vehicle v;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%19[^|]|", v.id) == 1) {
            if (strcmp(v.id, id) == 0) {
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

void add_vehicle(const char *body, char *response_json) {
    Vehicle v;
    char price_str[30];
    get_param(body, "id", v.id, sizeof(v.id));
    get_param(body, "name", v.name, sizeof(v.name));
    get_param(body, "type", v.type, sizeof(v.type));
    get_param(body, "price", price_str, sizeof(price_str));
    get_param(body, "status", v.status, sizeof(v.status));
    v.price = atof(price_str);

    if (strlen(v.id) == 0 || strlen(v.name) == 0 || strlen(v.type) == 0 || v.price <= 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Please provide valid vehicle details.\"}");
        return;
    }
    if (vehicle_id_exists(v.id)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID already exists.\"}");
        return;
    }

    FILE *fp = fopen(VEHICLES_FILE, "a");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Could not open vehicles file.\"}");
        return;
    }
    fprintf(fp, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
    fclose(fp);
    strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle added successfully.\"}");
}

void update_vehicle(const char *body, char *response_json) {
    char id[20], name[100], type[50], price_str[30], status[30];
    get_param(body, "id", id, sizeof(id));
    get_param(body, "name", name, sizeof(name));
    get_param(body, "type", type, sizeof(type));
    get_param(body, "price", price_str, sizeof(price_str));
    get_param(body, "status", status, sizeof(status));

    FILE *fp = fopen(VEHICLES_FILE, "r");
    FILE *temp = fopen("temp_vehicles.txt", "w");
    if (!fp || !temp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error.\"}");
        return;
    }

    char line[300];
    Vehicle v;
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, id) == 0) {
                found = 1;
                if (strlen(name) > 0) strcpy(v.name, name);
                if (strlen(type) > 0) strcpy(v.type, type);
                if (strlen(price_str) > 0 && atof(price_str) > 0) v.price = atof(price_str);
                if (strlen(status) > 0) strcpy(v.status, status);
            }
            fprintf(temp, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }

    fclose(fp);
    fclose(temp);
    remove(VEHICLES_FILE);
    rename("temp_vehicles.txt", VEHICLES_FILE);

    if (found) {
        strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle updated successfully.\"}");
    } else {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID not found.\"}");
    }
}

void delete_vehicle(const char *body, char *response_json) {
    char id[20];
    get_param(body, "id", id, sizeof(id));

    FILE *fp = fopen(VEHICLES_FILE, "r");
    FILE *temp = fopen("temp_vehicles.txt", "w");
    if (!fp || !temp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error.\"}");
        return;
    }

    char line[300];
    Vehicle v;
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, id) == 0) {
                found = 1;
                continue; // Skip writing to delete
            }
            fprintf(temp, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }

    fclose(fp);
    fclose(temp);
    remove(VEHICLES_FILE);
    rename("temp_vehicles.txt", VEHICLES_FILE);

    if (found) {
        strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle deleted permanently.\"}");
    } else {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID not found.\"}");
    }
}

// --- HTTP Response Helpers ---

void send_response(SOCKET client, const char *content_type, const char *body) {
    char header[512];
    int body_len = (int)strlen(body);
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s; charset=UTF-8\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n",
             content_type, body_len);
    send(client, header, (int)strlen(header), 0);
    send(client, body, body_len, 0);
}

void serve_html(SOCKET client) {
    FILE *fp = fopen("index.html", "rb");
    if (!fp) {
        send_response(client, "text/plain", "Error: index.html not found in project directory.");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *html = (char *)malloc(fsize + 1);
    fread(html, 1, fsize, fp);
    fclose(fp);
    html[fsize] = '\0';

    send_response(client, "text/html", html);
    free(html);
}

// --- Main Server Loop ---

int main() {
    init_files();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Could not create socket.\n");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed on port %d.\n", PORT);
        return 1;
    }

    listen(server_fd, 10);
    printf("==================================================\n");
    printf("  C Vehicle Rental System Web Server Running!\n");
    printf("  Open your browser at: http://localhost:%d\n", PORT);
    printf("  Default Admin: %s / %s\n", ADMIN_EMAIL, ADMIN_PASS);
    printf("==================================================\n");

    char *buffer = (char *)malloc(BUFFER_SIZE);
    char *json_out = (char *)malloc(BUFFER_SIZE);

    while (1) {
        struct sockaddr_in client_addr;
        int c = sizeof(struct sockaddr_in);
        SOCKET client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &c);
        if (client_fd == INVALID_SOCKET) continue;

        int recv_size = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (recv_size > 0) {
            buffer[recv_size] = '\0';
            char *body = strstr(buffer, "\r\n\r\n");
            body = body ? body + 4 : "";

            if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET /index.html", 15) == 0) {
                serve_html(client_fd);
            } else if (strncmp(buffer, "POST /api/register", 18) == 0) {
                register_user(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/login", 15) == 0) {
                login_user(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/reset-password", 24) == 0) {
                reset_password(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/admin-login", 21) == 0) {
                char email[100], pass[100];
                get_param(body, "email", email, sizeof(email));
                get_param(body, "password", pass, sizeof(pass));
                if (strcmp(email, ADMIN_EMAIL) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
                    strcpy(json_out, "{\"status\":\"success\",\"message\":\"Admin authenticated.\"}");
                } else {
                    strcpy(json_out, "{\"status\":\"error\",\"message\":\"Unauthorized Admin Credentials.\"}");
                }
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/vehicles", 17) == 0) {
                get_all_vehicles(json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/users", 14) == 0) {
                get_all_users(json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/vehicles/add", 22) == 0) {
                add_vehicle(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/vehicles/update", 25) == 0) {
                update_vehicle(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/vehicles/delete", 25) == 0) {
                delete_vehicle(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else {
                send_response(client_fd, "text/plain", "404 Not Found");
            }
        }
        closesocket(client_fd);
    }

    free(buffer);
    free(json_out);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}