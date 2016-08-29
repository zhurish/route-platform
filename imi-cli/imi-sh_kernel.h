#ifndef IMISH_KERNEL_H
#define IMISH_KERNEL_H

#ifdef IMISH_IMI_MODULE

extern int imish_kernel_shell_cmd_init(void);
extern int imish_kernel_shell_cmd_exit(void);

//执行系统脚本，并获取脚本输出
extern int imish_kernel_system(int dir, const char *cmd, char *buf, int size);

#endif /* IMISH_IMI_MODULE */

#endif/* IMISH_KERNEL_H */
