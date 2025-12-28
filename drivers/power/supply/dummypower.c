#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/device/driver.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/container_of.h>
#include <linux/array_size.h>
#include <linux/property.h>
#include <linux/mutex.h>
#include <linux/dev_printk.h>

#define DUMMY_NAME "Dummy_Driver"

struct dummy_psy_props {
    int status;
    int health;
    bool present;
    bool online;

    int constant_charge_current;
    int constant_charge_current_max;
    int constant_charge_voltage;
    int constant_charge_voltage_max;

    int scope;
    int charge_term_current;

    const char *model_name;
    const char *manufacturer;
};

struct dummy_data {
	struct timer_list dummy_timer;
	struct work_struct dummy_work;
	struct platform_device *dummy_platform_dev;
	struct power_supply_config dummy_ps_config;
	struct power_supply *dummy_ps;
	struct dummy_psy_props dummy_vals;
	struct mutex dummy_mutex;
};

static const enum power_supply_property dummy_prop[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,	
};

static int dummy_get_prop(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{

	struct dummy_data *dummy_d = (struct dummy_data *)power_supply_get_drvdata(psy); 
	dev_info(&dummy_d->dummy_platform_dev->dev, "vdummy get prop called on prop = %d\n", psp);

	mutex_lock(&dummy_d->dummy_mutex);
	switch(psp)
	{
	case POWER_SUPPLY_PROP_STATUS:
 		val->intval = dummy_d->dummy_vals.status;
	break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = dummy_d->dummy_vals.health;
	break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = dummy_d->dummy_vals.present;
	break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = dummy_d->dummy_vals.online;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = dummy_d->dummy_vals.constant_charge_current;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = dummy_d->dummy_vals.constant_charge_current_max;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		val->intval = dummy_d->dummy_vals.constant_charge_voltage;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = dummy_d->dummy_vals.constant_charge_voltage_max;
	break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = dummy_d->dummy_vals.scope;
	break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		val->intval = dummy_d->dummy_vals.charge_term_current;
	break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = dummy_d->dummy_vals.model_name;
	break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = dummy_d->dummy_vals.manufacturer;
	break;
	default:
		mutex_unlock(&dummy_d->dummy_mutex);
		dev_err(&dummy_d->dummy_platform_dev->dev, "not available config choosen!\n");
		return -EINVAL;
	}
	mutex_unlock(&dummy_d->dummy_mutex);
	return 0;
}

static int dummy_set_prop(struct power_supply *psy, enum power_supply_property psp, const union power_supply_propval *val)
{
	struct dummy_data *dummy_d = (struct dummy_data *)power_supply_get_drvdata(psy);
	dev_debug(&dummy_d->dummy_platform_dev->dev, "dummy set prop: %d to value: %d\n", psp, val->intval);

	mutex_lock(&dummy_d->dummy_mutex);
	switch(psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		dummy_d->dummy_vals.constant_charge_current = val->intval;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		dummy_d->dummy_vals.constant_charge_current_max = val->intval;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		dummy_d->dummy_vals.constant_charge_voltage = val->intval;
	break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		dummy_d->dummy_vals.constant_charge_voltage_max = val->intval;
	break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		dummy_d->dummy_vals.charge_term_current = val->intval;
	break;
	default:
		mutex_unlock(&dummy_d->dummy_mutex);
		dev_err(&dummy_d->dummy_platform_dev->dev, "%d is not writable!\n", psp);
		return -EINVAL;
	}
	mutex_unlock(&dummy_d->dummy_mutex);
	return 0;
}

static int dummy_prop_writeable(struct power_supply *psy, enum power_supply_property psp)
{
    struct dummy_data *dummy_d = (struct dummy_data *)power_supply_get_drvdata(psy);
    dev_debug(&dummy_d->dummy_platform_dev->dev, "dummy_prop_writeable called!\n");

    switch (psp) {
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
        return 1;  /* writable properties */
    default:
        return 0;  /* read-only */
    }
}

static const struct power_supply_desc dummy_ps_desc = {
    .name = DUMMY_NAME,
    .type = POWER_SUPPLY_TYPE_UNKNOWN,
    .properties = dummy_prop,
    .num_properties = ARRAY_SIZE(dummy_prop),
    .get_property = dummy_get_prop,
    .set_property = dummy_set_prop,
    .property_is_writeable = dummy_prop_writeable,
};


static void timer_callback(struct timer_list *timer)
{
	struct dummy_data *dummy_d = container_of(timer, struct dummy_data, dummy_timer);
	dev_debug(&dummy_d->dummy_platform_dev->dev, "Dummy Timer works");

	mod_timer(timer, jiffies + secs_to_jiffies(1));
	schedule_work(&dummy_d->dummy_work);
}

static void dummy_working(struct work_struct *work)
{
	/* MiAn proper log levels everywhere */
	struct dummy_data *dummy_d = (struct dummy_data *)container_of(work, struct dummy_data, dummy_work);	
	dev_debug(&dummy_d->dummy_platform_dev->dev, "Work queue called\n");

	mutex_lock(&dummy_d->dummy_mutex);
	switch(dummy_d->dummy_vals.status) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		dummy_d->dummy_vals.status = POWER_SUPPLY_STATUS_CHARGING;
		dummy_d->dummy_vals.health = POWER_SUPPLY_HEALTH_GOOD;
		mutex_unlock(&dummy_d->dummy_mutex);
		dev_info(&dummy_d->dummy_platform_dev->dev, "Start of charging!\n");
		mutex_lock(&dummy_d->dummy_mutex);	
	break;
	case POWER_SUPPLY_STATUS_CHARGING:
		dummy_d->dummy_vals.online = true;
		dummy_d->dummy_vals.health = POWER_SUPPLY_HEALTH_GOOD;
		dummy_d->dummy_vals.constant_charge_current += 100000; // increment
		if(dummy_d->dummy_vals.constant_charge_current >= dummy_d->dummy_vals.constant_charge_current_max) {
            		dummy_d->dummy_vals.status = POWER_SUPPLY_STATUS_DISCHARGING;	
			mutex_unlock(&dummy_d->dummy_mutex);
			dev_info(&dummy_d->dummy_platform_dev->dev, "End of charging, going to discharging!\n");
			mutex_lock(&dummy_d->dummy_mutex);
		}
	break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		dummy_d->dummy_vals.online = false;
		dummy_d->dummy_vals.constant_charge_current -= 100000; // decrement
		if(dummy_d->dummy_vals.constant_charge_current <= 0)
		{
			dummy_d->dummy_vals.status = POWER_SUPPLY_STATUS_CHARGING;
			mutex_unlock(&dummy_d->dummy_mutex);
			dev_info(&dummy_d->dummy_platform_dev->dev, "Discharged fully, going to charging\n!");
			mutex_lock(&dummy_d->dummy_mutex);
		}
	break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
	case POWER_SUPPLY_STATUS_FULL:
	default:
		mutex_unlock(&dummy_d->dummy_mutex);
		dev_err(&dummy_d->dummy_platform_dev->dev, "Unsupported state: %d\n", dummy_d->dummy_vals.status);
		mutex_lock(&dummy_d->dummy_mutex);
	break;
	}
	mutex_unlock(&dummy_d->dummy_mutex);
}

static int dummy_probe(struct platform_device *pdev)
{
	/* MiAn proper error handling */
	int ret = 0;
	dev_info(&pdev->dev, "Module probe called.\n");
	struct dummy_data *dummy_d = devm_kzalloc(&pdev->dev, sizeof(struct dummy_data), GFP_KERNEL);
	if(!dummy_d)
	{
		dev_info(&pdev->dev, "Error while accocating memory: %d\n", ret);
		return -ENOMEM;
	}

	dummy_d->dummy_ps_config.drv_data = dummy_d;
	dummy_d->dummy_ps_config.fwnode = dev_fwnode(&pdev->dev);	

	platform_set_drvdata(pdev, (void *)dummy_d);

	dummy_d->dummy_ps = devm_power_supply_register(&pdev->dev, &dummy_ps_desc, &dummy_d->dummy_ps_config);
	if(IS_ERR(dummy_d->dummy_ps))
	{

		ret = PTR_ERR(dummy_d->dummy_ps);
		dev_err(&pdev->dev, "Error while dev_power_supply_register:%d\n", ret);
		goto alloc_reg_err;
	}
	dev_info(&pdev->dev, "Probe success! \n");
	
	/* Initial values for DUMMY PS */
	dummy_d->dummy_vals.status = POWER_SUPPLY_STATUS_UNKNOWN;
	dummy_d->dummy_vals.health = POWER_SUPPLY_HEALTH_UNKNOWN;
	dummy_d->dummy_vals.present = true;
	dummy_d->dummy_vals.online = true;
	dummy_d->dummy_vals.constant_charge_current = 1500000; /* uA */
	dummy_d->dummy_vals.constant_charge_current_max = 3000000; /* uA */
	dummy_d->dummy_vals.constant_charge_voltage = 4200000; /* uV */
	dummy_d->dummy_vals.constant_charge_voltage_max = 4400000; /* uV */
	dummy_d->dummy_vals.scope = POWER_SUPPLY_SCOPE_SYSTEM;
	dummy_d->dummy_vals.charge_term_current = 100000; /* uA */
	dummy_d->dummy_vals.model_name = "Edu123";
	dummy_d->dummy_vals.manufacturer = "MiAn_electronics";
	
	mutex_init(&dummy_d->dummy_mutex);	
	timer_setup(&dummy_d->dummy_timer, timer_callback, 0);
	INIT_WORK(&dummy_d->dummy_work, dummy_working);
	mod_timer(&dummy_d->dummy_timer, jiffies + secs_to_jiffies(1));

/* MiAn proper error handling */

alloc_reg_err:
	return ret;
}
static void dummy_remove(struct platform_device *pdev)
{
	struct dummy_data *dummy_d = (struct dummy_data*)platform_get_drvdata(pdev);
	dev_info(&dummy_d->dummy_platform_dev->dev, "Dummy module remove called.\n");
	timer_delete_sync(&dummy_d->dummy_timer);
	cancel_work_sync(&dummy_d->dummy_work);
}

static const struct of_device_id dummy_of[] = {
	{ .compatible = "dt_string"},
	{}
};
MODULE_DEVICE_TABLE(of, dummy_of);

static const struct acpi_device_id dummy_acpi[] = {
    { "ABC1234", 0 },
    { }
};
MODULE_DEVICE_TABLE(acpi, dummy_acpi);

struct platform_driver dummy_drv = {
	.probe = dummy_probe,
	.remove = dummy_remove,
	.driver = {
		.name = DUMMY_NAME,
		.owner = THIS_MODULE,
		.of_match_table = dummy_of,
		.acpi_match_table = ACPI_PTR(dummy_acpi),
	},
	
};

/* MiAn: testing purposes, delete */
struct platform_device *pdev;

static int dummy_ps_init(void)
{
	pr_info("Dummy_Driver module init started...\n");
	platform_driver_register(&dummy_drv);

	/* MiAn: testing purposes, delete */
	pdev = platform_device_register_simple(DUMMY_NAME, -1, NULL, 0);
	
	return 0;
}
static void dummy_ps_exit(void)
{
	/* MiAn check return values */
    	platform_device_unregister(pdev);
    	platform_driver_unregister(&dummy_drv);
	pr_info("Dummy module exit \n");
	
}

module_init(dummy_ps_init);
module_exit(dummy_ps_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mico Antonic");
MODULE_DESCRIPTION("Dummy Power Supply driver example");


