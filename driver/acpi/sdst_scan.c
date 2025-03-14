#include <driver/acpi/acpi.h>
#include <minios/memman.h>
#include <minios/assert.h>
#include <minios/console.h>
#include <string.h>

static char *kern_strdup(const char *s) {
    if (s == NULL) { return NULL; }
    const size_t len = strlen(s);
    char *cloned = kern_kmalloc(len + 1);
    if (cloned == NULL) { return NULL; }
    strcpy(cloned, s);
    return cloned;
}

static int do_scan_acpi_sdst_visit_device(lai_nsnode_t *dev, lai_state_t *state, void *arg) {
    if (lai_ns_get_node_type(dev) != LAI_NODETYPE_DEVICE) { return ACPI_VISIT_CONTINUE; }

    UNUSED(arg);
    lai_variable_t var = {};
    lai_var_initialize(&var);

    typedef union {
        struct {
            uint8_t small_item_length : 3;
            uint8_t small_item_name : 4;
        };
        struct {
            uint8_t large_item_name : 7;
            uint8_t large_item : 1;
        };
        uint8_t value;
    } acpi_crs_desc_t;

    const char *large_item_name[] = {
        [0x01] = "24-Bit Memory Range",
        [0x02] = "Generic Register",
        [0x04] = "Vendor-Defined",
        [0x05] = "32-Bit Memory Range",
        [0x06] = "32-Bit Fixed Memory Range",
        [0x07] = "Address Space Resource",
        [0x08] = "Word Address Space",
        [0x09] = "Extended Interrupt",
        [0x0a] = "QWord Address Space",
        [0x0b] = "Extended Address Space",
        [0x0c] = "GPIO Connection",
        [0x0d] = "Pin Function",
        [0x0e] = "GenericSerialBus Connection",
        [0x0f] = "Pin Configuration",
        [0x10] = "Pin Group",
        [0x11] = "Pin Group Function",
        [0x12] = "Pin Group Configuration",
        [0x13] = "Clock Input Resource",
    };
    const char *small_item_name[] = {
        [0x04] = "IRQ Format",
        [0x05] = "DMA Format",
        [0x06] = "Start Dependent Functions",
        [0x07] = "End Dependent Functions",
        [0x08] = "I/O Port",
        [0x09] = "Fixed Location I/O Port",
        [0x0a] = "Fixed DMA",
        [0x0e] = "Vendor Defined",
        [0x0f] = "End Tag",
    };

    const char *dev_hid = NULL;
    const char *dev_uid = NULL;
    int dev_uid_int = -1;
    void *crs_buf = NULL;

    lai_nsnode_t *prop_node = NULL;
    if ((prop_node = lai_ns_get_child(dev, "_HID"))) {
        const auto resp = lai_eval(&var, prop_node, state);
        assert(resp == LAI_ERROR_NONE);
        const int ty = lai_obj_get_type(&var);
        if (ty == LAI_TYPE_STRING) {
            dev_hid = lai_exec_string_access(&var);
        } else if (ty == LAI_TYPE_INTEGER) {
            char eisaid[8] = {};
            lai_eisaid_to_str(var.integer, eisaid);
            dev_hid = eisaid;
        } else {
            unreachable();
        }
        dev_hid = kern_strdup(dev_hid);
    }
    if ((prop_node = lai_ns_get_child(dev, "_UID"))) {
        const auto resp = lai_eval(&var, prop_node, state);
        assert(resp == LAI_ERROR_NONE);
        const int ty = lai_obj_get_type(&var);
        if (ty == LAI_TYPE_STRING) {
            dev_uid = lai_exec_string_access(&var);
        } else if (ty == LAI_TYPE_INTEGER) {
            dev_uid_int = var.integer;
        } else {
            unreachable();
        }
        dev_uid = kern_strdup(dev_uid);
    }
    if ((prop_node = lai_ns_get_child(dev, "_CRS"))) {
        const auto resp = lai_eval(&var, prop_node, state);
        assert(resp == LAI_ERROR_NONE);
        assert(lai_obj_get_type(&var) == LAI_TYPE_BUFFER);
        crs_buf = lai_exec_buffer_access(&var);
    }

    if (dev_hid || dev_uid || dev_uid_int != -1 || crs_buf) {
        const int ws = 2;
        char *path_to_dev = lai_stringify_node_path(dev);
        kprintf("%s {\n", path_to_dev);
#define acpi_put_dev_prop(fmt, value) kprintf("%*s" fmt ",\n", ws, "", (value))
        if (dev_hid) { acpi_put_dev_prop("HID: \"%s\"", dev_hid); }
        if (dev_uid || dev_uid_int != -1) {
            if (dev_uid) {
                acpi_put_dev_prop("UID: \"%s\"", dev_uid);
            } else {
                acpi_put_dev_prop("UID: %d", dev_uid_int);
            }
        }
        if (crs_buf) {
            const acpi_crs_desc_t *desc = crs_buf;
            if (desc->large_item) {
                acpi_put_dev_prop("Type: \"%s\"", large_item_name[desc->large_item_name]);
                switch (desc->large_item_name) {
                    case 0x01: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t _3;
                            uint16_t base;
                            uint16_t limit;
                            uint16_t _4;
                            uint16_t size;
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Base: 0x%p", d->base);
                        acpi_put_dev_prop("Limit: 0x%p", d->limit);
                        acpi_put_dev_prop("Size: %#x", d->size);
                    } break;
                    case 0x02: {
                    } break;
                    case 0x04: {
                    } break;
                    case 0x05: {
                    } break;
                    case 0x06: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t _3;
                            uint32_t base;
                            uint32_t size;
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Base: 0x%p", d->base);
                        acpi_put_dev_prop("Size: %#x", d->size);
                    } break;
                    case 0x07: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t res_type;
                            uint8_t _4;
                            uint8_t _5;
                            uint32_t _6;
                            uint32_t base;
                            uint32_t limit;
                            uint32_t offset;
                            uint32_t size;
                            uint8_t _8;
                            char _9[1];
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Resource Type: %d", d->res_type);
                        acpi_put_dev_prop("Base: 0x%p", d->base);
                        acpi_put_dev_prop("Limit: 0x%p", d->limit);
                        acpi_put_dev_prop("Offset: 0x%p", d->offset);
                        acpi_put_dev_prop("Size: %#x", d->size);
                    } break;
                    case 0x08: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t res_type;
                            uint8_t _4;
                            uint8_t _5;
                            uint16_t _6;
                            uint16_t base;
                            uint16_t limit;
                            uint16_t offset;
                            uint16_t size;
                            uint8_t _8;
                            char _9[1];
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Resource Type: %d", d->res_type);
                        acpi_put_dev_prop("Base: 0x%p", d->base);
                        acpi_put_dev_prop("Limit: 0x%p", d->limit);
                        acpi_put_dev_prop("Offset: 0x%p", d->offset);
                        acpi_put_dev_prop("Size: %#x", d->size);
                    } break;
                    case 0x09: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t int_flags;
                            uint8_t int_nr_count;
                            uint32_t int_nr_table[0];
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Wake Capability: %s",
                                          !!(d->int_flags & BIT(4)) ? "true" : "false");
                        acpi_put_dev_prop("Interrupt Sharing: \"%s\"",
                                          !!(d->int_flags & BIT(3)) ? "Shared" : "Exclusive");
                        acpi_put_dev_prop("Interrupt Polarity: \"%s\"",
                                          !!(d->int_flags & BIT(2)) ? "Active-Low" : "Active-High");
                        acpi_put_dev_prop("Interrupt Mode: \"%s\"", !!(d->int_flags & BIT(1))
                                                                        ? "Edge-Triggered"
                                                                        : "Level-Triggered");
                        acpi_put_dev_prop("Device Role: \"%s\"",
                                          !!(d->int_flags & BIT(0)) ? "Consumer" : "Producer");
                        acpi_put_dev_prop("Total Interrupts: %d", d->int_nr_count);
                        if (d->int_nr_count > 0) {
                            kprintf("%*sInterrupt Numbers: [\n", ws, "");
                            for (int i = 0; i < d->int_nr_count; ++i) {
                                kprintf("%*s%#x,\n", ws * 2, "", d->int_nr_table[i]);
                            }
                            kprintf("%*s],\n", ws, "");
                        }

                    } break;
                    case 0x0a: {
                        struct {
                            uint8_t _1;
                            uint16_t _2;
                            uint8_t res_type;
                            uint8_t _4;
                            uint8_t _5;
                            uint64_t _6;
                            uint64_t base;
                            uint64_t limit;
                            uint64_t offset;
                            uint64_t size;
                            uint8_t _8;
                            char _9[1];
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("Resource Type: %d", d->res_type);
                        acpi_put_dev_prop("Base: 0x%llp", d->base);
                        acpi_put_dev_prop("Limit: 0x%llp", d->limit);
                        acpi_put_dev_prop("Offset: 0x%llp", d->offset);
                        acpi_put_dev_prop("Size: %#llx", d->size);
                    } break;
                    case 0x0b: {
                    } break;
                    case 0x0c: {
                    } break;
                    case 0x0d: {
                    } break;
                    case 0x0e: {
                    } break;
                    case 0x0f: {
                    } break;
                    case 0x10: {
                    } break;
                    case 0x11: {
                    } break;
                    case 0x12: {
                    } break;
                    case 0x13: {
                    } break;
                }
            } else {
                acpi_put_dev_prop("Type: \"%s\"", small_item_name[desc->small_item_name]);
                switch (desc->small_item_name) {
                    case 0x04: {
                        struct {
                            uint8_t _1;
                            uint16_t irq_mask;
                            uint8_t irq_info;
                        } PACKED *d = crs_buf;
                        char bitset[16] = {};
                        for (int i = 0; i < 16; ++i) { bitset[i] = !!(d->irq_mask & BIT(i)) + '0'; }
                        acpi_put_dev_prop("IRQ Mask: 0b%016.16s", bitset);
                        acpi_put_dev_prop("Wake Capability: %s",
                                          !!(d->irq_info & BIT(5)) ? "true" : "false");
                        acpi_put_dev_prop("Interrupt Sharing: \"%s\"",
                                          !!(d->irq_info & BIT(4)) ? "Shared" : "Exclusive");
                        acpi_put_dev_prop("Interrupt Polarity: \"%s\"",
                                          !!(d->irq_info & BIT(3)) ? "Active-Low" : "Active-High");
                        acpi_put_dev_prop("Interrupt Mode: \"%s\"", !!(d->irq_info & BIT(0))
                                                                        ? "Edge-Triggered"
                                                                        : "Level-Triggered");
                    } break;
                    case 0x05: {
                    } break;
                    case 0x06: {
                    } break;
                    case 0x07: {
                    } break;
                    case 0x08: {
                        struct {
                            uint8_t _1;
                            uint8_t _2;
                            uint16_t addr_min;
                            uint16_t addr_max;
                            uint8_t _3;
                            uint8_t len;
                        } PACKED *d = crs_buf;
                        acpi_put_dev_prop("I/O Min Addr: %#x", d->addr_min);
                        acpi_put_dev_prop("I/O Max Addr: %#x", d->addr_max);
                        acpi_put_dev_prop("Total Ports: %d", d->len);
                    } break;
                    case 0x09: {
                    } break;
                    case 0x0a: {
                    } break;
                    case 0x0e: {
                    } break;
                    case 0x0f: {
                    } break;
                }
            }
        }
#undef acpi_put_dev_prop
        kprintf("}\n");
        kern_kfree(path_to_dev);
    }

    if (dev_hid) { kern_kfree((void *)dev_hid); }
    if (dev_uid) { kern_kfree((void *)dev_uid); }

    lai_var_finalize(&var);

    return ACPI_VISIT_CONTINUE;
}

void scan_acpi_sdst() {
    acpi_sdst_visit(do_scan_acpi_sdst_visit_device, NULL);
}
