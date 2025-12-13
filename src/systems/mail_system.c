// This file implements the mail system for the embedded NAVI.

#include "../../include/systems/mail_system.h"
#include "../../include/ansi_colors.h" // For coloring output
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> // For directory operations
#include <ctype.h>  // For isspace
#include <sys/stat.h> // For stat
#include <unistd.h> // For rename

// --- Static Helper Functions ---
static void trim_whitespace(char *s) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*s)) s++;

    if(*s == 0)  // All spaces?
        return;

    // Trim trailing space
    end = s + strlen(s) - 1;
    while(end > s && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;
}

static void parse_email_filename(const char* filename, Email* email) {
    // Expected format: NNN_sender.eml,U or NNN_sender.eml,R or NNN_sender.eml,D
    // Example: "001_chisa.eml,U" -> id=1, sender="chisa"

    // Copy original filename
    strncpy(email->filename, filename, sizeof(email->filename) - 1);
    email->filename[sizeof(email->filename) - 1] = '\0';

    char temp_filename[MAX_PATH_LENGTH];
    strncpy(temp_filename, filename, sizeof(temp_filename) - 1);
    temp_filename[sizeof(temp_filename) - 1] = '\0';

    // Extract ID
    char* id_part = strtok(temp_filename, "_");
    if (id_part) {
        email->id = atoi(id_part);
    } else {
        email->id = 0; // Invalid ID
    }

    // Extract Sender
    char* sender_part = strtok(NULL, "."); // Get part before .eml
    if (sender_part) {
        strncpy(email->sender, sender_part, sizeof(email->sender) - 1);
        email->sender[sizeof(email->sender) - 1] = '\0';
    } else {
        strcpy(email->sender, "Unknown");
    }
    
    // Check for status (,U or ,R or ,D)
    const char* status_ptr = strrchr(filename, ',');
    if (status_ptr && *(status_ptr + 1)) {
        char status_char = *(status_ptr + 1);
        email->is_read = (status_char == 'R');
        email->is_deleted = (status_char == 'D'); // New: Check for deleted status
    } else {
        email->is_read = false; // Default to unread if no status found
        email->is_deleted = false; // Default to not deleted
    }

    // Default subject, will be overwritten if found in email body
    strcpy(email->subject, "(No Subject)");
}


// --- Public Functions ---

void mail_system_init(Mailbox* mailbox) {
    if (mailbox) {
        mailbox->email_count = 0;
        for (int i = 0; i < MAX_EMAILS; ++i) {
            mailbox->emails[i].id = 0;
            mailbox->emails[i].is_read = false;
            mailbox->emails[i].is_deleted = false; // Initialize new field
            mailbox->emails[i].body_line_count = 0;
            mailbox->emails[i].sender[0] = '\0';
            mailbox->emails[i].subject[0] = '\0';
            mailbox->emails[i].filename[0] = '\0';
        }
    }
}

void mail_system_load_emails(Mailbox* mailbox, const char* maildir_path) {
    if (!mailbox || !maildir_path) return;

    DIR *dir;
    struct dirent *ent;
    char filepath[MAX_PATH_LENGTH];
    FILE *fp;
    char line_buffer[MAX_LINE_LENGTH];
    Email current_email;

    mail_system_init(mailbox); // Clear existing emails

    // Check if maildir_path exists and is a directory
    struct stat st;
    if (stat(maildir_path, &st) == -1 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "ERROR: Maildir directory not found or not a directory: %s\n", maildir_path);
        return;
    }


    if ((dir = opendir(maildir_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            // Check for regular files ending with .eml,U or .eml,R or .eml,D
            if (ent->d_type == DT_REG && strstr(ent->d_name, ".eml") != NULL && strchr(ent->d_name, ',') != NULL) {
                if (mailbox->email_count >= MAX_EMAILS) {
                    fprintf(stderr, "WARNING: Max emails reached, skipping %s\n", ent->d_name);
                    break;
                }

                snprintf(filepath, sizeof(filepath), "%s/%s", maildir_path, ent->d_name);

                parse_email_filename(ent->d_name, &current_email);

                // New: Only load emails that are NOT deleted
                if (current_email.is_deleted) {
                    continue; 
                }
                
                fp = fopen(filepath, "r");
                if (fp != NULL) {
                    bool in_body = false;
                    current_email.body_line_count = 0;

                    // Ensure current_email.subject is cleared if parse_email_filename set a default
                    // as we prefer the header subject.
                    current_email.subject[0] = '\0'; 

                    while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL) {
                        char trimmed_line[MAX_LINE_LENGTH];
                        strncpy(trimmed_line, line_buffer, sizeof(trimmed_line) -1);
                        trimmed_line[sizeof(trimmed_line)-1] = '\0';
                        trim_whitespace(trimmed_line);

                        if (!in_body) {
                            if (current_email.subject[0] == '\0' && strncmp(trimmed_line, "Subject:", 8) == 0) {
                                strncpy(current_email.subject, trimmed_line + 8, sizeof(current_email.subject) - 1);
                                trim_whitespace(current_email.subject);
                            } else if (current_email.sender[0] == '\0' && strncmp(trimmed_line, "From:", 5) == 0) {
                                // Prefer sender from From: header if available and not set by filename
                                strncpy(current_email.sender, trimmed_line + 5, sizeof(current_email.sender) - 1);
                                trim_whitespace(current_email.sender);
                            }
                            // An empty line usually separates headers from body
                            if (strlen(trimmed_line) == 0) {
                                in_body = true;
                                continue;
                            }
                        } else {
                            if (current_email.body_line_count < MAX_EMAIL_BODY_LINES) {
                                strncpy(current_email.body[current_email.body_line_count], trimmed_line, sizeof(current_email.body[0]) - 1);
                                current_email.body[current_email.body_line_count][sizeof(current_email.body[0]) - 1] = '\0';
                                current_email.body_line_count++;
                            } else {
                                // Body too long, skip rest
                                break; 
                            }
                        }
                    }
                    fclose(fp);

                    // Final fallback for subject if not found in headers
                    if (current_email.subject[0] == '\0') {
                        // Extract from filename: NNN_sender_subject.eml,U
                        // This parsing is less robust, assuming sender part is clean
                        char* start = strchr(ent->d_name, '_'); // After ID
                        if (start) {
                            start = strchr(start + 1, '_'); // After sender
                            if (start) {
                                start++; // Skip second '_'
                                char* end = strstr(start, ".eml");
                                if (end) {
                                    size_t len = end - start;
                                    if (len > 0 && len < sizeof(current_email.subject)) {
                                        strncpy(current_email.subject, start, len);
                                        current_email.subject[len] = '\0';
                                    }
                                }
                            }
                        }
                        if (current_email.subject[0] == '\0') {
                            strcpy(current_email.subject, "(No Subject)");
                        }
                    }

                    mailbox->emails[mailbox->email_count++] = current_email;

                } else {
                    fprintf(stderr, "ERROR: Could not open email file: %s\n", filepath);
                }
            }
        }
        closedir(dir);
    } else {
        // Error already reported by stat check
    }
}

void mail_system_display_list(const Mailbox* mailbox) {
    if (!mailbox) return;

    printf("%s--- INBOX (%d messages) ---\n%s", ANSI_COLOR_CYAN, mailbox->email_count, ANSI_COLOR_RESET);
    if (mailbox->email_count == 0) {
        printf("No messages.\n");
        return;
    }

    // Sort emails by ID for consistent display
    Email sorted_emails[MAX_EMAILS];
    int display_count = 0;
    for(int i = 0; i < mailbox->email_count; ++i) {
        if (!mailbox->emails[i].is_deleted) { // Only add non-deleted emails to the display list
            sorted_emails[display_count++] = mailbox->emails[i];
        }
    }
    
    // Simple bubble sort for demonstration; a qsort would be better for larger arrays
    for (int i = 0; i < display_count - 1; ++i) {
        for (int j = i + 1; j < display_count; ++j) {
            if (sorted_emails[i].id > sorted_emails[j].id) {
                Email temp = sorted_emails[i];
                sorted_emails[i] = sorted_emails[j];
                sorted_emails[j] = temp;
            }
        }
    }

    if (display_count == 0) {
        printf("No messages in your inbox.\n");
        return;
    }

    for (int i = 0; i < display_count; ++i) {
        const Email* email = &sorted_emails[i];
        printf("%s[%2d] %s%-15s %s\n",
               email->is_read ? "" : ANSI_COLOR_YELLOW, // Color unread emails
               email->id,
               email->is_read ? "[R]" : "[U]",
               email->sender,
               email->subject);
        printf("%s", ANSI_COLOR_RESET); // Reset color after subject
    }
    printf("\n");
}

void mail_system_display_email(const Mailbox* mailbox, int email_index) {
    if (!mailbox || email_index < 0 || email_index >= mailbox->email_count) {
        printf("%sERROR: Invalid email index.\n%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }

    const Email* email = &mailbox->emails[email_index];

    if (email->is_deleted) { // Do not display deleted emails
        printf("%sERROR: Email #%d has been deleted.\n%s", ANSI_COLOR_RED, email->id, ANSI_COLOR_RESET);
        return;
    }

    printf("%s--- EMAIL #%d ---\n%s", ANSI_COLOR_CYAN, email->id, ANSI_COLOR_RESET);
    printf("From: %s\n", email->sender);
    printf("Subject: %s\n", email->subject);
    printf("Status: %s\n", email->is_read ? "Read" : "Unread");
    printf("-------------------------------\n");
    if (email->body_line_count == 0) {
        printf("(No body content)\n");
    } else {
        for (int i = 0; i < email->body_line_count; ++i) {
            printf("%s\n", email->body[i]);
        }
    }
    printf("-------------------------------\n%s", ANSI_COLOR_RESET);
}

void mail_system_mark_as_read(Mailbox* mailbox, int email_id) {
    if (!mailbox) return;

    for (int i = 0; i < mailbox->email_count; ++i) {
        if (mailbox->emails[i].id == email_id) {
            if (mailbox->emails[i].is_deleted) {
                printf("%sEmail #%d is deleted and cannot be marked as read.\n%s", ANSI_COLOR_RED, email_id, ANSI_COLOR_RESET);
                return;
            }
            if (!mailbox->emails[i].is_read) {
                mailbox->emails[i].is_read = true;
                printf("%sEmail #%d marked as read.\n%s", ANSI_COLOR_GREEN, email_id, ANSI_COLOR_RESET);
            } else {
                 printf("%sEmail #%d was already read.\n%s", ANSI_COLOR_YELLOW, email_id, ANSI_COLOR_RESET);
            }
            return;
        }
    }
    printf("%sEmail with ID %d not found to mark as read.\n%s", ANSI_COLOR_RED, email_id, ANSI_COLOR_RESET);
}

void mail_system_delete_email(Mailbox* mailbox, const char* maildir_path, int email_id) {
    if (!mailbox || !maildir_path) return;

    for (int i = 0; i < mailbox->email_count; ++i) {
        if (mailbox->emails[i].id == email_id) {
            if (mailbox->emails[i].is_deleted) {
                printf("%sEmail #%d is already deleted.\n%s", ANSI_COLOR_YELLOW, email_id, ANSI_COLOR_RESET);
                return;
            }

            // Mark in memory
            mailbox->emails[i].is_deleted = true;

            // Rename file for persistence: change suffix to ,D
            char old_filepath[MAX_PATH_LENGTH];
            char new_filepath_suffix[MAX_PATH_LENGTH]; // to hold filename before , and then ,D
            char* comma_pos = strrchr(mailbox->emails[i].filename, ',');

            if (comma_pos) {
                // Construct old path
                snprintf(old_filepath, sizeof(old_filepath), "%s/%s", maildir_path, mailbox->emails[i].filename);
                
                // Construct new filename with ',D'
                strncpy(new_filepath_suffix, mailbox->emails[i].filename, comma_pos - mailbox->emails[i].filename);
                new_filepath_suffix[comma_pos - mailbox->emails[i].filename] = '\0'; // Null terminate
                strcat(new_filepath_suffix, ",D"); // Append deleted status

                // Construct new full path
                char final_new_filepath[MAX_PATH_LENGTH];
                snprintf(final_new_filepath, sizeof(final_new_filepath), "%s/%s", maildir_path, new_filepath_suffix);

                if (rename(old_filepath, final_new_filepath) == 0) {
                    printf("%sEmail #%d deleted successfully (file renamed to %s).\n%s", ANSI_COLOR_GREEN, email_id, new_filepath_suffix, ANSI_COLOR_RESET);
                    // Update filename in memory to reflect the change
                    strncpy(mailbox->emails[i].filename, new_filepath_suffix, sizeof(mailbox->emails[i].filename) - 1);
                    mailbox->emails[i].filename[sizeof(mailbox->emails[i].filename) - 1] = '\0';
                } else {
                    fprintf(stderr, "%sERROR: Could not rename file for email #%d for deletion.\n%s", ANSI_COLOR_RED, email_id, ANSI_COLOR_RESET);
                    // If rename fails, revert in-memory status
                    mailbox->emails[i].is_deleted = false;
                }
            } else {
                fprintf(stderr, "%sERROR: Invalid filename format for email #%d, cannot delete.\n%s", ANSI_COLOR_RED, email_id, ANSI_COLOR_RESET);
                mailbox->emails[i].is_deleted = false;
            }
            return;
        }
    }
    printf("%sEmail with ID %d not found to delete.\n%s", ANSI_COLOR_RED, email_id, ANSI_COLOR_RESET);
}

void mail_system_cleanup(Mailbox* mailbox) {
    // For this simple implementation, nothing to free dynamically.
    // Just reset count.
    if (mailbox) {
        mailbox->email_count = 0;
    }
}