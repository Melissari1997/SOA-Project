#ifndef tbdeType_h
#define tbdeType_h


#define MODNAME "TAG_Data_Messaging"
// ###### Define Key tag_get ######
// int tag_get(int key, int command, int permission);
enum tag_get_KEY { TAG_IPC_PRIVATE = -1 };
enum tag_get_command { CREATE = 0, OPEN };
enum tag_get_permission { OPEN_ROOM = 0, PRIVATE_ROOM };
// ################################

// #### Define command tag_ctl ####
// int tag_ctl(int tag, int command);
enum tag_ctl_command { AWAKE_ALL = 0, REMOVE };
// ################################

// << Module Define >>
#define levelDeep 32 // Max number of sub-rooms
#define MAX_MESSAGE_SIZE 4096

#endif