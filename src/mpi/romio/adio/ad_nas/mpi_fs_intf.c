#include "mpi_fs_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "ad_oceanfs_pub.h"
#include "adio.h"
#include "securec.h"


#ifdef DEBUG
#define ASSERT(f) assert(f)
#else
#define ASSERT(f) ((void)0)
#endif

void* mpi_zalloc(uint32_t n)
{
    if (n <= 0) {
        return NULL;
    }

    char* p = malloc(n);
    if (p == NULL) {
        return NULL;
    }
    memset_s(p, n, 0, n);
    return p;
}

#define mpi_free(p) \
    do { \
        free(p); \
        (p) = NULL; \
    } while (0)

#define atomic_t int
#define atomic_inc(v) __sync_fetch_and_add(v, 1)
#define atomic_dec_and_test(v) (__sync_fetch_and_sub(v, 1) == 1)
#define atomic_set(v, i) ((*(v)) = (i))


#define CHECK_NULL_POINTER(x, ret) \
    do { \
        if ((x) == NULL) \
        { \
            return (ret); \
        } \
    }while (0)

#define TRUE 1
#define FALSE 0

#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
    #define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif


#define MAX_VIEW_READ_SIZE (4 * 1024 * 1024)
#define MPI_FD_HANDLE_HASHTBL_SIZE 64

// IOCTL�����֣�ͬʱҲ������odc.h��
#define ODCS_IOC_MPI_VIEW_READ _IOWR('S', 101, mpi_fs_view_read_t)
#define ODCS_IOC_MPI_PREFETCH _IOWR('S', 102, mpi_fs_view_read_t)
#define ODCS_IOC_IOCTL_MPI_GROUP_ID    _IOWR('S', 103, mpi_fs_group_id_t)
#define ODCS_IOC_IOCTL_MPI_SET_GROUP_ID    _IOWR('S', 111, mpi_fs_group_id_t)

static pthread_mutex_t g_init_mpi_fh_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_init_mpi_fh_pid = 0;

typedef struct mpi_fs_group_id {
    int fd;
    uint64_t group_id;
} mpi_fs_group_id_t;

typedef enum mpi_fh_state {
    MPI_FH_STATE_INUSE,
    MPI_FH_STATE_DELETE
} mpi_fh_state_t;

typedef struct mpi_fs_view {
    off_t offset; // filetype ��ʼλ�ã�������ļ���ͷ��ƫ��
    // ���ļ�����������ʼλ��
    u32 count; // filetype�������飨block��������
    // ���ļ����������ݿ����
    u32 *blocklens; // �洢ÿ�������鳤�ȣ�bytes�������飬���鳤��Ϊcount
    // ���ļ������������ݿ鳤��
    off_t *blockoffs; // �洢ÿ�������鿪ʼλ��ƫ�ƣ�bytes�������飬���鳤��Ϊcount
    // ���ļ������������ݿ�ƫ��
    off_t ub_off; // filetype����λ�ã���ֵΪ������ļ���ͷ��ƫ��
    // �ļ�������ʱ���˲�����Ч
    char data[0]; // blocklens + blockoffs ������
} mpi_fs_view_t;

typedef struct mpi_fs_view_read {
    off_t offset;
    uint32_t readLen;
    uint32_t readRangeLen;
    uint32_t readRangeCount;
    uint32_t *blocklens;
    off_t *blockoffs;
    char data[0];
    // read buffer + read range buffer
    // read range buffer 
    // blocklens[count]
    // blockoffs[count]
} mpi_fs_view_read_t;

typedef struct list_head {
        struct list_head *next, *prev;   /**< ǰ������ָ��  */
} list_head_t;

typedef struct mpi_list_with_lock_s {
    list_head_t list;
} mpi_list_with_lock;

typedef struct mpi_fh_hashtbl {
    mpi_list_with_lock ht[MPI_FD_HANDLE_HASHTBL_SIZE];
} mpi_fh_hashtbl_t;

static mpi_fh_hashtbl_t g_mpi_fh_hashtbl;

/** \brief ��ʼ������ͷ
    \param[in] ptr ����ṹָ��
*/
#define INIT_LIST_HEAD(ptr) \
    do { \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

/** \brief ͨ�������ַ��ýṹ��ָ��
    \param[in] ptr ����ṹָ��
    \param[in] type �ṹ������
    \param[in] member �ṹ������������ʾ���ֶ�
*/
#define list_entry(ptr, type, member) \
    ((type *)(void *)((char *)(ptr) - offsetof(type, member)))

static void list_add_tail(list_head_t *new_head, list_head_t *head)
{
    /* ��ӵ�����β */
    list_head_t *prev = head->prev;
    list_head_t *next = head;
        
    /* ��ӵ����� */
    next->prev = new_head;
    new_head->next = next;
    new_head->prev = prev;
    prev->next = new_head;
}

/** \brief ɾ��ָ������Ԫ�ز���ʼ��
    \param[in] entry ��ɾ������ʼ��������Ԫ��
*/
static void list_del_init(struct list_head *entry)
{
    /* ������ɾ�� */
    list_head_t *prev = entry->prev;
    list_head_t *next = entry->next;
    
    next->prev = prev;
    prev->next = next;
    
    INIT_LIST_HEAD(entry); 
}

typedef struct mpi_fd_handle {
    list_head_t node;
    int fd;
    pthread_mutex_t lock;
    atomic_t ref_count;
    mpi_fh_state_t state;
    mpi_fs_view_t *view;
} mpi_fh_handle_t;

static void init_mpi_hashtbl(mpi_list_with_lock *ht, int hash_size)
{
    if (ht == NULL) {
        return;
    }

    int i;
    for (i = 0; i < hash_size; i++) {
        INIT_LIST_HEAD(&ht[i].list);
    }
}

static int init_mpi_fh_table(void)
{
    pthread_mutex_lock(&g_init_mpi_fh_lock);
    if (getpid() == g_init_mpi_fh_pid) {
        pthread_mutex_unlock(&g_init_mpi_fh_lock);
        return 0;
    }

    init_mpi_hashtbl(g_mpi_fh_hashtbl.ht, MPI_FD_HANDLE_HASHTBL_SIZE);

    g_init_mpi_fh_pid = getpid();

    pthread_mutex_unlock(&g_init_mpi_fh_lock);

    return 0;
}

static int insert_mpi_fh_table(int fd)
{
    int ret = init_mpi_fh_table();
    if (ret != 0) {
        return ret;
    }

    if (fd < 0) {
        return -1;
    }

    mpi_fh_handle_t *fh = (mpi_fh_handle_t *)mpi_zalloc(sizeof(mpi_fh_handle_t));
    if (fh == NULL) {
        return -1;
    }

    fh->fd = fd;
    atomic_set(&fh->ref_count, 1);
    fh->state = MPI_FH_STATE_INUSE;
    pthread_mutex_init(&fh->lock, NULL);
    INIT_LIST_HEAD(&fh->node);

    int bucket = fd % MPI_FD_HANDLE_HASHTBL_SIZE;
    pthread_mutex_lock(&g_init_mpi_fh_lock);
    list_add_tail(&fh->node, &g_mpi_fh_hashtbl.ht[bucket].list);
    pthread_mutex_unlock(&g_init_mpi_fh_lock);

    return 0;
}

int mpi_fs_open(const char *pathname, int flags, mode_t mode)
{
    mode &= ~S_IFMT; // ȥ��filetypeλ
    mode |= S_IFREG; // ����Ϊ��ͨ�ļ�

    int fd = open(pathname, flags, mode);
    if (fd >= 0) {
        if (insert_mpi_fh_table(fd) != 0) {
            close(fd);
            return -1;
        }
    }

    return fd;
}

int mpi_fs_pread(int fd, void *buf, size_t count, off_t offset)
{
    return (int)pread(fd, buf, count, offset);
}

int mpi_fs_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    return (int)pwrite(fd, buf, count, offset);
}

int mpi_fs_stat(const char *pathname, struct stat *buf)
{
    return stat(pathname, buf);
}

static mpi_fh_handle_t *find_mpi_fh_in_ht(int fd, int is_del)
{
    mpi_fh_handle_t *fh = NULL;
    list_head_t *pos = NULL;
    list_head_t *next = NULL;

    int bucket = fd % MPI_FD_HANDLE_HASHTBL_SIZE;
    pthread_mutex_lock(&g_init_mpi_fh_lock);

    list_head_t* head = &g_mpi_fh_hashtbl.ht[bucket].list;
    for (pos = head->next, next = pos->next; pos != head; pos = next, next = pos->next) {
        fh = list_entry(pos, mpi_fh_handle_t, node);
        if (fd == fh->fd) {
            if (is_del) {
                list_del_init(&fh->node);
            }
            atomic_inc(&fh->ref_count);
            pthread_mutex_unlock(&g_init_mpi_fh_lock);
            return fh;
        }
    }
    pthread_mutex_unlock(&g_init_mpi_fh_lock);

    return NULL;
}

static void mpi_fh_put(mpi_fh_handle_t *fh)
{
    if (fh == NULL) {
        return;
    }

    if (!atomic_dec_and_test(&fh->ref_count)) {
        return;
    }

    pthread_mutex_lock(&fh->lock);
    mpi_fh_state_t status = fh->state;
    pthread_mutex_unlock(&fh->lock);

    if (status == MPI_FH_STATE_INUSE) {
        ASSERT(0);
        return;
    }

    if (status == MPI_FH_STATE_DELETE) {
        // �����ͼ
        mpi_free(fh->view);
        mpi_free(fh);
    }
}

static void mpi_delete_fd_handle(int fd)
{
    int ret = init_mpi_fh_table();
    if (ret != 0) {
        return;
    }

    if (fd < 0) {
        return;
    }

    mpi_fh_handle_t *fh = find_mpi_fh_in_ht(fd, TRUE);
    if (fh == NULL) {
        return;
    }

    pthread_mutex_lock(&fh->lock);
    fh->state = MPI_FH_STATE_DELETE;
    pthread_mutex_unlock(&fh->lock);

    // find_fh_in_ht ������ü������ȼ�һ��ref count
    mpi_fh_put(fh);

    // ɾ��fh
    mpi_fh_put(fh);
}

int mpi_fs_close(int fd)
{
    int ret = close(fd);
    if (ret == 0) {
        mpi_delete_fd_handle(fd);
    }

    return ret;
}

int mpi_fs_ftruncate(int fd, off_t length)
{
    return (int)ftruncate(fd, length);/*lint !e718 !e746*/
}

off_t mpi_fs_lseek(int fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}

static mpi_fs_view_t *AllocMPIFSView(uint32_t count)
{
    if (count == 0) {
        return NULL;
    }

    mpi_fs_view_t *view = mpi_zalloc(sizeof(mpi_fs_view_t) + count * sizeof(uint32_t) + count * sizeof(off_t));
    if (view == NULL) {
        return NULL;
    }

    view->count = count;
    view->blocklens = (u32 *)view->data;
    view->blockoffs = (off_t *)(view->data + count * sizeof(uint32_t));

    return view;
}

static mpi_fh_handle_t *mpi_fh_get(int fd)
{
    int ret = init_mpi_fh_table();
    if (ret != 0 || fd < 0) {
        return NULL;
    }

    return find_mpi_fh_in_ht(fd, FALSE);
}

int mpi_fs_set_fileview(int fd, off_t offset, u32 count, u32 *blocklens, off_t *blockoffs, off_t ub_off)
{
    int ret = 0;
    mpi_fh_handle_t *fh = mpi_fh_get(fd);
    if (fh == NULL) {
        errno = EINVAL;
        return -1;
    }

    mpi_fs_view_t *view = AllocMPIFSView(count);
    if (view == NULL) {
        mpi_fh_put(fh);
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&fh->lock);
    // �ͷ��ϵ�view
    mpi_free(fh->view);

    fh->view = view;
    ret = memcpy_s(fh->view->blocklens, count * sizeof(uint32_t), blocklens, count * sizeof(uint32_t));
    if (ret != 0) {
        ROMIO_LOG(AD_LOG_LEVEL_ALL, "memcpy error! errcode:%d", ret);
    }
    ret = memcpy_s(fh->view->blockoffs, count * sizeof(off_t), blockoffs, count * sizeof(off_t));
    if (ret != 0) {
        ROMIO_LOG(AD_LOG_LEVEL_ALL, "memcpy error! errcode:%d", ret);
    }

    fh->view->offset = offset;
    fh->view->count = count;
    fh->view->ub_off = ub_off;
    pthread_mutex_unlock(&fh->lock);

    mpi_fh_put(fh);
    return 0;
}

static uint32_t GetBlockCntInMPIView(off_t readStart, uint32_t readLen, mpi_fs_view_t *mpiView)
{
    // ÿ��filetype�Ĵ�С
    const uint32_t filetypeLen = mpiView->ub_off - mpiView->offset;
    const off_t displacement = mpiView->offset;

    // ��ǰ��filetype�е�ƫ��
    off_t posInFiletype = (readStart - displacement) % filetypeLen;
    // ���ڶ��ڼ�����
    uint32_t cntBlockRead = 0;
    // ����Ҫ�����ٸ��ֽ�
    uint32_t bytesLeft = readLen;

    uint32_t i;
    uint32_t bytesToRead;
    // ��ǰ��ȡ�Ŀ�С���ܵĿ�����
    while (bytesLeft > 0) {
        for (i = 0; i < mpiView->count; i++) {
            if (bytesLeft == 0) {
                break;
            }
            if (mpiView->blocklens[i] == 0) {
                // �յ�blocklens
                continue;
            }
            if (posInFiletype <= mpiView->blockoffs[i]) {
                // �ƻ���һ����
                bytesToRead = mpiView->blocklens[i];
                // ����Ƿ񹻶�
                bytesToRead = (bytesLeft > bytesToRead) ? bytesToRead : bytesLeft;
                // ����ʣ���ֽڣ�ֻ�����0��������ɸ���
                bytesLeft -= bytesToRead;

                posInFiletype += bytesToRead;
                cntBlockRead++;
            } else if (posInFiletype < (mpiView->blockoffs[i] + mpiView->blocklens[i])) {
                // �ƻ�������Ĳ���
                bytesToRead = mpiView->blockoffs[i] + mpiView->blocklens[i] - posInFiletype;
                // ����Ƿ񹻶�
                bytesToRead = (bytesLeft > bytesToRead) ? bytesToRead : bytesLeft;
                // ����ʣ���ֽڣ�ֻ�����0��������ɸ���
                bytesLeft -= bytesToRead;

                posInFiletype += bytesToRead;
                cntBlockRead++;
            } else {
                // ��ȡ��ƫ�Ƴ�����ǰ��ͼ���λ�ã�׼������һ��block
                continue;
            }
        }

        // ����һ��filetype��׼������һ��
        posInFiletype = 0;
    }

    return cntBlockRead;
}

static mpi_fs_view_read_t *AllocMPIViewRead(off_t offset, uint32_t readLen, uint32_t readRangeCount)
{
    mpi_fs_view_read_t *viewRead = NULL;
    uint32_t readRangeLen = readRangeCount * (sizeof(uint32_t) + sizeof(off_t));
    uint32_t viewReadSize = sizeof(mpi_fs_view_read_t) + readLen + readRangeLen;

    if (readRangeCount == 0) {
        return NULL;
    }

    if (readLen > MAX_VIEW_READ_SIZE || viewReadSize > MAX_VIEW_READ_SIZE) {
        return NULL;
    }

    viewRead = mpi_zalloc(viewReadSize);
    if (viewRead == NULL) {
        return NULL;
    }

    viewRead->offset = offset;
    viewRead->readLen = readLen;
    viewRead->readRangeLen = readRangeLen;
    viewRead->readRangeCount = readRangeCount;
    viewRead->blocklens = (uint32_t *)(viewRead->data + readLen);
    viewRead->blockoffs = (off_t *)(viewRead->data + readLen + (readRangeCount * sizeof(uint32_t)));

    return viewRead;
}

/* ****************************************************************************
 ��������  : ����MPI��ͼ�Ͷ��ķ�Χ�����������ķ�Χ
**************************************************************************** */
void MakeMPIViewReadRange(mpi_fs_view_read_t *viewRead, mpi_fs_view_t *mpiView)
{
    /*lint -e647*/
    uint32_t i;
    off_t readStart = viewRead->offset;
    uint32_t count = viewRead->readRangeCount;

    // ÿ��filetype�Ĵ�С
    const uint32_t filetypeLen = mpiView->ub_off - mpiView->offset;
    const off_t displacement = mpiView->offset;

    // ��ǰ��filetype�е�ƫ��
    off_t posInFiletype = (readStart - displacement) % filetypeLen;
    // �ڼ���filetype
    uint32_t posFiletype = (readStart - displacement) / filetypeLen;
    // ���ڶ��ڼ�����
    uint32_t cntBlockRead = 0;
    // ����Ҫ�����ٸ��ֽ�
    uint32_t bytesLeft = viewRead->readLen;

    uint32_t bytesToRead;
    // ��ǰ��ȡ�Ŀ�С���ܵĿ�����
    while (cntBlockRead < count && bytesLeft > 0) {
        for (i = 0; i < mpiView->count; i++) {
            if (cntBlockRead >= count || bytesLeft == 0) {
                break;
            }
            if (mpiView->blocklens[i] == 0) {
                // �յ�blocklens
                continue;
            }
            if (posInFiletype <= mpiView->blockoffs[i]) {
                // �ƻ���һ����
                bytesToRead = mpiView->blocklens[i];
                // ����Ƿ񹻶�
                bytesToRead = (bytesLeft > bytesToRead) ? bytesToRead : bytesLeft;
                // ����ʣ���ֽڣ�ֻ�����0��������ɸ���
                bytesLeft -= bytesToRead;

                viewRead->blocklens[cntBlockRead] = bytesToRead;
                viewRead->blockoffs[cntBlockRead] = displacement + posFiletype * filetypeLen + mpiView->blockoffs[i];

                posInFiletype += bytesToRead;
                cntBlockRead++;
            } else if (posInFiletype < (mpiView->blockoffs[i] + mpiView->blocklens[i])) {
                // �ƻ�������Ĳ���
                bytesToRead = mpiView->blockoffs[i] + mpiView->blocklens[i] - posInFiletype;
                // ����Ƿ񹻶�
                bytesToRead = (bytesLeft > bytesToRead) ? bytesToRead : bytesLeft;
                // ����ʣ���ֽڣ�ֻ�����0��������ɸ���
                bytesLeft -= bytesToRead;

                viewRead->blocklens[cntBlockRead] = bytesToRead;
                viewRead->blockoffs[cntBlockRead] = displacement + posFiletype * filetypeLen + posInFiletype;

                posInFiletype += bytesToRead;
                cntBlockRead++;
            } else {
                // ��ȡ��ƫ�Ƴ�����ǰ��ͼ���λ�ã�׼������һ��block
                continue;
            }
        }

        // ����һ��filetype��׼������һ��
        posFiletype++;
        posInFiletype = 0;
    }
}

/* ****************************************************************************
 ��������  : ����buffer��iov
 �� �� ֵ  :    δ�������ֽ���
**************************************************************************** */
static u32 CopyBuffToIov(const void *buff, u32 bufLen, struct iovec *iov, u32 iovLen)
{
    uint32_t pos = 0;
    uint32_t i;
    int ret = 0;

    CHECK_NULL_POINTER(buff, EINVAL);
    CHECK_NULL_POINTER(iov, EINVAL);

    uint32_t left_len = (uint32_t)bufLen;
    uint32_t copy_len = 0;
    for (i = 0; i < iovLen; i++) {
        copy_len = MIN(left_len, iov[i].iov_len);
        left_len -= copy_len;
        ret = memcpy_s(iov[i].iov_base, copy_len, (((char *)buff) + pos), copy_len);
        if (ret != EOK) {
            ROMIO_LOG(AD_LOG_LEVEL_ALL, "memcpy error!, errcode:%d", ret);
        }

        iov[i].iov_len = copy_len;
        pos += copy_len;
        if (left_len == 0) {
            break;
        }
    }

    return left_len;
}

int mpi_fs_view_read(int fd, u32 iovcnt, struct iovec *iov, off_t offset)
{
    int ret = -1;
    u32 leftLen = 0;

    if (iov == NULL || iovcnt == 0) {
        errno = EINVAL;
        return -1;
    }

    mpi_fh_handle_t *fh = mpi_fh_get(fd);
    if (fh == NULL) {
        errno = EINVAL;
        return -1;
    }

    // ������Ҫ��ȡ���ֽڳ���
    uint32_t readLen = 0;
    uint32_t i;
    for (i = 0; i < iovcnt; i++) {
        readLen += iov[i].iov_len;
    }

    // ��ȡ��ͼ�����
    pthread_mutex_lock(&fh->lock);

    // ����˴�viewread ��Ҫ��ȡ�Ŀ�ĸ���
    uint32_t count;
    count = GetBlockCntInMPIView(offset, readLen, fh->view);
    if (count == 0) {
        pthread_mutex_unlock(&fh->lock);
        mpi_fh_put(fh);
        return -1;
    }

    // �����������Ҫ��ȡ�Ŀ������������ڴ�
    // ��Ҫһ�������ڴ淢�͸�odcs
    // �ڴ��ʽΪmpi_fs_view_read_t��ĩβΪread buffer + read range buffer
    mpi_fs_view_read_t *viewRead = AllocMPIViewRead(offset, readLen, count);
    if (viewRead == NULL) {
        pthread_mutex_unlock(&fh->lock);
        mpi_fh_put(fh);
        return -1;
    }

    MakeMPIViewReadRange(viewRead, fh->view);

    // ʹ������ͼ�����
    pthread_mutex_unlock(&fh->lock);

    ret = ioctl(fd, ODCS_IOC_MPI_VIEW_READ, viewRead);
    if (ret == 0) {
        leftLen = CopyBuffToIov(viewRead->data, viewRead->readLen, iov, iovcnt);
        if (leftLen != 0) {
            ret = -1;
        }

        // ����ʵ�ʶ�ȡ�ĳ���
        ret = viewRead->readLen;
    }

    mpi_free(viewRead);

    mpi_fh_put(fh);
    return ret;
}

int mpi_fs_get_group_id(int fd, uint64_t *group_id)
{
    if (group_id == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    mpi_fs_group_id_t group;
    group.fd = fd;
    int ret = ioctl(fd, ODCS_IOC_IOCTL_MPI_GROUP_ID, &group);
    if (ret) {
        *group_id = 0;
        return ret;
    }   

    *group_id = group.group_id;
    return ret;
}

int mpi_fs_set_group_id(int fd, uint64_t group_id)
{
    mpi_fs_group_id_t group;
    group.fd = fd;
    group.group_id = group_id;
    return ioctl(fd, ODCS_IOC_IOCTL_MPI_SET_GROUP_ID, &group);
}

