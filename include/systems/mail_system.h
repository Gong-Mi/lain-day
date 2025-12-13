#ifndef MAIL_SYSTEM_H
#define MAIL_SYSTEM_H

#include "game_types.h" // For StringID and MAX_..._LENGTH

#define MAX_EMAIL_BODY_LINES 20 // Increased to allow more lines for email body
#define MAX_EMAIL_BODY_LINE_LENGTH MAX_LINE_LENGTH

typedef struct {
    int id;
    char filename[MAX_PATH_LENGTH]; // e.g., "001_chisa.eml,U"
    char sender[MAX_NAME_LENGTH];
    char subject[MAX_DESC_LENGTH];
    char body[MAX_EMAIL_BODY_LINES][MAX_EMAIL_BODY_LINE_LENGTH];
    int body_line_count;
    bool is_read;
    bool is_deleted; // New field
} Email;

#define MAX_EMAILS 20 // Increased max emails to 20

typedef struct {
    Email emails[MAX_EMAILS];
    int email_count;
} Mailbox;

// Function declarations (to be implemented)
void mail_system_init(Mailbox* mailbox);
void mail_system_load_emails(Mailbox* mailbox, const char* maildir_path);
void mail_system_display_list(const Mailbox* mailbox);
void mail_system_display_email(const Mailbox* mailbox, int email_index);
void mail_system_mark_as_read(Mailbox* mailbox, int email_id);
void mail_system_delete_email(Mailbox* mailbox, const char* maildir_path, int email_id); // New function
void mail_system_cleanup(Mailbox* mailbox); // To save state or free resources if needed

#endif // MAIL_SYSTEM_H
