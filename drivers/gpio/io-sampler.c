#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
 
//..............................................................................

#define LKM_NAME "io-sampler"

// modify and recompile to test different scenarios

#define USE_GPIO_REGS    1  // use BCM-2836 GPIO registers (vs gpio_get_value/gpio_set_value)
#define USE_RW_IRQ       1  // use interrupts for read-write benchmark
#define USE_RW_POLL      0  // use polling for read-write benchmark
#define USE_RW_YIELD     0  // yield CPU with schedule () during polling
#define USE_WO_BLOCKING  0  // perform blocking write-only benchmark in module_init
#define USE_WO_THREADED  0  // perform write-only benchmark in a dedicated thread
#define USE_AFFINITY     1  // assign threads to different CPU cores

// sanity check

#if (USE_RW_IRQ && USE_RW_POLL || USE_WO_BLOCKING && USE_WO_THREADED)
#	error invalid configuration (mutual exlusive settings)
#endif

// connect pins A_IN <-> A_OUT and B_IN <-> B_OUT for read-write benchmark

#define GPIO_A_IN  16
#define GPIO_B_OUT 23
#define GPIO_B_IN  19
//
#define GPIO_C_IN  26

// iteration counts for read-write and write-only tests

#if (USE_GPIO_REGS)
#	define RW_IRQ_ITERATION_COUNT  500000   // 500,000 iterations
#	define RW_POLL_ITERATION_COUNT 5000000  // 5 mil iterations
#	define WO_ITERATION_COUNT      10000000 // 10 mil iterations
#else
#	define RW_IRQ_ITERATION_COUNT  50000    // 50,000 iterations
#	define RW_POLL_ITERATION_COUNT 500000   // 500,000 iterations
#	define WO_ITERATION_COUNT      1000000  // 1 mil iterations
#endif

static volatile unsigned int arr_pulse_count[3] = {0, 0, 0};
static unsigned int pulse_count0 = 0;
static unsigned int pulse_count1 = 0;
static unsigned int pulse_count2 = 0;
//static int count;

//..............................................................................
#if (USE_GPIO_REGS)

static volatile unsigned int* g_gpio_regs;

#	define GPIO_BASE_ADDR 0x3f200000

#	define GPIO_SET_FUNC_IN(g)  *(g_gpio_regs+((g)/10)) &= ~(7<<(((g)%10)*3))
#	define GPIO_SET_FUNC_OUT(g) *(g_gpio_regs+((g)/10)) |= (1<<(((g)%10)*3))

#	define GPIO_GET(g) ((*(g_gpio_regs+13) & (1<<(g))) != 0)
#	define GPIO_SET(g) (*(g_gpio_regs+7) = 1<<(g))
#	define GPIO_CLR(g) (*(g_gpio_regs+10) = 1<<(g))
#	define GPIO_PULL     *(g_gpio_regs+37) // Pull up/pull down
#	define GPIO_PULLCLK0 *(g_gpio_regs+38) // Pull up/pull down clock
#else

#	define GPIO_GET(g) (gpio_get_value (g))
#	define GPIO_SET(g) (gpio_set_value (g, 1))
#	define GPIO_CLR(g) (gpio_set_value (g, 0))

#endif

//..............................................................................

// timestamps in 100-nsec intervals (aka windows file time)

static inline unsigned long long get_timestamp (void)
{
	struct timespec64 tspec;
        ktime_get_real_ts64 (&tspec);
	return tspec.tv_sec * 10000000 + tspec.tv_nsec / 100;
}

//..............................................................................

// read-write benchmark test

#if (USE_RW_IRQ)

//static unsigned long long g_count = 0;

static int g_gpio_a_irq = -1;
static int g_gpio_b_irq = -1;
static int g_gpio_c_irq = -1;
static unsigned long last_int1_time = 0;
static unsigned long last_int2_time = 0;
static unsigned long last_int3_time = 0;

static irqreturn_t gpio_a_irq_handler (int irq, void* context)
{
	unsigned long int_time = ktime_get_ns();
	unsigned long flags;
	
	//printk (KERN_INFO LKM_NAME ": IRQ1 time gap: %lu\n", int_time - last_int1_time);
	if (int_time - last_int1_time < 5500) {
		return IRQ_HANDLED;
	}
	last_int1_time = int_time;
        local_irq_save(flags);
	//printk (KERN_INFO LKM_NAME ": IRQ time: %lu\n", time);
	//__sync_add_and_fetch (&pulse_count[0], 1);
	arr_pulse_count[0]++;
	pulse_count0 = arr_pulse_count[0];
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static irqreturn_t gpio_b_irq_handler (int irq, void* context)
{
	unsigned long int_time = ktime_get_ns();
	unsigned long flags;
    
	//printk (KERN_INFO LKM_NAME ": IRQ2 time gap: %lu\n", int_time - last_int2_time);
	if (int_time - last_int2_time < 5500) {
		return IRQ_HANDLED;
	}
	last_int2_time = int_time;
        local_irq_save(flags);
	//__sync_add_and_fetch (&pulse_count[1], 1);
	arr_pulse_count[1]++;
	pulse_count1 = arr_pulse_count[1];
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static irqreturn_t gpio_c_irq_handler (int irq, void* context)
{
	unsigned long int_time = ktime_get_ns();
	unsigned long flags;
    
	//printk (KERN_INFO LKM_NAME ": IRQ3 time gap: %lu\n", int_time - last_int3_time);
	if (int_time - last_int3_time < 5500) {
		return IRQ_HANDLED;
	}
	last_int3_time = int_time;
        local_irq_save(flags);
	//__sync_add_and_fetch (&pulse_count[2], 1);
	arr_pulse_count[2]++;
	pulse_count2 = arr_pulse_count[2];
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

#endif

//..............................................................................
static void enable_pullup(void)
{
   	GPIO_PULL = 1;
   	udelay(1);
   	// clock on GPIO 16 & 19
   	GPIO_PULLCLK0 = 0x00090000;//0x03000000;
   	udelay(1);
   	GPIO_PULL = 0;
   	GPIO_PULLCLK0 = 0;
}

//..............................................................................

int __init io_sampler_init (void)
{
#if (USE_RW_IRQ || !USE_GPIO_REGS)
	int result;
#endif
	printk (KERN_INFO LKM_NAME ": --- loading GPIO benchmark test ---\n");

	
#if (USE_GPIO_REGS)
	printk (KERN_INFO LKM_NAME ": preparing GPIOs for register access...\n");

	g_gpio_regs = (volatile unsigned int*) ioremap (GPIO_BASE_ADDR, 16 * 1024);
	if (!g_gpio_regs)
	{
		printk (KERN_INFO LKM_NAME ": error mapping GPIO registers\n");
		return -EBUSY;
	}

	GPIO_SET_FUNC_IN (GPIO_A_IN);
	GPIO_SET_FUNC_IN (GPIO_B_IN);
	GPIO_SET_FUNC_IN (GPIO_C_IN);
	
	GPIO_CLR (GPIO_B_OUT);

	enable_pullup();
#endif
	
#if (USE_RW_IRQ)

	g_gpio_a_irq = gpio_to_irq (GPIO_A_IN);

	printk (KERN_INFO LKM_NAME ": setting interrupt handler for GPIO %d IRQ %d...\n", GPIO_A_IN, g_gpio_a_irq);
	
	result = request_irq (g_gpio_a_irq, gpio_a_irq_handler, IRQF_TRIGGER_FALLING, LKM_NAME, NULL);
	
	if (result != 0)
	{
		printk (KERN_ERR LKM_NAME ": error: %d\n", result);
		return result;
	}
	
	g_gpio_b_irq = gpio_to_irq (GPIO_B_IN);

	printk (KERN_INFO LKM_NAME ": setting interrupt handler for GPIO %d IRQ %d...\n", GPIO_B_IN, g_gpio_b_irq);

	result = request_irq (g_gpio_b_irq, gpio_b_irq_handler, IRQF_TRIGGER_FALLING, LKM_NAME, NULL);
	if (result != 0)
	{
		printk (KERN_ERR LKM_NAME ": error: %d\n", result);
		return result;
	}

	g_gpio_c_irq = gpio_to_irq (GPIO_C_IN);

	printk (KERN_INFO LKM_NAME ": setting interrupt handler for GPIO %d IRQ %d...\n", GPIO_C_IN, g_gpio_c_irq);

	result = request_irq (g_gpio_c_irq, gpio_c_irq_handler, IRQF_TRIGGER_FALLING, LKM_NAME, NULL);
	if (result != 0)
	{
		printk (KERN_ERR LKM_NAME ": error: %d\n", result);
		return result;
	}
#endif


#if (USE_RW_IRQ || USE_RW_POLL)
	//printk (KERN_INFO LKM_NAME ": lowering GPIO %d to initiate a loop...\n", GPIO_A_OUT);

	//g_rw_base_timestamp = get_timestamp ();
	//GPIO_CLR (GPIO_A_OUT);
#endif

	//__sync_add_and_fetch (&pulse_count[0], 0);
	arr_pulse_count[0] = 0;
	arr_pulse_count[1] = 0;
	arr_pulse_count[2] = 0;
	pulse_count0 = 0;
	pulse_count1 = 0;
	pulse_count2 = 0;
	//__sync_add_and_fetch (&pulse_count[1], 0);
	return 0;
}

void __exit io_sampler_exit (void)
{
	printk (KERN_INFO LKM_NAME ": --- unloading GPIO benchmark test ---\n");
        printk (KERN_INFO LKM_NAME ": count1: %d, count2: %d, count3: %d\n", arr_pulse_count[0], arr_pulse_count[1], arr_pulse_count[2]);
#if (USE_RW_IRQ)
	free_irq (g_gpio_a_irq, NULL);
	free_irq (g_gpio_b_irq, NULL);
	free_irq (g_gpio_c_irq, NULL);
#endif

#if (USE_GPIO_REGS)
	iounmap (g_gpio_regs);
#else
	gpio_free (GPIO_A_IN);
	gpio_free (GPIO_B_IN);
	gpio_free (GPIO_C_IN);
#endif
}

int notify_param(const char *val, const struct kernel_param *kp)
{
        int res = param_set_int(val, kp); // Use helper for write variable
		arr_pulse_count[0] = pulse_count0;
        arr_pulse_count[1] = pulse_count1;
		arr_pulse_count[2] = pulse_count2;
        if(res==0) {
                printk(KERN_INFO "Call back function called...\n");
                printk(KERN_INFO "New value of pulse_count0 = %d\n", pulse_count0);
				printk(KERN_INFO "New value of pulse_count1 = %d\n", pulse_count1);
				printk(KERN_INFO "New value of pulse_count2 = %d\n", pulse_count2);
				printk(KERN_INFO "volatile arr pulse count = %d,%d,%d\n", arr_pulse_count[0],
					arr_pulse_count[1], arr_pulse_count[2]);
				
                return 0;
        }
        return -1;
}
 
const struct kernel_param_ops my_param_ops = 
{
        .set = &notify_param, // Use our setter ...
        .get = &param_get_int, // .. and standard getter
};
 
module_param_cb(pulse_count0, &my_param_ops, &pulse_count0, 0664 );
module_param_cb(pulse_count1, &my_param_ops, &pulse_count1, 0664 );
module_param_cb(pulse_count2, &my_param_ops, &pulse_count2, 0664 );
//..............................................................................

//module_param_array(pulse_count, uint, NULL, 0664);
module_init (io_sampler_init);
module_exit (io_sampler_exit);

MODULE_LICENSE ("GPL v2");
MODULE_DESCRIPTION (LKM_NAME " - kernel module to sample io's on Raspberry Pi");
MODULE_VERSION ("1.3");

//..............................................................................
