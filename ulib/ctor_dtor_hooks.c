typedef void (**fn_ptr_t)();
typedef void (*fn_list_t[])();

extern fn_list_t minios_user_ctor_list_start;
extern fn_list_t minios_user_dtor_list_start;
extern fn_list_t minios_user_ctor_dtor_list_end;

void minios_hook_pre_start() {
    const fn_ptr_t start = minios_user_ctor_list_start;
    const fn_ptr_t end = minios_user_dtor_list_start;

    fn_ptr_t ctor_ptr = start;
    while (ctor_ptr < end) {
        (*ctor_ptr)();
        ++ctor_ptr;
    }
}

void minios_hook_post_start() {
    const fn_ptr_t start = minios_user_dtor_list_start;
    const fn_ptr_t end = minios_user_ctor_dtor_list_end;

    fn_ptr_t dtor_ptr = start;
    while (dtor_ptr < end) {
        (*dtor_ptr)();
        ++dtor_ptr;
    }
}
