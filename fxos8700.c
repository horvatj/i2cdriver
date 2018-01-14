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

/**
 * defines
 */
#define FXOS8700_ADDRESS	(0x1f)
#define FXOS8700_ID				(0xc7)

#define ACCEL_MG_LSB_2G 	(0.000244F)
#define ACCEL_MG_LSB_4G 	(0.000488F)
#define ACCEL_MG_LSB_8G 	(0.000976F)

#define ACCEL_RANGE				ACCEL_RANGE_4G

/**
 * typedefs
 */
typedef enum{																	// DEFAULT    TYPE
	FXOS8700_REGISTER_STATUS          = 0x00,
	FXOS8700_REGISTER_OUT_X_MSB       = 0x01,
	FXOS8700_REGISTER_OUT_X_LSB       = 0x02,
	FXOS8700_REGISTER_OUT_Y_MSB       = 0x03,
	FXOS8700_REGISTER_OUT_Y_LSB       = 0x04,
	FXOS8700_REGISTER_OUT_Z_MSB       = 0x05,
	FXOS8700_REGISTER_OUT_Z_LSB       = 0x06,
	FXOS8700_REGISTER_WHO_AM_I        = 0x0D,   // 11000111   r
	FXOS8700_REGISTER_XYZ_DATA_CFG    = 0x0E,
	FXOS8700_REGISTER_CTRL_REG1       = 0x2A,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG2       = 0x2B,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG3       = 0x2C,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG4       = 0x2D,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG5       = 0x2E,   // 00000000   r/w
	FXOS8700_REGISTER_MSTATUS         = 0x32,
	FXOS8700_REGISTER_MOUT_X_MSB      = 0x33,
	FXOS8700_REGISTER_MOUT_X_LSB      = 0x34,
	FXOS8700_REGISTER_MOUT_Y_MSB      = 0x35,
	FXOS8700_REGISTER_MOUT_Y_LSB      = 0x36,
	FXOS8700_REGISTER_MOUT_Z_MSB      = 0x37,
	FXOS8700_REGISTER_MOUT_Z_LSB      = 0x38,
	FXOS8700_REGISTER_MCTRL_REG1      = 0x5B,   // 00000000   r/w
	FXOS8700_REGISTER_MCTRL_REG2      = 0x5C,   // 00000000   r/w
	FXOS8700_REGISTER_MCTRL_REG3      = 0x5D,   // 00000000   r/w
} fxos8700Registers_t;

typedef struct fxos8700RawData_s{
	/*! Value for x-axis */
  int16_t x;
	/*! Value for y-axis */
  int16_t y;
	/*! Value for z-axis */
  int16_t z;
} fxos8700RawData_t;

typedef enum
{
	/*! ±0.244 mg/LSB */
	ACCEL_RANGE_2G                    = 0x00,
	/*! ±0.488 mg/LSB */
	ACCEL_RANGE_4G                    = 0x01,
	/*! ±0.976 mg/LSB */
	ACCEL_RANGE_8G                    = 0x02
} fxos8700AccelRange_t;


/**
 * function prototypes 
 */
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

static struct i2c_board_info info_fxos8700={
	I2C_BOARD_INFO("fxos8700", FXOS8700_ADDRESS), 
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
	unsigned long not_copied=0UL;
	unsigned long to_copy=0UL;
	char value=0x00;
	char command=0x00;

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
	char command=0x00;
	char value=0x00;

	printk("fxos8700_probe: %p %p \"%s\"- ", client, id, id->name);
	printk("slaveaddr: %d, name: %s\n", client->addr, client->name);
	if(client->addr !=FXOS8700_ADDRESS){
		printk("i2c_probe: not found\n");
		return -1;
	}
	slave=client;
	/* try to find/detect device */
	command=FXOS8700_REGISTER_WHO_AM_I;
	i2c_master_send(slave, &command, 1);
	i2c_master_recv(slave, &value, 1);

	if(value!=FXOS8700_ID){
		printk("i2c_probe: FXOS8700 not found\n");
		return -1;
	}

//	buf[0]=0x06; // direction port 0
//	buf[1]=0x00; // output
//	i2c_master_send(client, buf, 2);
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

	slave=i2c_new_device(adapter, &info_fxos8700);

	if(slave==NULL){
		pr_err("i2c_new_device failed\n");
		goto del_i2c_driver;
	}
	printk("FXOS8700 sensor detected and configured successfully.\n");
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
