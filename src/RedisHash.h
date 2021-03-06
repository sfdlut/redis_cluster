#ifndef __REDIS_HASH_H_H__
#define __REDIS_HASH_H_H__

#include <vector>
#include <map>
#include <string>

#include <boost/thread/mutex.hpp>

#include <zookeeper/zookeeper.h>

#include "Util.h"
#include "RedisEvent.h"

struct zk_ctx_t {

	zhandle_t *h;
	std::string path;


zk_ctx_t(zhandle_t *zkh,
	 const char *zk_path) :
	h(zkh), path(zk_path) {}
};


class RedisHash {
	LogOut *log_;

	boost::shared_mutex smux_;

	std::map< int, std::vector<uint64_t> > addrs_;
  std::vector<
    std::pair< int, std::vector<uint64_t> >
    > addrs_v_;

	// zookeeper
	zk_ctx_t zk_ctx_;

 private:
	int start();
 public:

 RedisHash(LogOut *log,
           zhandle_t *zkh,
           const char *zk_path)
	 : log_(log),
		zk_ctx_(zkh, zk_path)
		{
      start();
		}
	~RedisHash() { if (zk_ctx_.h) zookeeper_close(zk_ctx_.h);}
	void update_ends(const std::map< int ,std::vector< std::pair<std::string, int> > > &ends);
	zk_ctx_t *zk_ctx() { return &zk_ctx_; }

  // give the master
  uint64_t redis_addr_master(const std::string &key);
  // give the master
  void redis_all(std::map<int, uint64_t> &addrs);

  // if have slave return slave, else return master
  uint64_t redis_addr_slave_first(const std::string &key);

	LogOut *log() { return log_; }


};

#endif
