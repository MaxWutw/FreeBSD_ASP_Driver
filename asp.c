#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include "asp.h"

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

	/* ASP register */
	bus_size_t		reg_cmdresp;
	bus_size_t		reg_addr_lo;
	bus_size_t		reg_addr_hi;
};

struct pciid {
	uint32_t devid;
	const char *desc;
} asp_ids[] = {
	{ 0x14861022, "AMD Secure Processor (Milan)" },
	{ 0x00000000, NULL }
};

int sev_init(struct asp_softc *sc);
int sev_shutdown(struct asp_softc *sc);
int sev_get_platform_status(struct asp_softc *sc);


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
	/* softc get base register */
	sc->reg_cmdresp = ASP_REG_CMDRESP;
	sc->reg_addr_lo  = ASP_REG_ADDR_LO;
	sc->reg_addr_hi  = ASP_REG_ADDR_HI;

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

	sev_init(sc);

	/* Test for get SEV platform status */
	// struct sev_platform_status *status;
	error = sev_get_platform_status(sc);
	if (error != 0) {
		device_printf(dev, "%s: Failed to get SEV platform status\n", __func__);
		goto out;
	}

	sev_shutdown(sc);
	error = sev_get_platform_status(sc);
	if (error != 0) {
		device_printf(dev, "%s: Failed to get SEV platform status\n", __func__);
		goto out;
	}

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

static int
asp_send_cmd(struct asp_softc *sc, uint32_t cmd, uint64_t paddr)
{
	uint32_t reg_val, paddr_hi, paddr_lo, cmd_id;
	// for non blocking mode
	int32_t timeout = 1000;

	paddr_lo = paddr & 0xffffffff;
	paddr_hi = (paddr >> 32) & 0xffffffff;
	cmd_id = (cmd & 0x3ff) << 16;
	device_printf(sc->dev, "High address: 0x%x\n", paddr_hi);
	device_printf(sc->dev, "Low address: 0x%x\n", paddr_lo);

	bus_write_4(sc->pci_resource, sc->reg_addr_lo, paddr_lo);
	bus_write_4(sc->pci_resource, sc->reg_addr_hi, paddr_hi);
	bus_write_4(sc->pci_resource, sc->reg_cmdresp, cmd_id);

	/* busy waiting polling (temporarily) */
	while (timeout > -1) {
		reg_val = bus_read_4(sc->pci_resource, sc->reg_cmdresp);
		if (reg_val & ASP_CMDRESP_RESPONSE)
			break;
		DELAY(5000);
		timeout--;
	}

	if (timeout == 0) {
		device_printf(sc->dev, "ASP Command Timeout!\n");
		return (ETIMEDOUT);
	}

	device_printf(sc->dev, "ASP Command finished. Result: 0x%x\n", reg_val);
	device_printf(sc->dev, "Timeout left: %d\n", timeout);
	return (0);
}

int
sev_init(struct asp_softc *sc)
{
	struct sev_init *init;
	int error;

	init = contigmalloc(sizeof(struct sev_init), M_DEVBUF, M_NOWAIT | M_ZERO,
						0, ~0UL, PAGE_SIZE, 0);
	if (init == NULL) {
		return (ENOMEM);
	}
	
	/* For AMD SEV, currently we does not enable SEV-ES */
	bzero(init, sizeof(struct sev_init));
	uint64_t paddr = vtophys(init);
	device_printf(sc->dev, "SEV init physical address: 0x%lx\n", paddr);

	error = asp_send_cmd(sc, SEV_CMD_INIT, paddr);
	if (error)
		return (error);

	device_printf(sc->dev, "SEV init finished!\n");
	contigfree(init, sizeof(struct sev_init), M_DEVBUF);

	return (0);
}

int
sev_shutdown(struct asp_softc *sc)
{
	int error;

	error = asp_send_cmd(sc, SEV_CMD_SHUTDOWN, 0x0);
	if (error)
		return (error);

	return (0);
}

int
sev_get_platform_status(struct asp_softc *sc)
{
	struct sev_platform_status *status_data;
	int error;

	status_data = contigmalloc(sizeof(struct sev_platform_status), M_DEVBUF, M_NOWAIT | M_ZERO,
							   0, ~0UL, PAGE_SIZE, 0);
	if (status_data == NULL) {
		return (ENOMEM);
	}

	uint64_t paddr = vtophys(status_data);
	device_printf(sc->dev, "Platform status physical address: 0x%lx\n", paddr);

	error = asp_send_cmd(sc, SEV_CMD_PLATFORM_STATUS, paddr);
	if (error)
		return (error);

	device_printf(sc->dev, "SEV status:\n");
	device_printf(sc->dev, "	API version: %d.%d\n", status_data->api_major, status_data->api_minor);
	device_printf(sc->dev, "	State: %d\n", status_data->state);
	device_printf(sc->dev, "	Guests: %d\n", status_data->guest_count);
	
	contigfree(status_data, sizeof(struct sev_platform_status), M_DEVBUF);

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
