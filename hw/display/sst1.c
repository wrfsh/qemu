/*
 * QEMU 3DFX SST-1 (Voodoo Graphics) emulation
 *
 * This work is licensed under the GNU GPL license version 2 or later.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/range.h"
#include "qom/object.h"
#include "qapi/error.h"
#include "hw/pci/pci.h"

#define DEBUG_SST1

#ifdef DEBUG_SST1
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

#define TYPE_SST1_PCI "sst1-pci"
#define SST1_PCI_BASE(obj) \
    OBJECT_CHECK(SST1State, (obj), TYPE_SST1_PCI)

#define PCI_VENDOR_ID_3DFX          0x121a
#define PCI_DEVICE_ID_3DFX_VOODOO   0x0001
#define PCI_DEVICE_ID_3DFX_VOODOO2  0x0002

#define SST1_MMIO_SIZE_MB           4
#define SST1_FRAME_BUFFER_SIZE_MB   4
#define SST1_TEXTURE_MEMORY_SIZE_MB 8

#define SST1_PCI_INIT_ENABLE        0x40

/**
 * SST1 device state
 */
typedef struct SST1State
{
    PCIDevice dev;

    MemoryRegion mapped_base;
    MemoryRegion mmio;
    MemoryRegion frame_buffer;
    MemoryRegion texture_mem;

    bool pci_fifo_enable;
    bool fbiinit_enable;
    bool remap_fbiinit23;
} SST1State;

static uint64_t sst1_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void sst1_mmio_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
}

static const struct MemoryRegionOps sst1_mmio_ops = {
    .read = sst1_mmio_read,
    .write = sst1_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
};

static void sst1_update_init_enable(SST1State *s, uint32_t val)
{
    /* bit 0: enable writes to fbiinit registers */
    s->fbiinit_enable = val & 0x1;

    /* bit 1: enable writes to PCI FIFO */
    s->pci_fifo_enable = val & 0x2;

    /* bit 2: remap {fbiinit2, fbiinit3} to {dacRead videoChecksum} */
    s->remap_fbiinit23 = val & 0x4;

    /* TODO */
    assert((val & ~0x7) == 0);
}

static void sst1_pci_config_write(PCIDevice *dev, uint32_t addr, uint32_t val, int l)
{
    SST1State *s = SST1_PCI_BASE(dev);

    pci_default_write_config(dev, addr, val, l);

    if (ranges_overlap(addr, l, SST1_PCI_INIT_ENABLE, 4)) {
        DPRINTF("%s: addr = %#x, val = %#x, len = %d\n", __func__, addr, val, l);

        sst1_update_init_enable(s,
                pci_get_long(dev->config + SST1_PCI_INIT_ENABLE));
    }
}

static void sst1_reset(DeviceState *dev)
{
    SST1State *s = SST1_PCI_BASE(dev);

    pci_set_long(s->dev.config + SST1_PCI_INIT_ENABLE, 0);
    sst1_update_init_enable(s, 0);
}

static void sst1_realize(PCIDevice *dev, Error **errp)
{
    SST1State *s = SST1_PCI_BASE(dev);

    memory_region_init_io(&s->mapped_base, OBJECT(dev), NULL, NULL, "sst1.mapped_base",
            (SST1_MMIO_SIZE_MB + SST1_FRAME_BUFFER_SIZE_MB + SST1_TEXTURE_MEMORY_SIZE_MB) << 20);

    memory_region_init_io(&s->mmio, OBJECT(dev), &sst1_mmio_ops, s, "sst1.mmio",
            SST1_MMIO_SIZE_MB << 20);

    memory_region_init_ram(&s->frame_buffer, OBJECT(dev), "sst1.frame_buffer",
            SST1_FRAME_BUFFER_SIZE_MB << 20, &error_abort);

    memory_region_init_ram(&s->texture_mem, OBJECT(dev), "sst1.texture_mem",
            SST1_TEXTURE_MEMORY_SIZE_MB << 20, &error_abort);

    memory_region_add_subregion(&s->mapped_base, 0, &s->mmio);
    memory_region_add_subregion(&s->mapped_base, SST1_MMIO_SIZE_MB << 20, &s->frame_buffer);
    memory_region_add_subregion(&s->mapped_base, (SST1_MMIO_SIZE_MB + SST1_FRAME_BUFFER_SIZE_MB) << 20, &s->texture_mem);

    pci_register_bar(dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mapped_base);
}

static void sst1_exit(PCIDevice *dev)
{
}

static void sst1_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->reset = sst1_reset;
    dc->hotpluggable = false;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);

    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    k->class_id = PCI_CLASS_DISPLAY_OTHER;
    k->vendor_id = PCI_VENDOR_ID_3DFX;
    k->device_id = PCI_DEVICE_ID_3DFX_VOODOO;
    k->config_write = sst1_pci_config_write;
    k->realize = sst1_realize;
    k->exit = sst1_exit;
}

static const TypeInfo sst1_info = {
    .name = TYPE_SST1_PCI,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(SST1State),
    .class_init = sst1_class_init,
    .interfaces = (InterfaceInfo[]) {
          { INTERFACE_CONVENTIONAL_PCI_DEVICE },
          { },
    },
};

static void sst1_register_types(void)
{
    type_register_static(&sst1_info);
}

type_init(sst1_register_types)
