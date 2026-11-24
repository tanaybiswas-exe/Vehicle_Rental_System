#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 65536
#define USERS_FILE "users.txt"
#define VEHICLES_FILE "vehicles.txt"
#define RENTALS_FILE "rentals.txt"
#define ACTIVITY_LOG "activity.log"
#define CONFIG_FILE "config.txt"

#define ADMIN_EMAIL "admin@rental.com"
#define ADMIN_PASS "Admin@123"

typedef struct {
    char name[100];
    char email[100];
    char phone[30];
    char password[128];
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
    char payment_method[30];
    char status[20]; // "Active", "Returned", "Cancelled"
} Rental;

static char g_admin_email[100] = ADMIN_EMAIL;
static char g_admin_pass[100] = ADMIN_PASS;
static int g_port = PORT;

// --- Config & Hash Helpers ---

void load_config() {
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strncmp(line, "PORT=", 5) == 0) {
            int p = atoi(line + 5);
            if (p > 0 && p < 65536) g_port = p;
        } else if (strncmp(line, "ADMIN_EMAIL=", 12) == 0 && strlen(line + 12) > 0) {
            strncpy(g_admin_email, line + 12, sizeof(g_admin_email) - 1);
            g_admin_email[sizeof(g_admin_email) - 1] = '\0';
        } else if (strncmp(line, "ADMIN_PASS=", 11) == 0 && strlen(line + 11) > 0) {
            strncpy(g_admin_pass, line + 11, sizeof(g_admin_pass) - 1);
            g_admin_pass[sizeof(g_admin_pass) - 1] = '\0';
        }
    }
    fclose(fp);
}

void hash_secret(const char *raw, char *out, size_t out_len) {
    unsigned long long h1 = 1469598103934665603ULL;
    unsigned long long h2 = 1099511628211ULL;
    const char *salt = "dc_salt_v1_";
    for (int i = 0; salt[i]; i++) {
        h1 ^= (unsigned char)salt[i];
        h1 *= 1099511628211ULL;
    }
    for (int i = 0; raw && raw[i]; i++) {
        unsigned char c = (unsigned char)raw[i];
        h1 ^= c;
        h1 *= 1099511628211ULL;
        h2 = (h2 * 1315423911ULL) ^ (c + (unsigned int)i);
    }
    snprintf(out, out_len, "h1:%016llx%016llx", h1, h2);
}

int verify_secret(const char *stored, const char *candidate) {
    if (!stored || !candidate) return 0;
    if (strcmp(stored, candidate) == 0) return 1; // Backward-compatible plain match
    char hashed[128];
    hash_secret(candidate, hashed, sizeof(hashed));
    return strcmp(stored, hashed) == 0;
}

// --- Utility & Logging ---

void log_activity(const char *action, const char *details) {
    FILE *fp = fopen(ACTIVITY_LOG, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, action, details);
    fclose(fp);
}

void sanitize_json_str(const char *src, char *dst, size_t dst_len) {
    size_t j = 0;
    for (size_t i = 0; src && src[i] && j + 2 < dst_len; i++) {
        char c = src[i];
        if (c == '"' || c == '\\') {
            dst[j++] = '\\';
            dst[j++] = c;
        } else if (c == '\n' || c == '\r') {
            dst[j++] = ' ';
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

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

int parse_ymd_to_days(const char *ymd) {
    int y = 0, m = 0, d = 0;
    if (sscanf(ymd, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    if (m < 1 || m > 12 || d < 1 || d > 31) return -1;
    // Rata Die approximation sufficient for day-span comparisons
    if (m <= 2) { y -= 1; m += 12; }
    return 365 * y + y / 4 - y / 100 + y / 400 + (153 * m - 457) / 5 + d - 306;
}

void init_files() {
    FILE *fp = fopen(USERS_FILE, "a"); if (fp) fclose(fp);
    fp = fopen(RENTALS_FILE, "a"); if (fp) fclose(fp);
    fp = fopen(ACTIVITY_LOG, "a"); if (fp) fclose(fp);

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

void set_vehicle_status(const char *vehicle_id, const char *new_status) {
    FILE *vf = fopen(VEHICLES_FILE, "r");
    FILE *temp_v = fopen("temp_v.txt", "w");
    if (!vf || !temp_v) {
        if (vf) fclose(vf);
        if (temp_v) fclose(temp_v);
        return;
    }
    char line[300];
    Vehicle v;
    while (fgets(line, sizeof(line), vf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, vehicle_id) == 0) {
                strncpy(v.status, new_status, sizeof(v.status) - 1);
                v.status[sizeof(v.status) - 1] = '\0';
            }
            fprintf(temp_v, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }
    fclose(vf);
    fclose(temp_v);
    remove(VEHICLES_FILE);
    rename("temp_v.txt", VEHICLES_FILE);
}

int has_active_overlap(const char *vehicle_id, const char *start_date, const char *end_date) {
    int ns = parse_ymd_to_days(start_date);
    int ne = parse_ymd_to_days(end_date);
    if (ns < 0 || ne < 0) return 0;

    FILE *rf = fopen(RENTALS_FILE, "r");
    if (!rf) return 0;
    char line[500];
    Rental r;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%29[^|]|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                   &r.total_cost, r.payment_method, r.status) == 9) {
            if (strcmp(r.vehicle_id, vehicle_id) == 0 && strcmp(r.status, "Active") == 0) {
                int os = parse_ymd_to_days(r.start_date);
                int oe = parse_ymd_to_days(r.end_date);
                if (os >= 0 && oe >= 0 && ns <= oe && ne >= os) {
                    fclose(rf);
                    return 1;
                }
            }
        }
    }
    fclose(rf);
    return 0;
}

// --- User Operations ---

int email_exists(const char *email) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) return 0;
    char line[700];
    User u;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%127[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) { fclose(fp); return 1; }
        }
    }
    fclose(fp);
    return 0;
}

void register_user(const char *body, char *response_json) {
    User u;
    char raw_pass[128], raw_sec_a[100];
    get_param(body, "name", u.name, sizeof(u.name));
    get_param(body, "email", u.email, sizeof(u.email));
    get_param(body, "phone", u.phone, sizeof(u.phone));
    get_param(body, "password", raw_pass, sizeof(raw_pass));
    get_param(body, "sec_q", u.sec_q, sizeof(u.sec_q));
    get_param(body, "sec_a", raw_sec_a, sizeof(raw_sec_a));
    u.failed_attempts = 0;

    if (strlen(u.name) == 0 || strlen(u.phone) == 0 || strlen(u.sec_q) == 0 || strlen(raw_sec_a) == 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"All fields are required.\"}");
        return;
    }
    if (!validate_email(u.email)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid email format.\"}");
        return;
    }
    if (strcmp(u.email, g_admin_email) == 0 || strcmp(u.email, ADMIN_EMAIL) == 0 || email_exists(u.email)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Email already registered or restricted.\"}");
        return;
    }
    if (!validate_password(raw_pass)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Password must be 6+ chars with 1 letter, 1 number & 1 special symbol.\"}");
        return;
    }

    hash_secret(raw_pass, u.password, sizeof(u.password));
    hash_secret(raw_sec_a, u.sec_a, sizeof(u.sec_a));

    FILE *fp = fopen(USERS_FILE, "a");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"File error saving user.\"}");
        return;
    }
    fprintf(fp, "%s|%s|%s|%s|%s|%s|0\n", u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a);
    fclose(fp);
    log_activity("USER_REGISTER", u.email);
    strcpy(response_json, "{\"status\":\"success\",\"message\":\"Registration successful! Please login.\"}");
}

void login_user(const char *body, char *response_json) {
    char email[100], password[128];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "password", password, sizeof(password));

    FILE *fp = fopen(USERS_FILE, "r");
    FILE *temp = fopen("temp_users.txt", "w");
    if (!fp || !temp) {
        if (fp) fclose(fp); if (temp) fclose(temp);
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[700];
    User u;
    int found = 0;
    strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid email or password.\"}");

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%127[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                found = 1;
                if (u.failed_attempts >= 5) {
                    snprintf(response_json, BUFFER_SIZE,
                             "{\"status\":\"locked\",\"message\":\"Account locked after 5 failed attempts.\",\"sec_q\":\"%s\"}", u.sec_q);
                } else if (verify_secret(u.password, password)) {
                    u.failed_attempts = 0;
                    log_activity("USER_LOGIN_SUCCESS", u.email);
                    snprintf(response_json, BUFFER_SIZE,
                             "{\"status\":\"success\",\"user\":{\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\"}}",
                             u.name, u.email, u.phone);
                } else {
                    u.failed_attempts++;
                    log_activity("USER_LOGIN_FAILED", u.email);
                    if (u.failed_attempts >= 5) {
                        snprintf(response_json, BUFFER_SIZE,
                                 "{\"status\":\"locked\",\"message\":\"5 incorrect attempts! Answer security question to reset password.\",\"sec_q\":\"%s\"}", u.sec_q);
                    } else {
                        snprintf(response_json, BUFFER_SIZE,
                                 "{\"status\":\"error\",\"message\":\"Incorrect password! Attempts left: %d\"}", 5 - u.failed_attempts);
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

    if (!found) strcpy(response_json, "{\"status\":\"error\",\"message\":\"No account found with this email.\"}");
}

void update_user_profile(const char *body, char *response_json) {
    char email[100], new_name[100], new_phone[30], current_pass[128], new_pass[128];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "name", new_name, sizeof(new_name));
    get_param(body, "phone", new_phone, sizeof(new_phone));
    get_param(body, "current_pass", current_pass, sizeof(current_pass));
    get_param(body, "new_pass", new_pass, sizeof(new_pass));

    FILE *fp = fopen(USERS_FILE, "r");
    FILE *temp = fopen("temp_users.txt", "w");
    if (!fp || !temp) {
        if (fp) fclose(fp); if (temp) fclose(temp);
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[700];
    User u;
    int updated = 0, pass_wrong = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%127[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0) {
                if (verify_secret(u.password, current_pass)) {
                    strcpy(u.name, new_name);
                    strcpy(u.phone, new_phone);
                    if (strlen(new_pass) > 0 && validate_password(new_pass)) {
                        hash_secret(new_pass, u.password, sizeof(u.password));
                    }
                    updated = 1;
                    log_activity("PROFILE_UPDATE", email);
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
    char email[100], sec_a[100], new_pass[128], confirm_pass[128];
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

    char line[700];
    User u;
    int updated = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%127[^|]|%199[^|]|%99[^|]|%d",
                   u.name, u.email, u.phone, u.password, u.sec_q, u.sec_a, &u.failed_attempts) == 7) {
            if (strcmp(u.email, email) == 0 && verify_secret(u.sec_a, sec_a)) {
                hash_secret(new_pass, u.password, sizeof(u.password));
                u.failed_attempts = 0;
                updated = 1;
                log_activity("PASSWORD_RESET", email);
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
    char line[700], item[450];
    User u;
    int first = 1;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%99[^|]|%99[^|]|%29[^|]|%127[^|]|%199[^|]|%99[^|]|%d",
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
            if (strcmp(v.id, id) == 0) { fclose(fp); return 1; }
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
    log_activity("VEHICLE_ADD", v.id);
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
        log_activity("VEHICLE_UPDATE", id);
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
                continue;
            }
            fprintf(temp, "%s|%s|%s|%.2f|%s\n", v.id, v.name, v.type, v.price, v.status);
        }
    }
    fclose(fp);
    fclose(temp);
    remove(VEHICLES_FILE);
    rename("temp_vehicles.txt", VEHICLES_FILE);

    if (found) {
        log_activity("VEHICLE_DELETE", id);
        strcpy(response_json, "{\"status\":\"success\",\"message\":\"Vehicle deleted permanently.\"}");
    } else {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle ID not found.\"}");
    }
}

// --- Rental Booking, Payment & Lifecycle APIs ---

void book_vehicle(const char *body, char *response_json) {
    char email[100], vehicle_id[20], start_date[30], end_date[30], payment_method[30], cost_str[30];
    get_param(body, "email", email, sizeof(email));
    get_param(body, "vehicle_id", vehicle_id, sizeof(vehicle_id));
    get_param(body, "start_date", start_date, sizeof(start_date));
    get_param(body, "end_date", end_date, sizeof(end_date));
    get_param(body, "payment_method", payment_method, sizeof(payment_method));
    get_param(body, "total_cost", cost_str, sizeof(cost_str));

    if (strlen(start_date) == 0 || strlen(end_date) == 0 || strcmp(end_date, start_date) < 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"End date must be on or after start date.\"}");
        return;
    }

    if (has_active_overlap(vehicle_id, start_date, end_date)) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Selected dates overlap with an active booking for this vehicle.\"}");
        return;
    }

    double total_cost = atof(cost_str);
    if (total_cost <= 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid rental amount.\"}");
        return;
    }
    if (strlen(payment_method) == 0) strcpy(payment_method, "Cash on Delivery");

    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[300];
    Vehicle v;
    int v_found = 0;
    char v_name[100] = "";

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%19[^|]|%99[^|]|%49[^|]|%lf|%29[^|]",
                   v.id, v.name, v.type, &v.price, v.status) == 5) {
            if (strcmp(v.id, vehicle_id) == 0) {
                if (strcmp(v.status, "Available") != 0) {
                    fclose(fp);
                    strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle is already rented or under maintenance.\"}");
                    return;
                }
                v_found = 1;
                strcpy(v_name, v.name);
                break;
            }
        }
    }
    fclose(fp);

    if (!v_found) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Vehicle not found.\"}");
        return;
    }

    set_vehicle_status(vehicle_id, "Rented");

    char rental_id[40];
    snprintf(rental_id, sizeof(rental_id), "RNT-%u", (unsigned int)time(NULL));

    FILE *rf = fopen(RENTALS_FILE, "a");
    if (!rf) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Could not save booking.\"}");
        return;
    }
    fprintf(rf, "%s|%s|%s|%s|%s|%s|%.2f|%s|Active\n", rental_id, email, vehicle_id, v_name, start_date, end_date, total_cost, payment_method);
    fclose(rf);

    log_activity("VEHICLE_BOOKED_PAID", rental_id);
    snprintf(response_json, BUFFER_SIZE,
             "{\"status\":\"success\",\"message\":\"Payment successful & vehicle booked! ID: %s\",\"rental_id\":\"%s\"}",
             rental_id, rental_id);
}

void write_rentals_json(const char *filter_email, char *response_json) {
    FILE *fp = fopen(RENTALS_FILE, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"success\",\"rentals\":[]}");
        return;
    }

    strcpy(response_json, "{\"status\":\"success\",\"rentals\":[");
    char line[500], item[650];
    Rental r;
    int first = 1;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%29[^|]|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                   &r.total_cost, r.payment_method, r.status) == 9) {
            if (!filter_email || strcmp(r.user_email, filter_email) == 0) {
                if (!first) strcat(response_json, ",");
                snprintf(item, sizeof(item),
                         "{\"rental_id\":\"%s\",\"user_email\":\"%s\",\"vehicle_id\":\"%s\",\"vehicle_name\":\"%s\",\"start_date\":\"%s\",\"end_date\":\"%s\",\"total_cost\":%.2f,\"payment_method\":\"%s\",\"status\":\"%s\"}",
                         r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                         r.total_cost, r.payment_method, r.status);
                strcat(response_json, item);
                first = 0;
            }
        }
    }
    strcat(response_json, "]}");
    fclose(fp);
}

void get_user_rentals(const char *body, char *response_json) {
    char email[100];
    get_param(body, "email", email, sizeof(email));
    write_rentals_json(email, response_json);
}

void extend_rental(const char *body, char *response_json) {
    char rental_id[30], new_end_date[30], extra_cost_str[30];
    get_param(body, "rental_id", rental_id, sizeof(rental_id));
    get_param(body, "new_end_date", new_end_date, sizeof(new_end_date));
    get_param(body, "extra_cost", extra_cost_str, sizeof(extra_cost_str));

    double extra_cost = atof(extra_cost_str);
    int new_end_day = parse_ymd_to_days(new_end_date);
    if (new_end_day < 0 || extra_cost < 0) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Invalid extension date or cost.\"}");
        return;
    }

    FILE *rf = fopen(RENTALS_FILE, "r");
    FILE *temp_r = fopen("temp_r.txt", "w");
    if (!rf || !temp_r) {
        if (rf) fclose(rf);
        if (temp_r) fclose(temp_r);
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[500];
    Rental r;
    int updated = 0, invalid_date = 0;

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%29[^|]|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                   &r.total_cost, r.payment_method, r.status) == 9) {
            if (strcmp(r.rental_id, rental_id) == 0 && strcmp(r.status, "Active") == 0) {
                int old_end_day = parse_ymd_to_days(r.end_date);
                if (old_end_day >= 0 && new_end_day <= old_end_day) {
                    invalid_date = 1;
                } else {
                    strncpy(r.end_date, new_end_date, sizeof(r.end_date) - 1);
                    r.end_date[sizeof(r.end_date) - 1] = '\0';
                    r.total_cost += extra_cost;
                    updated = 1;
                }
            }
            fprintf(temp_r, "%s|%s|%s|%s|%s|%s|%.2f|%s|%s\n",
                    r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                    r.total_cost, r.payment_method, r.status);
        }
    }
    fclose(rf);
    fclose(temp_r);
    remove(RENTALS_FILE);
    rename("temp_r.txt", RENTALS_FILE);

    if (invalid_date) {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"New end date must be after current end date.\"}");
    } else if (updated) {
        log_activity("RENTAL_EXTENDED", rental_id);
        strcpy(response_json, "{\"status\":\"success\",\"message\":\"Trip extended successfully!\"}");
    } else {
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Active booking not found.\"}");
    }
}

void update_rental_status(const char *body, const char *target_status, const char *log_tag, const char *success_msg, char *response_json) {
    char rental_id[30], vehicle_id[20], note[120];
    get_param(body, "rental_id", rental_id, sizeof(rental_id));
    get_param(body, "vehicle_id", vehicle_id, sizeof(vehicle_id));
    get_param(body, "note", note, sizeof(note));

    FILE *rf = fopen(RENTALS_FILE, "r");
    FILE *temp_r = fopen("temp_r.txt", "w");
    if (!rf || !temp_r) {
        if (rf) fclose(rf);
        if (temp_r) fclose(temp_r);
        strcpy(response_json, "{\"status\":\"error\",\"message\":\"Database error.\"}");
        return;
    }

    char line[500];
    Rental r;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%29[^|]|%19[^|]",
                   r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                   &r.total_cost, r.payment_method, r.status) == 9) {
            if (strcmp(r.rental_id, rental_id) == 0) {
                strncpy(r.status, target_status, sizeof(r.status) - 1);
                r.status[sizeof(r.status) - 1] = '\0';
            }
            fprintf(temp_r, "%s|%s|%s|%s|%s|%s|%.2f|%s|%s\n",
                    r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                    r.total_cost, r.payment_method, r.status);
        }
    }
    fclose(rf);
    fclose(temp_r);
    remove(RENTALS_FILE);
    rename("temp_r.txt", RENTALS_FILE);

    set_vehicle_status(vehicle_id, "Available");

    char log_detail[200];
    if (strlen(note) > 0) {
        snprintf(log_detail, sizeof(log_detail), "%s (Note: %s)", rental_id, note);
    } else {
        snprintf(log_detail, sizeof(log_detail), "%s", rental_id);
    }
    log_activity(log_tag, log_detail);
    snprintf(response_json, BUFFER_SIZE, "{\"status\":\"success\",\"message\":\"%s\"}", success_msg);
}

void get_analytics(char *response_json) {
    double total_revenue = 0.0;
    double rev_bkash = 0.0, rev_nagad = 0.0, rev_card = 0.0, rev_cod = 0.0;
    int active_rentals = 0;
    int total_vehicles = 0;

    FILE *rf = fopen(RENTALS_FILE, "r");
    if (rf) {
        char line[500];
        Rental r;
        while (fgets(line, sizeof(line), rf)) {
            if (sscanf(line, "%29[^|]|%99[^|]|%19[^|]|%99[^|]|%29[^|]|%29[^|]|%lf|%29[^|]|%19[^|]",
                       r.rental_id, r.user_email, r.vehicle_id, r.vehicle_name, r.start_date, r.end_date,
                       &r.total_cost, r.payment_method, r.status) == 9) {
                if (strcmp(r.status, "Active") == 0) active_rentals++;
                if (strcmp(r.status, "Cancelled") != 0) {
                    total_revenue += r.total_cost;
                    if (strcmp(r.payment_method, "bKash") == 0) rev_bkash += r.total_cost;
                    else if (strcmp(r.payment_method, "Nagad") == 0) rev_nagad += r.total_cost;
                    else if (strcmp(r.payment_method, "Credit Card") == 0) rev_card += r.total_cost;
                    else rev_cod += r.total_cost;
                }
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
             "{\"status\":\"success\",\"revenue\":%.2f,\"active_rentals\":%d,\"total_vehicles\":%d,"
             "\"gateway\":{\"bKash\":%.2f,\"Nagad\":%.2f,\"Credit Card\":%.2f,\"Cash on Delivery\":%.2f}}",
             total_revenue, active_rentals, total_vehicles, rev_bkash, rev_nagad, rev_card, rev_cod);
}

void get_activity_logs(char *response_json) {
    FILE *fp = fopen(ACTIVITY_LOG, "r");
    if (!fp) {
        strcpy(response_json, "{\"status\":\"success\",\"logs\":[]}");
        return;
    }

    char lines[50][320];
    int count = 0;
    char buf[320];
    while (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;
        char clean[300];
        sanitize_json_str(buf, clean, sizeof(clean));

        if (count < 50) {
            strcpy(lines[count++], clean);
        } else {
            for (int k = 1; k < 50; k++) strcpy(lines[k - 1], lines[k]);
            strcpy(lines[49], clean);
        }
    }
    fclose(fp);

    strcpy(response_json, "{\"status\":\"success\",\"logs\":[");
    for (int i = count - 1; i >= 0; i--) {
        if (i < count - 1) strcat(response_json, ",");
        strcat(response_json, "\"");
        strcat(response_json, lines[i]);
        strcat(response_json, "\"");
    }
    strcat(response_json, "]}");
}

void export_csv(SOCKET client) {
    FILE *rf = fopen(RENTALS_FILE, "r");
    if (!rf) {
        char header[256];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nNo data");
        send(client, header, (int)strlen(header), 0);
        return;
    }
    char csv_data[16384] = "RentalID,UserEmail,VehicleID,VehicleName,StartDate,EndDate,TotalCost,PaymentMethod,Status\n";
    char line[500];
    while (fgets(line, sizeof(line), rf)) {
        for (int i = 0; line[i]; i++) if (line[i] == '|') line[i] = ',';
        if (strlen(csv_data) + strlen(line) + 1 < sizeof(csv_data)) {
            strcat(csv_data, line);
        }
    }
    fclose(rf);

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/csv; charset=UTF-8\r\n"
             "Content-Disposition: attachment; filename=\"rentals_report.csv\"\r\n"
             "Connection: close\r\n\r\n");
    send(client, header, (int)strlen(header), 0);
    send(client, csv_data, (int)strlen(csv_data), 0);
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

void serve_file(SOCKET client, const char *filename, const char *content_type) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        char err[256];
        snprintf(err, sizeof(err), "Error: %s not found.", filename);
        send_response(client, "text/plain", err);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = (char *)malloc(fsize + 1);
    fread(data, 1, fsize, fp);
    fclose(fp);
    data[fsize] = '\0';

    send_response(client, content_type, data);
    free(data);
}

int main() {
    load_config();
    init_files();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) return 1;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons((u_short)g_port);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) return 1;
    listen(server_fd, 10);

    printf("==================================================\n");
    printf("  DriveCore C Web Server Running at http://localhost:%d\n", g_port);
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
                serve_file(client_fd, "index.html", "text/html");
            } else if (strncmp(buffer, "GET /style.css", 14) == 0) {
                serve_file(client_fd, "style.css", "text/css");
            } else if (strncmp(buffer, "GET /app.js", 11) == 0) {
                serve_file(client_fd, "app.js", "application/javascript");
            } else if (strncmp(buffer, "GET /api/export-csv", 19) == 0) {
                export_csv(client_fd);
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
                if ((strcmp(email, g_admin_email) == 0 && strcmp(pass, g_admin_pass) == 0) ||
                    (strcmp(email, ADMIN_EMAIL) == 0 && strcmp(pass, ADMIN_PASS) == 0)) {
                    log_activity("ADMIN_LOGIN", email);
                    strcpy(json_out, "{\"status\":\"success\",\"message\":\"Admin authenticated.\"}");
                } else {
                    log_activity("ADMIN_LOGIN_FAILED", email);
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
            } else if (strncmp(buffer, "POST /api/rentals/extend", 24) == 0) {
                extend_rental(body, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/rentals/return", 24) == 0) {
                update_rental_status(body, "Returned", "VEHICLE_RETURNED",
                                     "Vehicle returned successfully. Status updated to Available.", json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "POST /api/rentals/cancel", 24) == 0) {
                update_rental_status(body, "Cancelled", "BOOKING_CANCELLED",
                                     "Booking cancelled & vehicle released to fleet.", json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/admin/rentals", 22) == 0) {
                write_rentals_json(NULL, json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/admin/analytics", 24) == 0) {
                get_analytics(json_out);
                send_response(client_fd, "application/json", json_out);
            } else if (strncmp(buffer, "GET /api/admin/logs", 19) == 0) {
                get_activity_logs(json_out);
                send_response(client_fd, "application/json", json_out);
            } else {
                send_response(client_fd, "text/plain", "404 Not Found");
            }
        }
        closesocket(client_fd);
    }
    return 0;
}
