## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2017 by DataDirect Networks
##     See COPYRIGHT in top-level directory.
##

if BUILD_AD_OCEANFS

noinst_HEADERS += adio/ad_nas/ad_oceanfs.h          \
    adio/ad_nas/ad_oceanfs_common.h                 \
    adio/ad_nas/ad_oceanfs_file.h                   \
    adio/ad_nas/ad_oceanfs_pub.h                    \
    adio/ad_nas/ad_oceanfs_tuning.h                 \
    adio/ad_nas/ad_oceanfs_group_tuning.h           \
    adio/ad_nas/ad_oceanfs_llt.h                    \
    adio/ad_nas/ad_oceanfs_aggrs.h                  \
    adio/ad_nas/mpi_fs_intf.h                       \
    adio/ad_nas/securec/include/securectype.h       \
    adio/ad_nas/securec/include/securec.h
romio_other_sources +=                              \
    adio/ad_nas/ad_oceanfs.c                        \
    adio/ad_nas/ad_oceanfs_close.c                  \
    adio/ad_nas/ad_oceanfs_common.c                 \
    adio/ad_nas/ad_oceanfs_fcntl.c                  \
    adio/ad_nas/ad_oceanfs_io.c                     \
    adio/ad_nas/ad_oceanfs_open.c                   \
    adio/ad_nas/ad_oceanfs_file.c                   \
    adio/ad_nas/ad_oceanfs_pub.c                    \
    adio/ad_nas/ad_oceanfs_tuning.c                 \
    adio/ad_nas/ad_oceanfs_group_tuning.c           \
    adio/ad_nas/ad_oceanfs_llt.c                    \
    adio/ad_nas/ad_oceanfs_aggrs.c                  \
    adio/ad_nas/ad_oceanfs_rdstr.c                  \
    adio/ad_nas/ad_oceanfs_rdcoll.c                 \
    adio/ad_nas/ad_oceanfs_wrcoll.c                 \
    adio/ad_nas/ad_oceanfs_resize.c                 \
    adio/ad_nas/ad_oceanfs_hints.c                  \
    adio/ad_nas/ad_oceanfs_view.c                   \
    adio/ad_nas/ad_oceanfs_viewio.c                 \
    adio/ad_nas/mpi_fs_intf.c                       \
    adio/ad_nas/securec/src/fscanf_s.c              \
    adio/ad_nas/securec/src/fwscanf_s.c             \
    adio/ad_nas/securec/src/gets_s.c                \
    adio/ad_nas/securec/src/input.inl               \
    adio/ad_nas/securec/src/memcpy_s.c              \
    adio/ad_nas/securec/src/memmove_s.c             \
    adio/ad_nas/securec/src/memset_s.c              \
    adio/ad_nas/securec/src/output.inl              \
    adio/ad_nas/securec/src/scanf_s.c               \
    adio/ad_nas/securec/src/secinput.h              \
    adio/ad_nas/securec/src/securecutil.c           \
    adio/ad_nas/securec/src/securecutil.h           \
    adio/ad_nas/securec/src/secureinput_a.c         \
    adio/ad_nas/securec/src/secureinput_w.c         \
    adio/ad_nas/securec/src/secureprintoutput_a.c   \
    adio/ad_nas/securec/src/secureprintoutput.h     \
    adio/ad_nas/securec/src/secureprintoutput_w.c   \
    adio/ad_nas/securec/src/snprintf_s.c            \
    adio/ad_nas/securec/src/sprintf_s.c             \
    adio/ad_nas/securec/src/sscanf_s.c              \
    adio/ad_nas/securec/src/strcat_s.c              \
    adio/ad_nas/securec/src/strcpy_s.c              \
    adio/ad_nas/securec/src/strncat_s.c             \
    adio/ad_nas/securec/src/strncpy_s.c             \
    adio/ad_nas/securec/src/strtok_s.c              \
    adio/ad_nas/securec/src/swprintf_s.c            \
    adio/ad_nas/securec/src/swscanf_s.c             \
    adio/ad_nas/securec/src/vfscanf_s.c             \
    adio/ad_nas/securec/src/vfwscanf_s.c            \
    adio/ad_nas/securec/src/vscanf_s.c              \
    adio/ad_nas/securec/src/vsnprintf_s.c           \
    adio/ad_nas/securec/src/vsprintf_s.c            \
    adio/ad_nas/securec/src/vsscanf_s.c             \
    adio/ad_nas/securec/src/vswprintf_s.c           \
    adio/ad_nas/securec/src/vswscanf_s.c            \
    adio/ad_nas/securec/src/vwscanf_s.c             \
    adio/ad_nas/securec/src/wcscat_s.c              \
    adio/ad_nas/securec/src/wcscpy_s.c              \
    adio/ad_nas/securec/src/wcsncat_s.c             \
    adio/ad_nas/securec/src/wcsncpy_s.c             \
    adio/ad_nas/securec/src/wcstok_s.c              \
    adio/ad_nas/securec/src/wmemcpy_s.c             \
    adio/ad_nas/securec/src/wmemmove_s.c            \
    adio/ad_nas/securec/src/wscanf_s.c 

endif BUILD_AD_OCEANFS
