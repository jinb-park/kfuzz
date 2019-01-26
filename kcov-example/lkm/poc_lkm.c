#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/msg.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/bpf.h>

struct poc_lkm_size {
	char dummy[128];
};

#define POC_LKM_IOCTL_MAGIC ('P')
#define POC_LKM_CMD_0 _IOWR(POC_LKM_IOCTL_MAGIC, 0, struct poc_lkm_size)
#define POC_LKM_CMD_1 _IOWR(POC_LKM_IOCTL_MAGIC, 1, struct poc_lkm_size)
#define POC_LKM_CMD_2 _IOWR(POC_LKM_IOCTL_MAGIC, 2, struct poc_lkm_size)

void poc_lkm_coverage1(unsigned long arg)
{
	if (arg < 0xffff000000000000)
		pr_info("user!\n");
	else
		pr_info("kernel!\n");
}

void poc_lkm_coverage0(unsigned long arg)
{
	return;
}

void poc_lkm_coverage2(unsigned long arg)
{
	if (arg == 0x10) {
		pr_info("arg = 0x10\n");
		poc_lkm_coverage1(arg);
	}
}

long poc_lkm_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case POC_LKM_CMD_0:
		poc_lkm_coverage0(arg);
		break;
	case POC_LKM_CMD_1:
		poc_lkm_coverage1(arg);
		break;
	case POC_LKM_CMD_2:
		poc_lkm_coverage2(arg);
		break;
	default:
		break;
	}
	return 0;
}

struct file_operations poc_lkm_fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl = poc_lkm_ioctl,
};

static int create_poc_lkm_fops(void)
{
    proc_create("poc_lkm", 0666, NULL, &poc_lkm_fops);
    return 0;
}

static int remove_poc_lkm_fops(void)
{
    remove_proc_entry("poc_lkm", NULL);
    return 0;
}

int poc_lkm_init(void)
{
	create_poc_lkm_fops();
	return 0;
}

void poc_lkm_exit(void)
{
	remove_poc_lkm_fops();
	return;
}

module_init(poc_lkm_init);
module_exit(poc_lkm_exit);
MODULE_LICENSE("GPL");
