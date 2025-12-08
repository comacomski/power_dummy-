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

#define DUMMY_NAME "Dummy_Driver"

// MiAn check what can go in driver struct
static enum power_supply_property dummy_prop[] = {
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
static struct timer_list dummy_timer;
static struct work_struct dummy_work;

static int dummy_get_prop(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{

	pr_err("dummy get prop called; val=%s; int val = %d; prop = %d\n", val->strval, val->intval, psp);
	val->intval = 0;
	return 0;
}

static int dummy_set_prop(struct power_supply *psy, enum power_supply_property psp, const union power_supply_propval *val)
{
	pr_err("dummy set prop called!\n");
	return 0;
}

static int dummy_prop_writeable(struct power_supply *psy, enum power_supply_property psp)
{
    pr_err("DUMMY_WRITEABLE\n");
    switch (psp) {
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
        return 1;  // writable properties
    default:
        return 0;  // read-only
    }
}

// MiAn check what can go in driver struct
static struct power_supply_desc dummy_ps_desc = {
	.name = DUMMY_NAME,
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = dummy_prop,
	.num_properties = 12,
	.get_property = dummy_get_prop,
	.set_property = dummy_set_prop,
	.property_is_writeable = dummy_prop_writeable,

};
static struct power_supply_config dummy_ps_config = {};
static struct power_supply *dummy_ps;

static void timer_callback(struct timer_list *timer)
{
	pr_err("Dummy Timer works");
	schedule_work(&dummy_work);
	mod_timer(&dummy_timer, jiffies + secs_to_jiffies(1));
}

static void dummy_working(struct work_struct *work)
{
	pr_err("Work queue called\n");
}

static int dummy_probe(struct platform_device *pdev)
{
	// MiAn proper error handling
	int ret = 0;
	dev_info(&pdev->dev, "Module probe called.\n");
	dummy_ps = devm_power_supply_register(&pdev->dev, &dummy_ps_desc, &dummy_ps_config);
	if(IS_ERR(dummy_ps))
	{
		dev_err(&pdev->dev, "Error while dev_power_supply_register:%ld\n", PTR_ERR(dummy_ps));
		ret = PTR_ERR(dummy_ps);
		goto reg_err;
	}
	dev_err(&pdev->dev, "Probe success! \n");
	
	timer_setup(&dummy_timer, timer_callback, 0);
	INIT_WORK(&dummy_work, dummy_working);
	mod_timer(&dummy_timer, jiffies + secs_to_jiffies(1));
reg_err:
	return ret;
}
static void dummy_remove(struct platform_device *pdev)
{
	pr_err("Dummy module remove called.\n");
	timer_delete_sync(&dummy_timer);
}

// MiAn check what can go in driver struct
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

struct platform_device *pdev;

static int dummy_ps_init(void)
{
	pr_err("Dummy module init started...\n");
	platform_driver_register(&dummy_drv);
	// hack, delete
	pdev = platform_device_register_simple(DUMMY_NAME, -1, NULL, 0);
	
	return 0;
}
static void dummy_ps_exit(void)
{
	// MiAn check return values
    	platform_device_unregister(pdev);
    	platform_driver_unregister(&dummy_drv);
	pr_err("Dummy module exit \n");
	
}

module_init(dummy_ps_init);
module_exit(dummy_ps_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mico Antonic");
MODULE_DESCRIPTION("Dummy Power Supply driver example");

