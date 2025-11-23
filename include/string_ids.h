#ifndef STRING_IDS_H
#define STRING_IDS_H

typedef enum {
    TEXT_INVALID = 0,
    // Navi Scene Text
    TEXT_NAVI_STATE_TITLE,
    TEXT_NAVI_STATE_DESC1,
    TEXT_NAVI_STATE_DESC2,
    TEXT_NAVI_STATE_DESC3,
    TEXT_NAVI_STATE_DESC4,
    // Navi Scene Choices
    TEXT_CHOICE_NAVI_SHUTDOWN,
    TEXT_CHOICE_NAVI_REBOOT,
    TEXT_CHOICE_NAVI_CONNECT,

    TEXT_COUNT // Always last, gives a count of total strings
} StringID;

#endif // STRING_IDS_H
