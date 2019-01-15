#define CMD_SIZE	          5
#define REGISTER                  "reg_0"
#define TERMINATE                 "ter_0"
#define EXECUTE                   "exe_0"
#define LOC_REQUEST               "loc_r"
#define LOC_SUCCESS               "loc_s"
#define LOC_FAILURE               "loc_f"
#define EXEC_SUCCESS              "exe_s"
#define EXEC_FAILURE              "exe_f"
#define REG_SUCCESS               "reg_s"
#define REG_FAILURE               "reg_f"

#define BUFFER_LEN               64

#define ERROR_OPEN_SOCKET        -1
#define ERROR_BIND_SOCKET        -2
#define ERROR_CONN_BINDER        -3
#define ERROR_CONN_SERVER        -4
#define ERROR_SELECT             -5
#define ERROR_SEND_BINDER        -6
#define ERROR_RECV_BINDER        -7
#define ERROR_SEND_SERVER        -8
#define ERROR_RECV_SERVER        -9
#define ERROR_SEND_CLIENT        -10
#define ERROR_RECV_CLIENT        -11
#define ERROR_INVALID_CMD        -12

#define ERROR_EMPTY_DB           -21
#define SKELETON_WARNING         404

#define BINDER_SHUTDOWN 255
