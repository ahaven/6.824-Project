#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static int rpc_status_translate(extent_protocol::status);
  static std::string filename(inum);
  static inum n2i(std::string);
  static inum find_in_dir(std::string, std::string);
  static std::string add_to_dir(std::string, inum, std::string);
  static std::string remove_from_dir(std::string, std::string);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int read(inum, std::string &);
  int write(inum, std::string);
  int remove(inum);

  int lookup(inum, std::string, inum &);
  int create(inum, std::string, inum &, bool dir = false);
  int unlink(inum, std::string);
  int readdir(inum, std::vector<dirent> &);

  int resize(inum, unsigned long long);

  int read_part(inum, size_t, size_t, std::string &);
  int write_part(inum, size_t, std::string);
};

#endif 
