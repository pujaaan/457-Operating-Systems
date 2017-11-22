#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/cred.h>
#include <asm/spinlock.h>
#include <asm/uaccess.h>

int add_tag(pid_t pid, char *tag);
int remove_tag(pid_t pid, char *tag);

asmlinkage long sys_ptag(pid_t pid, char * tag, int mode){
	int returnedValue;
	if (mode == 0) {				// Checks if mode is 0. If it is then
		printk("Add Tag\n");			// call add_tag method
		returnedValue = add_tag(pid, tag);
	}
	else if (mode == 1) {				// Checks if mode is 1. If it is then
		printk("Remove Tag\n");			// call  the remove_tag method
		returnedValue = remove_tag(pid, tag);
	}
	else {
		printk("Invalid Command\n");		// If its not 0 or 1 then do nothing 
		return -1;	
	}
	if (returnedValue != 0) {			// Check if returnedValue is 0 or not/
		printk("Something Went Wrong\n");	// If not 0 then something went wrong
		return -1;
	}
	return 0;	
} 

void copy_tag(struct task_struct *child, struct task_struct *parent){
	
	struct ptag_struct *originalTag;
	struct ptag_struct *new_tag;
	
	INIT_LIST_HEAD(&parent->tagList);
	list_for_each_entry(originalTag, &parent->tagList, listOfProcess) {
		new_tag = kmalloc(sizeof(struct ptag_struct), GFP_KERNEL);
		new_tag->ptags = kmalloc(strlen(originalTag->ptags), GFP_KERNEL);
		strcpy(new_tag->ptags, originalTag->ptags);
		INIT_LIST_HEAD(&new_tag->listOfProcess);
		list_add(&new_tag->listOfProcess, &parent -> tagList);
	}
}

/* adds a tag with some error checking */
int add_tag(pid_t pid, char *tag) {
	struct ptag_struct *new_tag;					// Define struct
	struct task_struct *ts;			

	ts = pid_task(find_vpid((pid_t) pid), PIDTYPE_PID);		// Process
	task_lock(ts);

	new_tag = kmalloc(sizeof(*new_tag), GFP_ATOMIC);		// allocate memory for new_tag

	if (!new_tag) {							
		printk("Memory allocation failed\n");
		kfree(new_tag);						// Free memory
		return -1;
	}
	new_tag->ptags = (char *)kmalloc(strlen_user(tag), GFP_KERNEL);	// allocate memory 
	if (!new_tag->ptags) {						// check
		printk("Memory allocation failed\n");
		kfree(new_tag);						// free memory
		return -1;						// Exit
	}

	memcpy(new_tag->ptags, tag, strlen_user(tag));			// memcpy
	INIT_LIST_HEAD(&new_tag->listOfProcess);			// Initialize Head

	list_add(&new_tag->listOfProcess, &ts->tagList);		// Add to list
	task_unlock(ts);

	printk("Tag %s attached to process %d\n", tag, pid);
	kfree(new_tag);
	return 0;
}

/* removes a tag with some error checking */
int remove_tag(pid_t pid, char *tag) {
	struct ptag_struct *currentTag, *next;					// Define Struct
	struct task_struct *ts;	

	int checkValue = -1;							
	ts = pid_task(find_vpid((pid_t) pid), PIDTYPE_PID);			// Process
	
	task_lock(ts);								// lock process

	if (!list_empty(&ts->tagList)) {					// Check if tag is empty
		list_for_each_entry_safe(currentTag, next, &ts->tagList, listOfProcess) {	// loop
			if(strcmp(currentTag->ptags, tag) == 0) {				// Compare
				printk("Deleting node with tag: %s", tag);			// if equals then delete
				list_del(&currentTag->listOfProcess);				// Delete from list
				kfree(currentTag);						// Free memort
				checkValue = 0;
				return 0;
			}
		}
	}
	if (checkValue == -1) 
		printk("Tag not found.\n");

	task_unlock(ts);
	return checkValue;
}

