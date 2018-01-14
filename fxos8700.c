/**
 * @file: fxos8700.c
 *
 * @brief Linux driver for FXOS8700
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <asm/uaccess.h>

/*  function prototypes */
static ssize_t driver_write(struct file* filep, const char __user* user, 
			    size_t count, loff_t* offset);
static ssize_t driver_read(struct file* filep, char __user* user, size_t count,
			   loff_t* offset);
static int fxos8700_probe(struct i2c_client* client, 
			  const struct i2c_device_id* id);
static int fxos8700_remove(struct i2c_client* client);
static int __init mod_init(void);
static void __exit mod_exit(void);

/*  globals */
static dev_t fxos8700_dev_number;

static struct cdev* driver_object;
static struct class* fxos8700_class;
static struct device* fxos8700_dev;

static struct i2c_adapter* adapter;
static struct i2c_client* slave;

static struct i2c_device_id fxos8700_idtable[]={
	{"fxos8700", 0}, 
	{}
};

static struct file_operations fops={
	.owner=THIS_MODULE, 
	.write=driver_write, 
	.read=driver_read, 
};

static struct i2c_driver fxos8700_driver={
	.driver={
		.name ="fxos8700", 
	}, 
	.id_table   =fxos8700_idtable, 
	.probe     =fxos8700_probe, 
	.remove    =fxos8700_remove, 
};


MODULE_DEVICE_TABLE(i2c, fxos8700_idtable);

static struct i2c_board_info info_20={
	I2C_BOARD_INFO("fxos8700", 0x20), 
};

static ssize_t driver_write(struct file* filep, const char __user* user, 
			    size_t count, loff_t* offset){
	unsigned long not_copied, to_copy;
	char value=0, buf[2];

	to_copy=min(count, sizeof(value));
	not_copied=copy_from_user(&value, user, to_copy);
	to_copy-=not_copied;

	if(to_copy>0){
		buf[0]=0x02; // output port 0
		buf[1]=value;
		i2c_master_send(slave, buf, 2);
	}
	return to_copy;
}

static ssize_t driver_read(struct file* filep, char __user* user, size_t count,
			   loff_t* offset){
	unsigned long not_copied, to_copy;
	char value, command;

	command=0x01; // input port 1
	i2c_master_send(slave, &command, 1);
	i2c_master_recv(slave, &value, 1);

	to_copy=min(count, sizeof(value));
	not_copied=copy_to_user(user, &value, to_copy);
	return to_copy - not_copied;
}

static int fxos8700_probe(struct i2c_client* client, 
			  const struct i2c_device_id* id){
	char buf[2];

	printk("fxos8700_probe: %p %p \"%s\"- ", client, id, id->name);
	printk("slaveaddr: %d, name: %s\n", client->addr, client->name);
	if(client->addr !=0x20){
		printk("i2c_probe: not found\n");
		return -1;
	}
	slave=client;
	// configuration
	buf[0]=0x06; // direction port 0
	buf[1]=0x00; // output
	i2c_master_send(client, buf, 2);
	return 0;
}

static int fxos8700_remove(struct i2c_client* client){
	return 0;
}

static int __init mod_init(void){
	if(alloc_chrdev_region(&fxos8700_dev_number, 0, 1, "fxos8700")<0){
		return -EIO;
	}

	driver_object=cdev_alloc();

	if(driver_object==NULL){
		goto free_device_number;
	}

	driver_object->owner=THIS_MODULE;
	driver_object->ops=&fops;

	if(cdev_add(driver_object, fxos8700_dev_number, 1)){
		goto free_cdev;
	}

	fxos8700_class=class_create(THIS_MODULE, "fxos8700");

	if(IS_ERR(fxos8700_class)){
		pr_err("fxos8700: no udev support\n");
		goto free_cdev;
	}

	fxos8700_dev=device_create(fxos8700_class, NULL, 
				   fxos8700_dev_number, NULL, "%s", "fxos8700");

	dev_info(fxos8700_dev, "mod_init\n");

	if(i2c_add_driver(&fxos8700_driver)){
		pr_err("i2c_add_driver failed\n");
		goto destroy_dev_class;
	}

	adapter=i2c_get_adapter(0); // Adapter sind durchnummeriert

	if(adapter==NULL){
		pr_err("i2c_get_adapter failed\n");
		goto del_i2c_driver;
	}

	slave=i2c_new_device(adapter, &info_20);

	if(slave==NULL){
		pr_err("i2c_new_device failed\n");
		goto del_i2c_driver;
	}
	return 0;

del_i2c_driver:
	i2c_del_driver(&fxos8700_driver);

destroy_dev_class:
	device_destroy(fxos8700_class, fxos8700_dev_number);
	class_destroy(fxos8700_class);

free_cdev:
	kobject_put(&driver_object->kobj);

free_device_number:
	unregister_chrdev_region(fxos8700_dev_number, 1);
	return -EIO;
}

static void __exit mod_exit(void){
	dev_info(fxos8700_dev, "mod_exit");

	device_destroy(fxos8700_class, fxos8700_dev_number);
	class_destroy(fxos8700_class);

	cdev_del(driver_object);
	unregister_chrdev_region(fxos8700_dev_number, 1);

	i2c_unregister_device(slave);
	i2c_del_driver(&fxos8700_driver);

	return;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
