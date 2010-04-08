noinst_LIBRARIES = libvolume_id.a

AM_CFLAGS = $(STAGE2_CFLAGS) -I$(top_srcdir)/stage2 -I$(top_srcdir)/stage1

LIBVOLUME_ID_FS_SUPPORTED = 	ext.c fat.c hfs.c jfs.c  \
			 	luks.c ntfs.c ocfs.c reiserfs.c \
			 	xfs.c 

LIBVOLUME_ID_FS_UNSUPPORTED = 	cramfs.c gfs.c hpfs.c iso9660.c \
			 	lvm.c minix.c romfs.c squashfs.c \
				sysv.c udf.c ufs.c vxfs.c

LIBVOLUME_ID_RAID_SUPPORTED = 	ddf_raid.c 

LIBVOLUME_ID_RAID_UNSUPPORTED = adaptec_raid.c highpoint.c isw_raid.c \
				jmicron_raid.c linux_raid.c lsi_raid.c \
				nvidia_raid.c promise_raid.c silicon_raid.c \
				via_raid.c 

LIBVOLUME_ID_MISC_UNSUPPORTED = linux_swap.c netware.c

libvolume_id_a_SOURCES = 	$(LIBVOLUME_ID_FS_SUPPORTED) \
   			 	$(LIBVOLUME_ID_RAID_SUPPORTED) \
				$(LIBVOLUME_ID_FS_UNSUPPORTED) \
				$(LIBVOLUME_ID_RAID_UNSUPPORTED) \
			 	volume_id.h volume_id.c util.c util.h misc.c 

