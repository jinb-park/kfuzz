#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

struct poc_lkm_object {
	unsigned char data0;
	unsigned char data1;
	unsigned long data2;
	unsigned long data3;
};

// Construct Use-After-Free with combination of below commands
#define POC_LKM_IOCTL_MAGIC ('P')
#define POC_LKM_CMD_0 _IOWR(POC_LKM_IOCTL_MAGIC, 0, struct poc_lkm_object)
#define POC_LKM_CMD_1 _IOWR(POC_LKM_IOCTL_MAGIC, 1, struct poc_lkm_object)

noinline void poc_lkm_bug0(struct poc_lkm_object *obj)
{
	pr_info("%ld\n", *(unsigned long *)obj->data2);
}
noinline void poc_lkm_bug1(struct poc_lkm_object *obj)
{
	pr_info("%ld\n", *(unsigned long *)obj->data3);
}

noinline void poc_lkm_cmd0(struct poc_lkm_object *obj)
{
	if (obj->data0 == 32)
		poc_lkm_bug0(obj);
}
noinline void poc_lkm_cmd1(struct poc_lkm_object *obj)
{
	if (obj->data1 == 64)
		poc_lkm_bug1(obj);
}

noinline long poc_lkm_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct poc_lkm_object obj;

	if (cmd == POC_LKM_CMD_0 || cmd == POC_LKM_CMD_1) {
		if (copy_from_user(&obj, (void *)arg, sizeof(obj))) {
			pr_err("copy_from_user error\n");
			return 0;
		}
	}

	switch (cmd) {
	case POC_LKM_CMD_0:
		poc_lkm_cmd0(&obj);
		break;
	case POC_LKM_CMD_1:
		poc_lkm_cmd1(&obj);
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
    //pr_info("CMD0 : %08x, CMD1 : %08x\n", POC_LKM_CMD_0, POC_LKM_CMD_1);
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
