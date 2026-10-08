#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>

struct asp_softc {
	device_t dev;
	bool detaching;

	/* BAR0 MMIO */
	struct resource *pci_resource;
	int32_t pci_resource_id;
	bus_space_tag_t pci_bus_tag;
	bus_space_handle_t pci_bus_handle;
};

struct pciid {
	uint32_t devid;
	const char *desc;
} asp_ids[] = {
	{ 0x14861022, "AMD Secure Processor (Milan)" },
	{ 0x00000000, NULL }
};

static int
asp_probe(device_t dev)
{
	struct pciid *ip;
	uint32_t id;
	id = pci_get_devid(dev);
	
	for (ip = asp_ids; ip < &asp_ids[nitems(asp_ids)]; ip++) {
		if (id == ip->devid) {
			device_set_desc(dev, ip->desc);
			return 0;
		}
	}
	return (ENXIO);
}

static int
asp_map_pci_bar(device_t dev)
{
	struct asp_softc *sc;
	sc = device_get_softc(dev);


}

static int
asp_attach(device_t dev)
{

}

static int
asp_detach(device_t dev)
{
	struct asp_softc *sc;

	sc = device_get_softc(dev);

}
