#ifndef __FAKED_SYSVL_H__
#define __FAKED_SYSVL_H__

#define VL_MAX 256

#ifdef __cplusplus
extern "C" {
#endif
int faked_sysvl_init();
int faked_sys_mkvl(void **prod_head_page_base,
                   void **cons_head_page_base,
                   volatile void **prod_devmem_base,
                   volatile void **cons_devmem_base);
#ifdef __cplusplus
}
#endif

#endif /* __FAKED_SYSVL_H__ */
