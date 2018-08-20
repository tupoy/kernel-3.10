include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/gslX680/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/gslX680/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-y	+=  mtk_gslX680_6589_6572.o
obj-y   +=  gsl_point_id.o

