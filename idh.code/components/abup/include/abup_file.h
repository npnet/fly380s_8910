#ifndef _ABUP_FILE_H_
#define _ABUP_FILE_H_

extern int32_t abup_remove_file(char *file_path);
extern char* abup_get_delta_file_path(void);
extern char* abup_get_version_file_path(void);
extern char* abup_get_login_file_path(void);
extern char* abup_get_backup_file_path(void);
extern void abup_init_file(void);
extern uint8_t abup_check_upgrade(void);
extern uint32_t abup_get_fota_size(void);
extern void abup_close_file(int file_handle);
extern int8_t abup_copy_detla_buff(char *buff,uint32_t data_len);
extern int32_t abup_write_detla_to_file(void);
extern void abup_recv_buf_memory(void);

#endif
