/*
 * QEMU 3DFX SST-1 (Voodoo Graphics) emulation
 *
 * This work is licensed under the GNU GPL license version 2 or later.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "qapi/error.h"
#include "hw/pci/pci.h"

#define TYPE_SST1_PCI "sst1-pci"
#define SST1_PCI_BASE(obj) \
    OBJECT_CHECK(SST1State, (obj), TYPE_SST1_PCI)

#define PCI_VENDOR_ID_3DFX          0x121a
#define PCI_DEVICE_ID_3DFX_VOODOO   0x0001
#define PCI_DEVICE_ID_3DFX_VOODOO2  0x0002

#define SST1_MMIO_SIZE_MB           4
#define SST1_FRAME_BUFFER_SIZE_MB   4
#define SST1_TEXTURE_MEMORY_SIZE_MB 8

typedef struct SST1State
{
    PCIDevice dev;

    MemoryRegion mapped_base;
    MemoryRegion mmio;
    MemoryRegion frame_buffer;
    MemoryRegion texture_mem;
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

static void sst1_reset(DeviceState *dev)
{
    // TODO
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
