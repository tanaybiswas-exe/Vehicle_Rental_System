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
#define RENTALS_FILE "rentals.txt"

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
    char status[30];
} Vehicle;

typedef struct {
    char rental_id[30];
    char user_email[100];
    char vehicle_id[20];
    char vehicle_name[100];
    char start_date[30];
    char end_date[30];
    double total_cost;
    char status[20]; // "Active" or "Returned"
} Rental;

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

void init_files() {
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp) fclose(fp);

    fp = fopen(RENTALS_FILE, "a");
    if (fp) fclose(fp);

    fp = fopen(VEHICLES_FILE, "r");
    if (!fp) {
        fp = fopen(VEHICLES_FILE, "w");
        if (fp) {
            fprintf(fp, "V101|Toyota Corolla|Sedan|3500.00|Available\n");
            fprintf(fp, "V102|Honda Civic|Sedan|4200.00|Available\n");
            fprintf(fp, "V103|Mitsubishi Pajero|SUV|7500.00|Available\n");
            fprintf(fp, "V104|Yamaha R15|Bike|1500.00|Available\n");
            fclose(fp);
        }
    } else {
        fclose(fp);
    }
}

// --- User Operations ---

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

void update_user_profile(const char *body, char *response_json) {
    char email[100], new_name[100], new_phone[30], current_pass[100], new_pass[100];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "name", new_name, sizeof(new_name));
    get_param(body, "phone", new_phone, sizeof(new_phone));
    get_param(body, "current_pass", current_pass, sizeof(current_pass));
    get_param(body, "new_pass", new_pass, sizeof(new_pass));

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
    int updated = 0, pass_wrong = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%99[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                if (strcmp(u.password, current_pass) == 0) {
                    strcpy(u.name, new_name);
                    strcpy(u.phone, new_phone);
                    if (strlen(new_pass) > 0) {
                        if (validate_password(new_pass)) strcpy(u.password, new_pass);
                    }
                    updated = 1;
                    snprintf(response_json, BUFFER_SIZE,
                             "{\"status\":\"success\",\"message\":\"Profile updated successfully!\",\"user\":{\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\"}}",
                             u.name, u.email, u.phone);
                } else {
                    pass_wrong = 1;
                }
            }
            fprintf(temp, "%s|%s|%s|%s|%s|%s|%d\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, u.failed_attempts);
        }
    }
    fclose(fp);
    fclose(temp);
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);

    if (pass_wrong) strcpy(response_json, "{\"status\":\"error\",\"message\":\"Incorrect current password.\"}");
    else if (!updated) strcpy(response_json, "{\"status\":\"error\",\"message\":\"User not found.\"}");
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
            if (strcmp(u.email, email) == 0 && strcmp(u.sec_a, sec_a) == 0) {
                strcpy(u.password, new_pass);
                u.failed_attempts = 0;
                updated = 1;
            }
            fprintf(temp, "%s|%s|%s|%s|%s|%s|%d\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, u.failed_attempts);
        }
    }
    fclose(fp);
    fclose(temp);
    remove(USERS_FILE);
    rename("temp_users.txt", USERS_FILE);

    if (updated) strcpy(response_json, "{\"status\":\"success\",\"message\":\"Password reset successful!\"}");
    else strcpy(response_json, "{\"status\":\"error\",\"message\":\"Incorrect security answer.\"}");
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
            snprintf(item, sizeof(item), "{\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\"}", u.name, u.email, u.phone);
            strcat(response_json, item);
            first = 0;
        }
    }
    strcat(response_json, "]}");
    fclose(fp);
}

// --- Vehicle & Rental Operations ---

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

    if (vehicle_id_exists(v.id)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID already exists.\"}");
        return;
    }

    FILE *fp = fopen(VEHICLES_FILE, "a");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error.\"}");
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

    if (found) strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle updated successfully.\"}");
    else strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID not found.\"}");
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
                continue;
            }
            fprintf(temp, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }
    fclose(fp);
    fclose(temp);
    remove(VEHICLES_FILE);
    rename("temp_vehicles.txt", VEHICLES_FILE);

    if (found) strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle deleted permanently.\"}");
    else strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID not found.\"}");
}

// --- Rental Booking APIs ---

void book_vehicle(const char *body, char *response_json) {
    char email[100], vehicle_id[20], start_date[30], end_date[30];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "vehicle_id", vehicle_id, sizeof(vehicle_id));
    get_param(body, "start_date", start_date, sizeof(start_date));
    get_param(body, "end_date", end_date, sizeof(end_date));

    // Find vehicle price & name
    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[300];
    Vehicle v;
    int v_found = 0;
    char v_name[100] = "";
    double v_price = 0.0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, vehicle_id) == 0) {
                if (strcmp(v.status, "Available") != 0) {
                    fclose(fp);
                    strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle is already rented.\"}");
                    return;
                }
                v_found = 1;
                strcpy(v_name, v.name);
                v_price = v.price;
                break;
            }
        }
    }
    fclose(fp);

    if (!v_found) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle not found.\"}");
        return;
    }

    // Mark vehicle as Rented
    FILE *vf = fopen(VEHICLES_FILE, "r");
    FILE *temp_v = fopen("temp_v.txt", "w");
    while (fgets(line, sizeof(line), vf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, vehicle_id) == 0) {
                strcpy(v.status, "Rented");
            }
            fprintf(temp_v, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }
    fclose(vf);
    fclose(temp_v);
    remove(VEHICLES_FILE);
    rename("temp_v.txt", VEHICLES_FILE);

    // Save rental record
    char rental_id[40];
    snprintf(rental_id, sizeof(rental_id), "RNT-%u", (unsigned int)time(NULL));

    FILE *rf = fopen(RENTALS_FILE, "a");
    if (!rf) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Could not save booking.\"}");
        return;
    }
    fprintf(rf, "%s|%s|%s|%s|%s|%s|%.2f|Active\n", rental_id, email, vehicle_id, v_name, start_date, end_date, v_price);
    fclose(rf);

    snprintf(response_json, BUFFER_SIZE, "{\"status\":\"success\",\"message\":\"Vehicle booked successfully! Rental ID: %s\"}", rental_id);
}

void get_user_rentals(const char *body, char *response_json) {
    char email[100];
    get_param(body, "email", email, sizeof(email));

    FILE *fp = fopen(RENTALS_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"success\",\"rentals\":[]}");
        return;
    }

    strcpy(response_json, "{\"status\":\"success\",\"rentals\":[");
    char line[400], item[500];
    Rental r;
    int first = 1;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date, &r.total_cost, r.status) == 8) {
            if (strcmp(r.user_email, email) == 0) {
                if (!first) strcat(response_json, ",");
                snprintf(item, sizeof(item),
                         "{\"rental_id\":\"%s\",\"vehicle_id\":\"%s\",\"vehicle_name\":\"%s\",\"start_date\":\"%s\",\"end_date\":\"%s\",\"total_cost\":%.2f,\"status\":\"%s\"}",
                         r.rental_id, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date, r.total_cost, r.status);
                strcat(response_json, item);
                first = 0;
            }
        }
    }
    strcat(response_json, "]}");
    fclose(fp);
}

void return_vehicle(const char *body, char *response_json) {
    char rental_id[30], vehicle_id[20];
    get_param(body, "rental_id", rental_id, sizeof(rental_id));
    get_param(body, "vehicle_id", vehicle_id, sizeof(vehicle_id));

    // Update rental status to Returned
    FILE *rf = fopen(RENTALS_FILE, "r");
    FILE *temp_r = fopen("temp_r.txt", "w");
    char line[400];
    Rental r;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date, &r.total_cost, r.status) == 8) {
            if (strcmp(r.rental_id, rental_id) == 0) {
                strcpy(r.status, "Returned");
            }
            fprintf(temp_r, "%s|%s|%s|%s|%s|%s|%.2f|%s\n", r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date, r.total_cost, r.status);
        }
    }
    fclose(rf);
    fclose(temp_r);
    remove(RENTALS_FILE);
    rename("temp_r.txt", RENTALS_FILE);

    // Make vehicle Available again
    FILE *vf = fopen(VEHICLES_FILE, "r");
    FILE *temp_v = fopen("temp_v.txt", "w");
    Vehicle v;
    while (fgets(line, sizeof(line), vf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, vehicle_id) == 0) {
                strcpy(v.status, "Available");
            }
            fprintf(temp_v, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }
    fclose(vf);
    fclose(temp_v);
    remove(VEHICLES_FILE);
    rename("temp_v.txt", VEHICLES_FILE);

    strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle returned successfully. Status updated to Available.\"}");
}

void get_analytics(char *response_json) {
    double total_revenue = 0.0;
    int active_rentals = 0;
    int total_vehicles = 0;

    FILE *rf = fopen(RENTALS_FILE, "r");
    if (rf) {
        char line[400];
        Rental r;
        while (fgets(line, sizeof(line), rf)) {
            if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%19[^|]",
                       r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date, &r.total_cost, r.status) == 8) {
                if (strcmp(r.status, "Active") == 0) active_rentals++;
                total_revenue += r.total_cost;
            }
        }
        fclose(rf);
    }

    FILE *vf = fopen(VEHICLES_FILE, "r");
    if (vf) {
        char line[300];
        Vehicle v;
        while (fgets(line, sizeof(line), vf)) {
            if (sscanf(line, "%19[^|]|", v.id) == 1) total_vehicles++;
        }
        fclose(vf);
    }

    snprintf(response_json, BUFFER_SIZE,
             "{\"status\":\"success\",\"revenue\":%.2f,\"active_rentals\":%d,\"total_vehicles\":%d}",
             total_revenue, active_rentals, total_vehicles);
}

// --- HTTP Helpers ---

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
        send_response(client, "text/plain", "Error: index.html not found.");
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

int main() {
    init_files();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) return 1;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) return 1;
    listen(server_fd, 10);

    printf("==================================================\n");
    printf("  DriveCore C Web Server Running at http://localhost:%d\n", PORT);
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
            } else if (strncmp(buffer, "POST /api/update-profile", 24) == 0) {
                update_user_profile(body, json_out);
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
                    strcpy(json_out, "{\"status\":\"error\",\"message\":\"Unauthorized.\"}");
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
            } else if (strncmp(buffer, "POST /api/rentals/book", 22) == 0) {
                book_vehicle(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/rentals/user", 22) == 0) {
                get_user_rentals(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/rentals/return", 24) == 0) {
                return_vehicle(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/admin/analytics", 24) == 0) {
                get_analytics(json_out);
                send_response(client_fd, "application/json", json_out);
            } else {
                send_response(client_fd, "text/plain", "404 Not Found");
            }
        }
        closesocket(client_fd);
    }
    return 0;
}
