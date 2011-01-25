#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pspsdk.h>
#include <pspthreadman_kernel.h>
#include "systemctrl.h"
#include "systemctrl_se.h"
#include "systemctrl_private.h"
#include "isoreader.h"
#include "printk.h"
#include "utils.h"
#include "strsafe.h"
#include "virtual_pbp.h"

typedef struct __attribute__((packed))
{
	u32 signature;
	u32 version;
	u32 fields_table_offs;
	u32 values_table_offs;
	int nitems;
} SFOHeader;

typedef struct __attribute__((packed))
{
	u16 field_offs;
	u8  unk;
	u8  type; // 0x2 -> string, 0x4 -> number
	u32 unk2;
	u32 unk3;
	u16 val_offs;
	u16 unk4;
} SFODir;

typedef struct _PBPEntry {
	u32 enabled;
	char *name;
} PBPEntry;

static PBPEntry pbp_entries[8] = {
	{ 1, "PARAM.SFO" },
	{ 1, "ICON0.PNG" },
	{ 1, "ICON1.PMF" },
	{ 1, "PIC0.PNG"  },
	{ 1, "PIC1.PNG"  },
	{ 1, "SND0.AT3"  },
	{ 0, "DATA.PSP"  }, // never enable it
	{ 0, "DATA.PSAR" }, // never enable it
};

static u8 virtualsfo[408] = {
	0x00, 0x50, 0x53, 0x46, 0x01, 0x01, 0x00, 0x00,
	0x94, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 
	0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x04, 0x02, 
	0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x12, 0x00, 0x04, 0x02, 
	0x0A, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 
	0x08, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x04, 0x02, 
	0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x00, 0x27, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
	0x20, 0x00, 0x00, 0x00, 0x36, 0x00, 0x04, 0x02,
	0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
	0x24, 0x00, 0x00, 0x00, 0x45, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
	0x2C, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x04, 0x02, 
	0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
	0x30, 0x00, 0x00, 0x00, 0x42, 0x4F, 0x4F, 0x54, 
	0x41, 0x42, 0x4C, 0x45, 0x00, 0x43, 0x41, 0x54, 
	0x45, 0x47, 0x4F, 0x52, 0x59, 0x00, 0x44, 0x49, 
	0x53, 0x43, 0x5F, 0x49, 0x44, 0x00, 0x44, 0x49, 
	0x53, 0x43, 0x5F, 0x56, 0x45, 0x52, 0x53, 0x49,
	0x4F, 0x4E, 0x00, 0x50, 0x41, 0x52, 0x45, 0x4E, 
	0x54, 0x41, 0x4C, 0x5F, 0x4C, 0x45, 0x56, 0x45, 
	0x4C, 0x00, 0x50, 0x53, 0x50, 0x5F, 0x53, 0x59, 
	0x53, 0x54, 0x45, 0x4D, 0x5F, 0x56, 0x45, 0x52, 
	0x00, 0x52, 0x45, 0x47, 0x49, 0x4F, 0x4E, 0x00, 
	0x54, 0x49, 0x54, 0x4C, 0x45, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x4D, 0x47, 0x00, 0x00, 
	0x55, 0x43, 0x4A, 0x53, 0x31, 0x30, 0x30, 0x34, 
	0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x31, 0x2E, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x31, 0x2E, 0x30, 0x30, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
	0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 
	0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 
	0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
	0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 
	0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

VirtualPBP *g_vpbps = NULL;
int g_vpbps_cnt = 0;
static int g_sema = -1;
static int g_gamedfd = -1;
static VirtualPBP *g_caches = NULL;
static u32 g_caches_cnt;
static u8 g_referenced[32];
static u8 g_need_update = 0;

static int is_iso(SceIoDirent * dir)
{
	//result
	int result = 0;

	//grab extension
	char * ext = dir->d_name + strlen(dir->d_name) - 3;

	//filename length check
	if (ext > dir->d_name) {
		//check extension
		if (stricmp(ext, "iso") == 0 /* || stricmp(ext, "cso") */) {
			//valid iso detected (more checks can be added here lateron)
			result = 1;
		}
	}

	return result;
}

static int get_vpbp_by_path(const char *path)
{
	char *p;
	u32 isoindex;

	p = strstr(path, "ISOGAME");

	if (p == NULL) {
		return -17;
	}

	p += sizeof("ISOGAME")-1;
	isoindex = strtoul(p, NULL, 16);

	if (isoindex >= g_vpbps_cnt) {
		return -18;
	}

	return isoindex;
}

static inline void lock(void)
{
	sceKernelWaitSema(g_sema, 1, 0);
}

static inline void unlock(void)
{
	sceKernelSignalSema(g_sema, 1);
}

static VirtualPBP* get_vpbp_by_fd(SceUID fd)
{
	fd -= MAGIC_VPBP_FD;

	if (fd < 0 || fd >= g_vpbps_cnt) {
		return NULL;
	}

	return &g_vpbps[fd];
}

static void get_sfo_title(char *title, int n, char *sfo)
{
	SFOHeader *header = (SFOHeader *)sfo;
	SFODir *entries = (SFODir *)(sfo+0x14);
	int i;

	for (i = 0; i < header->nitems; i++) {
		if (0 == strcmp(sfo+header->fields_table_offs+entries[i].field_offs, "TITLE")) {
			memset(title, 0, n);
			strncpy(title, sfo+header->values_table_offs+entries[i].val_offs, n);
		}
	}
}

static int disable_cache(int idx)
{
	VirtualPBP *vpbp;
	
	if (idx < 0 || idx >= g_vpbps_cnt) {
		unlock();

		return -15;
	}

	vpbp = &g_vpbps[idx];
	vpbp->enabled = 0;

	return 0;
}

static int add_cache(VirtualPBP *vpbp)
{
	int i;

	if (vpbp == NULL || !vpbp->enabled) {
		return -22;
	}

	for(i=0; i<g_caches_cnt; ++i) {
		if(!g_caches[i].enabled) {
			memcpy(&g_caches[i], vpbp, sizeof(*vpbp));
			g_referenced[i] = 1;
			g_need_update = 1;

			return 1;
		}
	}

	return 0;
}

static int load_cache(void)
{
	int i, fd, ret;
	char path[] = CACHE_PATH;
	u32 magic;

	memset(g_caches, 0, sizeof(g_caches[0]) * g_caches_cnt);

	for(i=0; i<3; ++i) {
		fd = sceIoOpen(path, PSP_O_RDONLY, 0777);

		if (fd >= 0) {
			break;
		}

		printk("%s: sceIoOpen returns 0x%08X\n", __func__, fd);
		strncpy(path, "ef", sizeof("ef")-1);

		fd = sceIoOpen(path, PSP_O_RDONLY, 0777);

		if (fd >= 0) {
			break;
		}

		printk("%s: sceIoOpen returns 0x%08X\n", __func__, fd);
		strncpy(path, "ms", sizeof("ms")-1);
	}

	if (fd < 0) {
		return -24;
	}

	ret = sceIoRead(fd, &magic, sizeof(magic));

	if (ret != sizeof(magic) && magic != MAGIC_ISOCACHE) {
		return -25;
	}

	sceIoRead(fd, g_caches, g_caches_cnt*sizeof(g_caches[0]));
	memset(g_referenced, 0, sizeof(g_referenced));
	sceIoClose(fd);

	return 0;
}

static int save_cache(void)
{
	int i;
	SceUID fd;
	char path[] = CACHE_PATH;
	u32 magic = MAGIC_ISOCACHE;

	for(i=0; i<g_caches_cnt; ++i) {
		if (g_caches[i].enabled && !g_referenced[i]) {
			g_need_update = 1;
			memset(&g_caches[i], 0, sizeof(g_caches[i]));
		}
	}

	if(!g_need_update) {
		printk("%s: no need to update\n", __func__);

		return 0;
	}

	for(i=0; i<3; ++i) {
		fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

		if (fd >= 0) {
			break;
		}

		printk("%s: sceIoOpen returns 0x%08X\n", __func__, fd);
		strncpy(path, "ef", sizeof("ef")-1);

		fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

		if (fd >= 0) {
			break;
		}

		printk("%s: sceIoOpen returns 0x%08X\n", __func__, fd);
		strncpy(path, "ms", sizeof("ms")-1);
	}

	if (fd < 0) {
		return -21;
	}

	sceIoWrite(fd, &magic, sizeof(magic));
	sceIoWrite(fd, g_caches, sizeof(g_caches[0])*g_caches_cnt);
	sceIoClose(fd);

	g_need_update = 0;

	return 0;
}

static int get_cache(const char *file, ScePspDateTime *mtime, VirtualPBP* pbp)
{
	int i;

	for(i=0; i<g_caches_cnt; ++i) {
		if(g_caches[i].enabled && 0 == strcmp(g_caches[i].name, file)) {
			if (memcmp(&g_caches[i].mtime, mtime, sizeof(*mtime)) == 0) {
				memcpy(pbp, &g_caches[i], sizeof(*pbp));
				g_referenced[i] = 1;

				return 0;
			}
		}
	}

	return -23;
}

static int build_vpbp(VirtualPBP *vpbp)
{
	int ret, i;
	u32 off;

	printk("Need to build vpbp %s\n", vpbp->name);
	memset(vpbp->header, 0, sizeof(vpbp->header));
	memset(vpbp->sects, 0, sizeof(vpbp->sects));
	vpbp->enabled = 1;
	vpbp->file_pointer = 0;
	vpbp->header[0] = 0x50425000; // PBP magic
	vpbp->header[1] = 0x10000; // version

	// fill vpbp offsets
	off = 0x28;

	isoSetFile(vpbp->name);

	for(i=0; i<NELEMS(pbp_entries); ++i) {
		vpbp->header[i+2] = off;

		if (pbp_entries[i].enabled) {
			PBPSection *sec = &vpbp->sects[i];

			sec->lba = isoGetFileInfo(pbp_entries[i].name, &sec->size);

			if (i == 0) {
				off += sizeof(virtualsfo);
			} else {
				off += sec->size;
			}
		}
	}

	vpbp->total_size = vpbp->header[9];
	ret = add_cache(vpbp);
	printk("%s: add_cache -> %d\n", __func__, ret);

	return ret;
}

static int rebuild_vpbps(const char *dirname)
{
	SceUID dfd;
	int ret, i, nextdir;
	char isopath[] = "ms0:/ISO";
	SceIoDirent dir;
	
	strncpy(isopath, dirname, 2);
	dfd = sceIoDopen(isopath);
	
	if (dfd < 0)
		return dfd;

	i = 0;

	do {
		memset(&dir, 0, sizeof(dir));
		ret = sceIoDread(dfd, &dir);

		if (is_iso(&dir)) {
			i++;
		}
	} while(ret > 0);

	g_vpbps_cnt = i;
	printk("%s: have %d ISO(s)\n", __func__, g_vpbps_cnt);
	sceIoDclose(dfd);

	if (g_vpbps != NULL) {
		oe_free(g_vpbps);
	}

	g_vpbps = oe_malloc(g_vpbps_cnt * sizeof(VirtualPBP));
	memset(g_vpbps, 0, g_vpbps_cnt * sizeof(VirtualPBP));
	dfd = sceIoDopen(isopath);

	if (g_vpbps == NULL) {
		printk("%s: g_vpbps cannot allocate\n", __func__);

		return -20;
	}

	i = 0;
	
	do {
		memset(&dir, 0, sizeof(dir));
		nextdir = sceIoDread(dfd, &dir);

		if (is_iso(&dir)) {
			VirtualPBP *vpbp;

			vpbp = &g_vpbps[i];

			STRCPY_S(vpbp->name, isopath);
			STRCAT_S(vpbp->name, "/");
			STRCAT_S(vpbp->name, dir.d_name);
			memcpy(&vpbp->mtime, &dir.d_stat.st_mtime, sizeof(dir.d_stat.st_mtime));

			ret = get_cache(vpbp->name, &vpbp->mtime, vpbp);

			if (ret < 0) {
				build_vpbp(vpbp);
			}
			
			i++;
		}
	} while(nextdir > 0);

	sceIoDclose(dfd);

	return 0;
}

int vpbp_init(void)
{
	g_sema = sceKernelCreateSema("VPBPSema", 0, 1, 1, NULL);

	if (g_sema < 0)
		return g_sema;

	if (g_caches != NULL) {
		oe_free(g_caches);
		g_caches_cnt = 0;
	}

	g_caches_cnt = CACHE_MAX_SIZE;
	g_caches = oe_malloc(sizeof(*g_caches) * g_caches_cnt);

	if (g_caches == NULL) {
		g_caches_cnt = 0;
		printk("%s: g_cache cannot allocate\n", __func__);

		return -27;
	}

	memset(g_caches, 0, sizeof(g_caches[0]) * g_caches_cnt);
	
	return 0;
}

SceUID vpbp_open(const char * file, int flags, SceMode mode)
{
	int idx;
	VirtualPBP *vpbp;

	lock();

	if (flags & (PSP_O_WRONLY | PSP_O_TRUNC | PSP_O_CREAT) || !(flags & PSP_O_RDONLY)) {
		printk("%s: bad flags 0x%08X\n", __func__, flags);
		unlock();

		return -6;
	}
	
	idx = get_vpbp_by_path(file);
	vpbp = &g_vpbps[idx];

	if (vpbp->enabled) {
		vpbp->file_pointer = 0;
		unlock();

		return MAGIC_VPBP_FD+idx;
	}

	unlock();

	return -26;
}

SceOff vpbp_lseek(SceUID fd, SceOff offset, int whence)
{
	VirtualPBP *vpbp;

	lock();
	vpbp = get_vpbp_by_fd(fd);
	
	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -3;
	}

	switch(whence) {
		case PSP_SEEK_SET:
			vpbp->file_pointer = (int)offset;
			break;
		case PSP_SEEK_CUR:
			vpbp->file_pointer += (int)offset;
			break;
		case PSP_SEEK_END:
			vpbp->file_pointer = vpbp->total_size + (int)offset;
			break;
		default:
			break;
	}

	unlock();

	return vpbp->file_pointer;
}

int vpbp_read(SceUID fd, void * data, SceSize size)
{
	VirtualPBP *vpbp;
	int remaining;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);

	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -4;
	}

	isoSetFile(vpbp->name);
	remaining = size;

	while(remaining > 0) {
		if (vpbp->file_pointer >= 0 && vpbp->file_pointer < vpbp->header[2]) {
			int re;

			re = MIN(remaining, vpbp->header[2] - vpbp->file_pointer);
			memcpy(data, vpbp->header+vpbp->file_pointer, re);
			vpbp->file_pointer += re;
			data += re;
			remaining -= re;
		}

		if (vpbp->file_pointer >= vpbp->header[2] && vpbp->file_pointer < vpbp->header[3]) {
			// use own custom sfo
			int re;
			void *buf, *buf_64;
			char sfotitle[64];

			buf = oe_malloc(SECTOR_SIZE+64);

			if (buf != NULL) {
				buf_64 = PTR_ALIGN_64(buf);
				isoReadRawData(buf_64, vpbp->sects[0].lba, 0, SECTOR_SIZE);
				get_sfo_title(sfotitle, 64, buf_64);
				oe_free(buf);
				memcpy(virtualsfo+0x118, sfotitle, 64);
				re = MIN(remaining, sizeof(virtualsfo) - (vpbp->file_pointer - vpbp->header[2]));
				memcpy(data, virtualsfo+vpbp->file_pointer-vpbp->header[2], re);
				vpbp->file_pointer += re;
				data += re;
				remaining -= re;
			} else {
				printk("%s: buf cannot allocate\n", __func__);
				unlock();

				return -13;
			}
		}

		// ignore last two sections
		int i; for(i=1; i<NELEMS(pbp_entries)-2; ++i) {
			if (pbp_entries[i].enabled && 
					vpbp->file_pointer >= vpbp->header[2+i] && 
					vpbp->file_pointer < vpbp->header[2+1+i]) {
				void *buf, *buf_64;

				buf = oe_malloc(8*SECTOR_SIZE+64);

				if (buf != NULL) {
					int rest, pos, re, total;

					pos = vpbp->file_pointer - vpbp->header[2+i];
					rest = total = MIN(remaining, vpbp->sects[i].size - pos);
					buf_64 = PTR_ALIGN_64(buf);

					while (rest > 0) {
						re = MIN(rest, 8*SECTOR_SIZE);
						isoReadRawData(buf_64, vpbp->sects[i].lba, pos, re);
						memcpy(data, buf_64, re);
						rest -= re;
						pos += re;
						vpbp->file_pointer += re;
						data += re;
						remaining -= re;
					}

					oe_free(buf);
				} else {
					printk("%s: buf(2) cannot allocate\n", __func__);
					unlock();

					return -5;
				}
			}
		}

		if (vpbp->file_pointer >= vpbp->total_size)
			break;
	}

	unlock();
	return size - remaining;
}

int vpbp_close(SceUID fd)
{
	VirtualPBP *vpbp;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);

	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -7;
	}

	unlock();

	return 0;
}

int vpbp_disable_all_caches(void)
{
	lock();

	int i; for(i=0; i<g_vpbps_cnt; ++i) {
		VirtualPBP *vpbp;
		
		vpbp = &g_vpbps[i];
		vpbp->enabled = 0;
	}

	unlock();

	return 0;
}

int vpbp_remove(const char * file)
{
	int ret, idx;
	const char *iso_fn;

	lock();
	idx = get_vpbp_by_path(file);

	if (idx < 0) {
		unlock();

		return -14;
	}

	iso_fn = g_vpbps[idx].name;
	ret = sceIoRemove(iso_fn);
	disable_cache(idx);

	unlock();

	return ret;
}

int vpbp_is_fd(SceUID fd)
{
	VirtualPBP *vpbp;
	int ret;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);

	if (vpbp != NULL)
		ret = 1;
	else
		ret = 0;
	unlock();

	return ret;
}

int vpbp_getstat(const char * file, SceIoStat * stat)
{
	int ret, idx;

	lock();
	idx = get_vpbp_by_path(file);

	if (idx < 0) {
		unlock();

		return idx;
	}

	ret = sceIoGetstat(g_vpbps[idx].name, stat);
	unlock();

	return ret;
}

int vpbp_loadexec(char * file, struct SceKernelLoadExecVSHParam * param)
{
	int ret, idx;
	SEConfig config;

	lock();
	idx = get_vpbp_by_path(file);

	if (idx < 0) {
		unlock();
		return idx;
	}

	//set iso file for reboot
	sctrlSESetUmdFile(g_vpbps[idx].name);

	sctrlSEGetConfig(&config);
	//set iso mode for reboot
	sctrlSESetBootConfFileIndex(config.umdmode);

	//full memory doesn't hurt on isos
	sctrlHENSetMemory(48, 0);
	printk("%s: ISO %s, UMD mode %d\n", __func__, g_vpbps[idx].name, config.umdmode);
	
	//reset and configure reboot parameter
	memset(param, 0, sizeof(param));
	param->size = sizeof(param);
	param->argp = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN";
	param->args = strlen(param->argp) + 1;
	param->key = "game";

	//fix apitypes
	int apitype = 0x120;
	if (sceKernelGetModel() == PSP_GO) apitype = 0x125;

	//start game image
	return sctrlKernelLoadExecVSHWithApitype(apitype, param->argp, param);

	unlock();

	return ret;
}

SceUID vpbp_dopen(const char * dirname)
{
	SceUID result;

	lock();

	if (strlen(dirname) == 13 && stricmp(dirname + 2, "0:/PSP/GAME") == 0) {
		load_cache();
		rebuild_vpbps(dirname);
	}

	g_gamedfd = result = sceIoDopen(dirname);
	unlock();

	return result;
}

int vpbp_dread(SceUID fd, SceIoDirent * dir)
{
	int result;
	static int cur_idx = 0;
   
	lock();
	result = sceIoDread(fd, dir);

	if (fd == g_gamedfd && result == 0) {
		while(cur_idx < g_vpbps_cnt && !g_vpbps[cur_idx].enabled) {
			cur_idx++;
		}
		
		if (cur_idx < g_vpbps_cnt) {
			dir->d_stat.st_mode = 0x11FF;
			dir->d_stat.st_attr = 0x10;
			sprintf(dir->d_name, "ISOGAME%08X", cur_idx);
			result = 1;
			cur_idx++;
		}
	} else {
		cur_idx = 0;
	}

	unlock();

	return result;
}

int vpbp_dclose(SceUID fd)
{
	int result;

	result = sceIoDclose(fd);

	if (fd == g_gamedfd) {
		save_cache();
		g_gamedfd = -1;
	}

	return result;
}

int vpbp_reset(void)
{
	if (g_vpbps != NULL) {
		oe_free(g_vpbps);
		g_vpbps = NULL;
		g_vpbps_cnt = 0;
	}

	if (g_caches != NULL) {
		oe_free(g_caches);
		g_caches = NULL;
		g_caches_cnt = 0;
	}

	g_gamedfd = -1;

	return 0;
}
