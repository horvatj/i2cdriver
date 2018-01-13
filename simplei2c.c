/************************************************************************/
/* Quellcode zum Buch                                                   */
/*                     Linux Treiber entwickeln                         */
/* (4. Auflage) erschienen im dpunkt.verlag                             */
/* Copyright (c) 2004-2015 Juergen Quade und Eva-Katharina Kunst        */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         */
/* GNU General Public License for more details.                         */
/*                                                                      */
/************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

static char hello_world[]="simple I2C\n";

static dev_t hello_dev_number;
static struct cdev *driver_object;
static struct class *hello_class;
static struct device *hello_dev;

/* function prototypes */
static int driver_open(struct inode* inodep, struct file* filep);
static int driver_close(struct inode* inodep, struct file* filep);
static ssize_t driver_read(struct file* filep, char __user* user,
  size_t count, loff_t* offset);

/* globals */
static struct file_operations fops={
  .owner=THIS_MODULE,
  .read=driver_read,
  .open=driver_open,
  .release=driver_close,
};

/* code */
static int driver_open(struct inode* inodep, struct file* filep){
	dev_info(hello_dev, "driver_open called\n");
	return 0;
}

static int driver_close(struct inode* inodep, struct file* filep){
	dev_info(hello_dev, "driver_close called\n");
	return 0;
}

static ssize_t driver_read(struct file* filep, char __user* user,
		size_t count, loff_t* offset){
	unsigned long not_copied;
	unsigned long to_copy;

	to_copy=min(count, strlen(hello_world)+1);
	not_copied=copy_to_user(user, hello_world, to_copy);
	*offset+=to_copy-not_copied;

	return to_copy-not_copied;
}

static int __init mod_init(void){
	if(alloc_chrdev_region(&hello_dev_number, 0, 1, "simplei2c")<0){
		return -EIO;
	}
	
	driver_object=cdev_alloc(); /* Anmeldeobjekt reservieren */
	
	if(driver_object==NULL){
		goto free_device_number;
	}
	
	driver_object->owner=THIS_MODULE;
	driver_object->ops=&fops;
	
  if (cdev_add(driver_object,hello_dev_number, 1)){
		goto free_cdev;
	}
	
  /* Eintrag im Sysfs, damit Udev den Geraetedateieintrag erzeugt. */
	hello_class=class_create(THIS_MODULE, "simplei2c");
	if (IS_ERR(hello_class)){
		pr_err("simplei2c: no udev support\n");
		goto free_cdev;
	}

	hello_dev=device_create(hello_class, NULL, hello_dev_number,
			NULL, "%s", "simplei2c");
	
  if (IS_ERR(hello_dev)){
		pr_err("simplei2c: device_create failed\n");
		goto free_class;
	}
	return 0;

free_class:
	class_destroy(hello_class);

free_cdev:
	kobject_put(&driver_object->kobj);

free_device_number:
	unregister_chrdev_region(hello_dev_number, 1);
	return -EIO;
}

static void __exit mod_exit(void){
	/* Loeschen des Sysfs-Eintrags und damit der Geraetedatei */
	device_destroy(hello_class, hello_dev_number);
	class_destroy(hello_class);
	/* Abmelden des Treibers */
	cdev_del(driver_object);
	unregister_chrdev_region(hello_dev_number, 1);
	return;
}

module_init(mod_init);
module_exit(mod_exit);

/* Metainformation */
MODULE_AUTHOR("Johann Horvat");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Device Driver reading out the accelerometer/magnetometer NXP FXOS8700 using I2C");
