#include <driver/acpi/acpi.h>
#include <minios/assert.h>
#include <string.h>

void init_acpi() {
    lai_create_namespace();
}

typedef struct {
    lai_nsnode_t *node;
    lai_state_t *state;
    acpi_sdst_visit_t visit;
    void *arg;
    bool done;
} acpi_sdst_visit_ds_t;

static void acpi_sdst_visit_impl(acpi_sdst_visit_ds_t *d) {
    const int state = d->visit(d->node, d->state, d->arg);
    if (state == ACPI_VISIT_STOP) { d->done = true; }
    if (state != ACPI_VISIT_CONTINUE) { return; }

    lai_nsnode_t *node = d->node;
    struct lai_ns_child_iterator iter = {};
    lai_initialize_ns_child_iterator(&iter, node);
    while (!d->done && (d->node = lai_ns_child_iterate(&iter))) { acpi_sdst_visit_impl(d); }
    d->node = node;
}

void acpi_sdst_visit(acpi_sdst_visit_t visit, void *arg) {
    lai_state_t state = {};
    lai_init_state(&state);
    acpi_sdst_visit_ds_t d = {
        .node = lai_ns_get_root(),
        .state = &state,
        .visit = visit,
        .arg = arg,
        .done = false,
    };
    acpi_sdst_visit_impl(&d);
    lai_finalize_state(&state);
}

typedef struct {
    const char *hw_id;
    acpi_device_visit_t visit;
    void *arg;
} acpi_find_device_ds_t;

static int acpi_find_device_visit_wrapper(lai_nsnode_t *node, lai_state_t *state, void *arg) {
    int visit_state = ACPI_VISIT_CONTINUE;
    if (lai_ns_get_node_type(node) != LAI_NODETYPE_DEVICE) { return visit_state; }

    acpi_find_device_ds_t *d = arg;

    char hid[32] = {};
    lai_nsnode_t *prop_node = NULL;

    lai_variable_t var = {};
    lai_var_initialize(&var);

    if ((prop_node = lai_ns_get_child(node, "_HID"))) {
        const auto resp = lai_eval(&var, prop_node, state);
        assert(resp == LAI_ERROR_NONE);
        const int ty = lai_obj_get_type(&var);
        if (ty == LAI_TYPE_STRING) {
            strcpy(hid, lai_exec_string_access(&var));
        } else if (ty == LAI_TYPE_INTEGER) {
            lai_eisaid_to_str(var.integer, hid);
        } else {
            unreachable();
        }
    }

    if (strcmp(hid, d->hw_id) == 0) {
        void *crs_buf = NULL;
        if ((prop_node = lai_ns_get_child(node, "_CRS"))) {
            const auto resp = lai_eval(&var, prop_node, state);
            assert(resp == LAI_ERROR_NONE);
            assert(lai_obj_get_type(&var) == LAI_TYPE_BUFFER);
            crs_buf = lai_exec_buffer_access(&var);
        }
        visit_state = d->visit(node, state, d->arg, crs_buf);
    }

    lai_var_finalize(&var);

    return visit_state;
}

void acpi_find_device(const char *name, acpi_device_visit_t visit, void *arg) {
    acpi_find_device_ds_t d = {
        .hw_id = name,
        .visit = visit,
        .arg = arg,
    };
    acpi_sdst_visit(acpi_find_device_visit_wrapper, &d);
}
