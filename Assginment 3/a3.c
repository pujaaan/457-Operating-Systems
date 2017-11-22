#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>
/****
 *
 * This is a skeleton for assignment 3. It includes the code to handle the 
 * production and clean up of customers. Your job, is to write the barista
 * threads and manage the buffers (pthread_mutexes will be your friend).
 * 
 *
 */

/**
 * Definition of the maximum buffer size
 */
#define MAX_BUFFER_SIZE 500

/**
 * Definition of how many customers we will processes and how many
 * baristas we'll have serve them
 */


/* Data structure to pass data to the customer thread */
typedef struct _customer_data_t {
  int tid;
} customer_data_t; 

/* Data structure to pass data to the cleaner thread */
typedef struct _cleaner_data_t {
  int tid;
} cleaner_data_t; 

// Space and train model data structures
typedef struct _train_barista_data_t {
  int tid;
} train_barista_data_t; 

typedef struct _space_barista_data_t {
  int tid;
} space_barista_data_t;

/* Data structures to organize information about the order */
typedef struct _item_t {
	int production_time_ms;
} item_t;

typedef struct _order_t {
	int customer;
	int size;
	item_t items [];
} order_t;

/**
 * Buffer to manage the order data
 */
order_t ** order_buffer;
int order_index;

// mutex for order buffer
pthread_mutex_t order_mutex;
// semaphore for order buffer
sem_t order_sem;

/**
 * Buffer to manage the returned "products"
 */

int * bar_buffer;
int bar_index;

// mutex for bar buffer
pthread_mutex_t bar_mutex;
// semaphore for bar buffer
sem_t bar_sem;


// Buffer for space
order_t ** space_buffer;
int space_index;

// mutex for space buffer
pthread_mutex_t space_mutex;
// semaphore for space buffer
sem_t space_sem;

// Mutex for cash
pthread_mutex_t cash_mutex;

/**
 * Integer to play the "cash register"
 */

int cash;


// Number of baristas
int TOTAL_BARISTAS;

/* Counter for how many customers yet to processes */
int TOTAL_CUSTOMERS;
int total_customer;
/*
 * Calculate the amount for the customer thread to sleep 
 */
void decide_time_to_sleep(struct timespec *st) {

	int adjust_ms, base_ms;

	/* Assume we want to wait about half a second */
	base_ms = 500;
	adjust_ms = (rand() % 500) - 250;

	/* Set adjusted time in timespec */
	st->tv_sec = 0;
	st->tv_nsec = (base_ms + adjust_ms) * 1000000L;

}

/*
 * Generate how long a barista must "work" to make the order
 */
int generate_prduction_time_ms() {

	int c = (rand() % 3);

	switch(c) {
		case 0 : return(200); break;
		case 1 : return(400); break;
		case 2 : return(800); break;

	}	

}

/*
 * For a given order size, generate items to be produced
 */
void generate_order (order_t *  order) {

	int i;
	for (i = 0; i < order->size; i++) {

		item_t order_part;
		order_part.production_time_ms = generate_prduction_time_ms();
		order->items[i] = order_part;
	}

}

/*
 * Generate a size for an order
 */ 
int generate_order_size() { 

		int o_size; 
		o_size = (rand() % 4) + 1;
		return(o_size);

}

void print_order(order_t * order){

		printf("Order %d (%d)\n", order->customer, order->size);
		for(int i = 0; i < order->size; i++) {
			printf("\tItem %d: %d\n", i, order->items[i].production_time_ms);

		}

}

 
/*
 * Main Customer Thread function.
 *
 * Generate customers and orders over time, clean up as customers are
 * served.
 */
void *customer_run(void *arg) {
  customer_data_t *data = (customer_data_t *)arg;
	int customer_id = 0;
	int order_size = 0;
	int copy_customer = TOTAL_CUSTOMERS;
	
	/* Run for as long as we have customers incoming */
	while (copy_customer > 0) {
		/* Create customer and generate order */
		printf("Created Customer %d\n", ++customer_id); 
		order_size = generate_order_size();	

		order_t * new_order = malloc(sizeof (order_t) + sizeof (item_t [order_size]));
		new_order->customer=customer_id;
		new_order->size=order_size;
		printf("Ordering %d items\n", new_order->size);
		generate_order(new_order);
		
		// Locks order buffer so others cant access it
		pthread_mutex_lock(&order_mutex);		
		order_buffer[order_index] = new_order;	
		order_index++;
		pthread_mutex_unlock(&order_mutex);	
		
		

		/* Put the thread to sleep until the next customer comes */
		struct timespec c;
		decide_time_to_sleep(&c);
		printf("Sleeping Until Next Customer in: %lld.%.3ld\n", (long long)c.tv_sec, c.tv_nsec);
		struct timespec r;
		nanosleep(&c, &r);	
		copy_customer--;
		sem_post(&order_sem);

	}

  pthread_exit(NULL);
}

void *cleanup_run(void *arg) {

	int customers_cleared = 0;
	int cash_total;

	int bar;
	
	while (customers_cleared < TOTAL_CUSTOMERS) {

		// Wait until there is something but by the barista threads in the buffer
		sem_wait(&bar_sem);

	
		/* Take the next bar out of the buffer */
		
		// Lock barista buffer
		pthread_mutex_lock(&bar_mutex);	
		bar_index--;
		bar = bar_buffer[bar_index];
		pthread_mutex_unlock(&bar_mutex);	
		
		customers_cleared++;
		
		// Lock cash variable
		pthread_mutex_lock(&cash_mutex);			
		cash_total = cash;
		pthread_mutex_unlock(&cash_mutex);	

		printf("Cleared %d customers - Cash (%d)\n", customers_cleared, cash_total);

	}

	pthread_exit(NULL);
}
 
void *train_barista_run (void *arg){
	while(1){
		// wait until there is something in the order buffer
		sem_wait(&order_sem);
		
		int *barista_id = (int *)arg;
		
		// Mutex lock
		pthread_mutex_lock(&order_mutex);
		order_index--;
		order_t * cust_order = order_buffer[order_index];
		pthread_mutex_unlock(&order_mutex);
		
		printf("The barista %d is producing the orders for customer %d \n", *barista_id - 1001, cust_order -> customer);
		
		for(int i = 0; i < cust_order -> size; i++){
			usleep(cust_order -> items[i].production_time_ms*1000);		
		}
		// Mutex lock
		pthread_mutex_lock(&bar_mutex);
		bar_buffer[bar_index] = cust_order -> customer;
		pthread_mutex_unlock(&bar_mutex);
		
		
		barista_id = (int *) arg;
		
		// Mutex lock
		pthread_mutex_lock(&cash_mutex);
		cash += 10;
		pthread_mutex_unlock(&cash_mutex);
		
		
		printf("The barista %d has served customer %d \n", *barista_id -1001, cust_order -> customer);
		sem_post(&bar_sem);
	}


	pthread_exit(NULL);
	
}

void *cashier_barista_run (void *arg){
	while(1){
		// Wait until there is something in the order buffer
		sem_wait(&order_sem);
		order_t * cust_order;
		
		// Mutex lock
		pthread_mutex_lock(&order_mutex);
		order_index--;
		cust_order = order_buffer[order_index];
		pthread_mutex_unlock(&order_mutex);
		
		printf("The cashier barista took orders from customer %d \n", cust_order -> customer);
		
		// Mutex lock
		pthread_mutex_lock(&space_mutex);
		space_buffer[space_index++] = cust_order;
		pthread_mutex_unlock(&space_mutex);
		
		// Increment semaphore
		sem_post(&space_sem);		
		
	}
	pthread_exit(NULL);
}		
		
		
		
void *producer_barista_run (void *arg){
	while(1){
		
		// Wait till the first thread puts into the space buffer
		sem_wait(&space_sem);		
		
		order_t * cust_order;
		// Mutex lock
		pthread_mutex_lock(&space_mutex);
		space_index--;
		cust_order = space_buffer[space_index];
		pthread_mutex_unlock(&space_mutex);
		
		
		int *barista_id = (int *)arg;
		printf("The barista %d is producing the orders for customer %d \n", *barista_id - 1002, cust_order -> customer);
		
		for(int i = 0; i < cust_order -> size; i++){
			usleep(cust_order -> items[i].production_time_ms*1000);		
		}
		pthread_mutex_lock(&bar_mutex);
		bar_buffer[bar_index] = cust_order -> customer;
		pthread_mutex_unlock(&bar_mutex);
		
		
		barista_id = (int *) arg;
		
		pthread_mutex_lock(&cash_mutex);
		cash += 10;
		pthread_mutex_unlock(&cash_mutex);
		
		
		printf("The barista %d has served customer %d \n", *barista_id -1002, cust_order -> customer);
		sem_post(&bar_sem);
	}

	pthread_exit(NULL);
	
}
 
/*
 * Run the main simulation
 */
int main(int argc, char **argv) {

	int model, rc;
	char *endPtr;
	// Semaphore Initialization
	sem_init(&order_sem, 0, 0);
	sem_init(&bar_sem, 0, 0);
	sem_init(&space_sem, 0, 0);
	// mutex initialization
	if (pthread_mutex_init(&order_mutex, NULL) != 0){
		printf("Error!!!! Cannot Initialize Mutex \n");
	}
	if (pthread_mutex_init(&bar_mutex, NULL) !=0){
		printf("Error!!!! Cannot Initialize Mutex \n");
	}
	
	// Getting command line agruments and doing the error checks.
	if(argc !=4){
		printf("usage: %s <number of customer>, <number of barista> <model: 0 for train, 1 for space> \n", argv[0]);
		return EXIT_FAILURE;
	}
	TOTAL_CUSTOMERS = strtol(argv[1], &endPtr, 10);
	if(*endPtr){
		printf("usage: %s <INT> <int> <int> \n", argv[0]);
		return EXIT_FAILURE;
	}
    TOTAL_BARISTAS = strtol(argv[2], &endPtr, 10);
	if(*endPtr){
		printf("usage: %s <int> <INT> <int> \n", argv[0]);
		return EXIT_FAILURE;
	}
	model = strtol(argv[3], &endPtr, 10);
	if(*endPtr){
		printf("usage: %s <int> <int> <INT> \n", argv[0]);
		return EXIT_FAILURE;
	}
	if(0 > model || model > 1){
		printf("usage: %s <int> <int> <0 OR 1> \n", argv[0]);
		return EXIT_FAILURE;

	}
	if(TOTAL_CUSTOMERS < 0 || TOTAL_BARISTAS < 0){
		printf("The number of Customers or Baristas must be greater than 1");	
	}
	
	total_customer = TOTAL_CUSTOMERS;
  /* Intializes random number generator */
  time_t t;
  srand((unsigned) time(&t));

	/* Initalize buffers */
	order_buffer = malloc (sizeof(order_t) * MAX_BUFFER_SIZE);
	order_index = 0;
	bar_buffer = malloc (sizeof(int) * MAX_BUFFER_SIZE);
	space_buffer =  malloc (sizeof(int) * MAX_BUFFER_SIZE);
	bar_index = 0;
	cash=0;

	/* Create Customer Thread */
	pthread_t cust_t;
	customer_data_t cust_data;
	cust_data.tid=1000;
  if ((rc = pthread_create(&cust_t, NULL, customer_run, &cust_data))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return EXIT_FAILURE;
	}

	/* Create Cleaner Thread */
	pthread_t cleaner_t;
	cleaner_data_t cleaner_data;
	cleaner_data.tid=1001;
	if ((rc = pthread_create(&cleaner_t, NULL, cleanup_run, &cleaner_data))) {
		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		return EXIT_FAILURE;
	}
	 /* Create Barista Threads */

	pthread_t barista_t[TOTAL_BARISTAS];
	train_barista_data_t train_barista_data[TOTAL_BARISTAS];
	space_barista_data_t space_barista_data[TOTAL_BARISTAS];
	// Train Model
	if(model == 0){

		for (int i = 0; i < TOTAL_BARISTAS; i++) {
			train_barista_data[i].tid= 1002 + i;
			if (rc = pthread_create(&barista_t[i], NULL, train_barista_run, &train_barista_data[i])) {
				fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
				return EXIT_FAILURE;
			}	
		}

	}
	// Space model
	else{
		// create space cashier threads
		space_barista_data[0].tid = 1002;
		if (rc = pthread_create(&barista_t[0], NULL, cashier_barista_run, &space_barista_data[0])) {
				fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		}
		// Create space producer threads
		for (int i = 1; i < TOTAL_BARISTAS; i++) {
			space_barista_data[i].tid=1003 + i;
			if (rc = pthread_create(&barista_t[i], NULL, producer_barista_run, &space_barista_data[i])) {
				fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
				return EXIT_FAILURE;
			}	
		}
	}



	pthread_join(cust_t, NULL);
	pthread_join(cleaner_t, NULL);

	for (int i = 0; i < TOTAL_BARISTAS; i++) {
		pthread_cancel(barista_t[i]);
		pthread_join(barista_t[i], NULL);
	}
	// Destroying
	pthread_mutex_destroy(&order_mutex);
	pthread_mutex_destroy(&bar_mutex);
	pthread_mutex_destroy(&cash_mutex);
	
	sem_destroy(&order_sem);
	sem_destroy(&bar_sem);

  return EXIT_SUCCESS;
}
