#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir _mkdir
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

// ANSI Colors
#define COLOR_RESET  "\x1b[0m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_CYAN   "\x1b[36m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_BOLD   "\x1b[1m"

typedef struct EmailUploadStatus {
    size_t bytes_sent;
    const char* full_email_message;
} EmailUploadStatus;

void trim(char* buffer) {
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r' || buffer[len - 1] == ' ')) {
        buffer[--len] = '\0';
    }
}

void Prompt(const char* label, char* buffer, size_t size) {
    fprintf(stdout, COLOR_CYAN "%s" COLOR_RESET, label);
    fgets(buffer, size, stdin);
    trim(buffer);
}

void ClearTerminal() {
    #define CLEAR_TERMINAL "\033[2J\033[H"
    fprintf(stdout, CLEAR_TERMINAL);
}

// Cross-platform get home directory
const char* get_home_dir() {
#ifdef _WIN32
    return getenv("USERPROFILE");
#else
    const char* home = getenv("HOME");
    if (!home) home = getpwuid(getuid())->pw_dir;
    return home;
#endif
}

// Ensure ~/.c-mailer/ directory exists
void ensure_config_dir_exists() {
    char config_dir[512];
    snprintf(config_dir, sizeof(config_dir), "%s/.C-Mailer", get_home_dir());
    mkdir(config_dir
#ifndef _WIN32
    , 0700
#endif
    );
}

// Get path to ~/.C-Mailer/credentials.txt
void get_credentials_path(char* path, size_t size) {
    snprintf(path, size, "%s/.C-Mailer/credentials.txt", get_home_dir());
}

static size_t payload_source(char* dest, size_t size, size_t nmemb, void* userp) {
    EmailUploadStatus* upload_ctx = (EmailUploadStatus*) userp;
    size_t buffer_size = size * nmemb;
    const char* remaining = upload_ctx->full_email_message + upload_ctx->bytes_sent;
    size_t len = strlen(remaining);

    if (len == 0) return 0;
    if (len > buffer_size) len = buffer_size;

    memcpy(dest, remaining, len);
    upload_ctx->bytes_sent += len;
    return len;
}

int load_or_create_credentials(const char* path, char* fullname, char* email, char* smtp, char* pass) {
    FILE* file = fopen(path, "a+");
    if (!file) {
        fprintf(stderr, COLOR_RED "Failed to open credentials file\n" COLOR_RESET);
        return 0;
    }

    char buffer[100];
    int count = 0;
    rewind(file);
    while (fgets(buffer, sizeof(buffer), file)) count++;

    if (count != 4) {
        fprintf(stdout, COLOR_YELLOW "⚠️  No saved user found. Let's set you up:\n" COLOR_RESET);
        Prompt("> Full Name: ", fullname, 50);
        Prompt("> Email: ", email, 50);
        Prompt("> SMTP URL (e.g. smtps://smtp.gmail.com:465): ", smtp, 100);
        Prompt("> App Password: ", pass, 100);

        fprintf(file, "%s\n%s\n%s\n%s\n", fullname, email, smtp, pass);
        fprintf(stdout, 
            COLOR_GREEN "✅ Credentials saved!"
            COLOR_CYAN " (for any change, check: %s)\n\n" COLOR_RESET
        , path);
    }

    rewind(file);
    fgets(fullname, 50, file); trim(fullname);
    fgets(email, 50, file);    trim(email);
    fgets(smtp, 100, file);    trim(smtp);
    fgets(pass, 100, file);    trim(pass);
    fclose(file);
    return 1;
}

void SendEmail(const char* from, const char* smtp, const char* password) {
    char to[100], subject[100], body[2000] = "", line[256];

    Prompt("> To: ", to, sizeof(to));
    Prompt("> Subject: ", subject, sizeof(subject));

    fprintf(stdout, COLOR_CYAN "> Write your message below (type 'save' to send):\n" COLOR_RESET);
    while (1) {
        fgets(line, sizeof(line), stdin);
        trim(line);
        if (strcmp(line, "exit") == 0) break;
        strcat(body, line);
        strcat(body, "\n");
    }

    char message[5000];
    snprintf(message, sizeof(message),
        "To: %s\r\n"
        "From: %s\r\n"
        "Subject: %s\r\n"
        "\r\n"
        "%s\r\n",
        to, from, subject, body);

    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, COLOR_RED "curl init failed\n" COLOR_RESET);
        return;
    }

    struct curl_slist* recipients = NULL;
    EmailUploadStatus upload_ctx = {0, message};

    curl_easy_setopt(curl, CURLOPT_URL, smtp);
    curl_easy_setopt(curl, CURLOPT_USERNAME, from);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
    recipients = curl_slist_append(recipients, to);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    // dev only
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, COLOR_RED "❌ Send failed: %s\n" COLOR_RESET, curl_easy_strerror(res));
    } else {
        fprintf(stdout, COLOR_GREEN "✅ Email sent successfully!\n" COLOR_RESET);
    }

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}

void HelpPage() {
    fprintf(stdout, COLOR_BOLD COLOR_CYAN
        "\nAvailable commands:\n"
        "  " COLOR_YELLOW "send  " COLOR_RESET "  - Compose and send a new email\n" 
        "  " COLOR_YELLOW "whoami" COLOR_RESET "  - Show credentials path\n" 
        "  " COLOR_YELLOW "clear " COLOR_RESET "  - Clear the terminal screen\n"
        "  " COLOR_YELLOW "help  " COLOR_RESET "  - Show this help page again\n"
        "  " COLOR_YELLOW "exit  " COLOR_RESET "  - Exit the c-mailer program\n\n"
    COLOR_RESET);
}

void PrintBanner() {
    fprintf(stdout,
        "\n                                        \n"
        "   _____      __  __       _ _            \n"
        "  / ____|    |  \\/  |     (_) |          \n"
        " | |   ______| \\  / | __ _ _| | ___ _ __ \n"
        " | |  |______| |\\/| |/ _` | | |/ _ \\ '__|\n"
        " | |____     | |  | | (_| | | |  __/ |   \n"
        "  \\_____|    |_|  |_|\\__,_|_|_|\\___|_|   \n"
        "                                         \n"
        "             By Hassan Ouammou           \n"
    );
}


int main() {
    PrintBanner();
    char credentials_path[512];
    ensure_config_dir_exists();
    get_credentials_path(credentials_path, sizeof(credentials_path));

    char fullname[50], email[50], smtp[100], pass[100];
    if (!load_or_create_credentials(credentials_path, fullname, email, smtp, pass)) return 1;

    fprintf(stdout, COLOR_BOLD COLOR_GREEN "\nWelcome, %s!\n" COLOR_RESET, fullname);
    HelpPage();

    char command[20];
    while (1) {
        fprintf(stdout, COLOR_YELLOW "c-mailer> " COLOR_RESET);
        fgets(command, sizeof(command), stdin);
        trim(command);

        if (strcmp(command, "send") == 0) {
            SendEmail(email, smtp, pass);
        } else if (strcmp(command, "whoami") == 0) {
            fprintf(stdout, COLOR_CYAN "> Full name        " COLOR_RESET "%s\n", fullname);
            fprintf(stdout, COLOR_CYAN "> Email            " COLOR_RESET "%s\n", email);
            fprintf(stdout, COLOR_CYAN "> SMTP             " COLOR_RESET "%s\n", smtp);
            fprintf(stdout, COLOR_CYAN "> App Pass         " COLOR_RESET "%s\n", pass);
            fprintf(stdout, COLOR_CYAN "> Credentials path " COLOR_RESET "%s\n", credentials_path);
        } else if (strcmp(command, "clear") == 0) {
            ClearTerminal();
        } else if (strcmp(command, "help") == 0) {
            HelpPage();
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            fprintf(stderr, COLOR_RED "Unknown command. Try 'send', 'help', or 'exit'.\n" COLOR_RESET);
        }
    }

    return 0;
}
