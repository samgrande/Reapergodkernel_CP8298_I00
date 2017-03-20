SSW_DIR=$(srctree)/drivers/misc/mediatek/ssw/$(subst ",,$(CONFIG_CUSTOM_KERNEL_SSW))
ssw_exist=$(shell if [ -d ${SSW_DIR} ]; then echo "y"; else echo "n"; fi;)
ifeq ($(ssw_exist), y)
obj-y += $(subst ",,$(CONFIG_CUSTOM_KERNEL_SSW))/

else

obj-  := dummy.o # avoid build error
endif

