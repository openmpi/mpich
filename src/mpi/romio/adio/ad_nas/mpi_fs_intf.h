#ifndef _MPI_FS_INTERFACE_H_
#define _MPI_FS_INTERFACE_H_
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <linux/types.h>
#include <linux/uuid.h>
#include <linux/ioctl.h>

typedef unsigned int u32;

/*
#define EPERM     1    Operation not permitted 
#define ENOEN     2    No such file or directory 
#define ESRCH     3    No such process 
#define EINTR     4    Interrupted system call 
#define EIO       5    I/O error 
#define ENXIO     6    No such device or address 
#define E2BIG     7    Argument list too long 
#define ENOEX     8    Exec format error 
#define EBADF     9    Bad file number 
#define ECHIL    10    No child processes 
#define EAGAI    11    Try again 
#define ENOME    12    Out of memory 
#define EACCE    13    Permission denied 
#define EFAUL    14    Bad address 
#define ENOTB    15    Block device required 
#define EBUSY    16    Device or resource busy 
#define EEXIS    17    File exists 
#define EXDEV    18    Cross-device link 
#define ENODE    19    No such device 
#define ENOTD    20    Not a directory 
#define EISDI    21    Is a directory 
#define EINVA    22    Invalid argument 
#define ENFIL    23    File table overflow 
#define EMFIL    24    Too many open files 
#define ENOTT    25    Not a typewriter 
#define ETXTB    26    Text file busy 
#define EFBIG    27    File too large 
#define ENOSP    28    No space left on device 
#define ESPIP    29    Illegal seek 
#define EROFS    30    Read-only file system 
#define EMLIN    31    Too many links 
#define EPIPE    32    Broken pipe 
#define EDOM     33    Math argument out of domain of func 
#define ERANG    34    Math result not representable
*/

#define FS_MAX_XATTR_NAME_LEN 256UL /* Max user-xattr's name len: 256(����\0������) */
#define FS_MAX_NAME_LEN 1024UL      /* Max  name len: 256(����\0������) */
#define FS_MAX_PATH_LENGTH 4096UL   /* Max  path len: 4096(����\0������) */
#define FS_MAX_LONG_NAME_LEN 1024UL /* Max  long name len: 1024(����\0������) */

/* ****************************************************************************
 �� �� ��  : mpi_fs_open
 ��������  : ��һ���ļ�
 �������  :
             param[in]  pathname  �򿪶���·��(����/0)
             param[in]  flags     �򿪱�־
             param[in]  mode      ��ģʽ
 ����ֵ����
             1���ɹ�����: ��fd
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1.���÷���֤pathname��Ϊ�գ�����/0�������ɵ����߷��䡢�ͷ��ڴ�
**************************************************************************** */
int mpi_fs_open(const char *pathname, int flags, mode_t mode);

/* ****************************************************************************
 �� �� ��  : mpi_fs_pread
 ��������  : ��ȡ�ļ�����
 �������  :
             param[in]  fd        ������fd
             param[in]  buf       ��ȡ���ݴ����buf
             param[in]  count     ��ȡ�ֽ���
             param[in]  offset    ��ȡ��ƫ�Ƶ�ַ
 ����ֵ����
             1���ɹ�����: ���ڵ����㣬������ɹ����ֽ���
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1.���÷���֤buf��Ϊ�գ��ɵ����߷��䡢�ͷ��ڴ�
**************************************************************************** */
int mpi_fs_pread(int fd, void *buf, size_t count, off_t offset);

/* ****************************************************************************
 �� �� ��  : mpi_fs_pwrite
 ��������  : д�ļ�����
 �������  :
             param[in]  fd        д����fd
             param[in]  buf       д����buf
             param[in]  count     д�ֽ���
             param[in]  offset    дƫ�Ƶ�ַ
 ����ֵ����
             1���ɹ�����: ���ڵ����㣬������ɹ����ֽ���
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1.���÷���֤buf��Ϊ�գ��ɵ����߷��䡢�ͷ��ڴ�
**************************************************************************** */
int mpi_fs_pwrite(int fd, const void *buf, size_t count, off_t offset);

/* ****************************************************************************
 �� �� ��  : mpi_fs_stat
 ��������  : �����ļ�·������ѯ�ļ�����״̬
 �������  :
             param[in]  pathname        ����ѯ�����ļ�·��(����/0)
             param[in]  buf             ��ѯ���Խ��buf
 ����ֵ����
             1���ɹ�����: 0
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1��pathname��Ϊ�գ�����/0�������ɵ����߷��䡢�ͷ��ڴ�
             2��buf��Ϊ�գ��ɵ����߷��䡢�ͷ��ڴ�
**************************************************************************** */
int mpi_fs_stat(const char *pathname, struct stat *buf);

/* ****************************************************************************
 �� �� ��  : mpi_fs_close
 ��������  : �ر��ļ�
 �������  :
             param[in]  fd        �رն���fd
 ����ֵ����
             1���ɹ�����: 0
             2��ʧ�ܷ���: -1 is returned and errno is set to indicate the error
 ʹ��Լ��  :
             1�����÷���֤fd��Ч�ԡ�
**************************************************************************** */
int mpi_fs_close(int fd);
/* ****************************************************************************
 �� �� ��  : mpi_fs_ftruncate
 ��������  : ����fd�ض��ļ���ָ������
 �������  :
             param[in]  fd              ���ض��ļ�fd
             param[in]  length          �ضϳ���
 ����ֵ����
             1���ɹ�����: 0
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1��length ���ڵ���0
**************************************************************************** */
int mpi_fs_ftruncate(int fd, off_t length);
/* ****************************************************************************
 �� �� ��  : mpi_fs_lseek
 ��������  : reposition read/write file offset
 �������  :
             param[in]  fd              ��������fd
             param[in]  offset          ��תƫ��
             param[in]  whence          SEEK_SET��SEEK_CUR��SEEK_END
 ����ֵ����
             1���ɹ�����: �����㣬����offset location as measured in bytes
                from the beginning of the file
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
**************************************************************************** */
off_t mpi_fs_lseek(int fd, off_t offset, int whence);

/* ****************************************************************************
 �� �� ��  : mpi_fs_set_fileview
 ��������  : �ļ���ͼͨ���ýӿ�͸����˽�пͻ��ˣ�
            ��˽�пͻ��˸�����ͼ��Ϣ������֯���ݵ�cacheԤȡ�����ݴ洢���ԣ�ϵͳ��ֱ�Ż�
 �������  :
            \param[in]  fd              ��������fd
            \param[in]  offset          �ļ���ͼ��ʼƫ��(bytes)
            \param[in]  count           filetype�������飨block��������
            \param[in]  blocklens       �洢ÿ�������鳤�ȣ�bytes�������飬���鳤��Ϊcount
            \param[in]  blockoffs       �洢ÿ�������鿪ʼλ��ƫ�ƣ�bytes�������飬���鳤��Ϊcount
            \param[in]  ub_off          filetype����λ��ƫ�ƣ�bytes��
 ����ֵ����
             1���ɹ�����: 0
             2��ʧ�ܷ���: -1 and errno is set to indicate the error
 ʹ��Լ��  :
             1.����ǰ��֤�ļ��ѱ��򿪣��ļ�close����ͼ��֮���
             2.���÷���֤fd�Ϸ���, blocklens��Ϊ��, blockoffs��Ϊ�գ��ɵ����߷��䡢�ͷ��ڴ档
**************************************************************************** */
int mpi_fs_set_fileview(int fd, off_t offset, u32 count, u32 *blocklens, off_t *blockoffs, off_t ub_off);

/* ****************************************************************************
 �� �� ��  : mpi_fs_view_read
 ��������  : ������ͼ��ȡ�ļ�����
 �������  :
            \param[in]  fd          ������fd
            \param[in]  iov         ��ȡ���ݴ����io vector
            \param[in]  iovcnt      iov����
            \param[in]  offset      ��ȡ��ƫ�Ƶ�ַ(bytes)
 ����ֵ����
             1.С��0      ʧ��
             2.���ڵ���0   �ɹ�,��ȡ�ɹ����ֽ���
 ʹ��Լ��  :
             1�����δ������ͼҲ�ܵ��ã���������ͨ��read������ͬ
             2�����÷���֤������β�Ϊ�գ�����֤����Ч�ԡ�
                iov��Ϊ�գ�iov_base��Ϊ�գ��ɵ����߷��䡢�ͷ��ڴ�
**************************************************************************** */
int mpi_fs_view_read(int fd, u32 iovcnt, struct iovec *iov, off_t offset);

int mpi_fs_get_group_id(int fd, uint64_t *group_id);
int mpi_fs_set_group_id(int fd, uint64_t group_id);

#endif
