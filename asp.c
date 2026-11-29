#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/rman.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/bus.h>
#include <machine/resource.h>

struct asp_softc {
	device_t dev;
	bool detaching;

	/* Primary BAR (RID 2) used for register access */
	struct resource *pci_resource;
	int32_t pci_resource_id;
	bus_space_tag_t pci_bus_tag;
	bus_space_handle_t pci_bus_handle;

	/* Secondary BAR (RID 5) apparently used for MSI-X */
	/*
	int pci_resource_id_msix;
	struct resource *pci_resource_msix;
	*/

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

	sc->pci_resource_id = PCIR_BAR(2);
	sc->pci_resource = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
		&sc->pci_resource_id, RF_ACTIVE);
	if (sc->pci_resource == NULL) {
		device_printf(dev, "unable to allocate pci resource\n");
		return (ENODEV);
	}

	/*
	sc->pci_resource_id_msix = PCIR_BAR(5);
	sc->pci_resource_msix = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
		&sc->pci_resource_id_msix, RF_ACTIVATE);
	if (sc->pci_resource_msix == NULL) {
		device_printf(dev, "unable to allocate pci resource\n");
	}
	*/

	sc->pci_bus_tag = rman_get_bustag(sc->pci_resource);
	sc->pci_bus_handle = rman_get_bushandle(sc->pci_resource);
	return (0);
}

static int
asp_ummap_pci_bar(device_t dev)
{
	struct asp_softc *sc;

	sc = device_get_softc(dev);
	
	/*
	bus_release_resource(dev, SYS_RES_MEMORY, sc->pci_resource_id_msix,
		sc->pci_resource_msix);
	*/	
	bus_release_resource(dev, SYS_RES_MEMORY, sc->pci_resource_id,
		sc->pci_resource);
	
	return (0);
}

static int
asp_attach(device_t dev)
{
	struct asp_softc *sc;
	uint32_t val;
	int error;

	sc = device_get_softc(dev);
	sc->dev = dev;

	error = asp_map_pci_bar(dev);
	if (error != 0) {
		device_printf(dev, "%s: couldn't map BAR(s)\n", __func__);
		goto out;
	}

	error = pci_enable_busmaster(dev);
	if (error != 0) {
		device_printf(dev, "%s: couldn't enable busmaster\n", __func__);
		goto out;
	}
	
	val = bus_read_4(sc->pci_resource, 0x00);
	device_printf(dev, "ASP Hardware found! Reg[0x00] = 0x%08x\n", val);

	device_printf(dev, "Memory mapped at physical address: 0x%lx\n", rman_get_start(sc->pci_resource));

out:
	if (error != 0) {
		
	}
	return (error);
}

static int
asp_detach(device_t dev)
{
	// struct asp_softc *sc;

	// sc = device_get_softc(dev);
	
	pci_disable_busmaster(dev);
	asp_ummap_pci_bar(dev);
	return (0);
}

static device_method_t asp_methods[] = {
	DEVMETHOD(device_probe, asp_probe),
	DEVMETHOD(device_attach, asp_attach),
	DEVMETHOD(device_detach, asp_detach),

	DEVMETHOD_END
};

static driver_t asp_driver = {
	"asp",
	asp_methods,
	sizeof(struct asp_softc)
};

DRIVER_MODULE(asp, pci, asp_driver, NULL, NULL);
MODULE_VERSION(asp, 1);
MODULE_DEPEND(asp, pci, 1, 1, 1);
