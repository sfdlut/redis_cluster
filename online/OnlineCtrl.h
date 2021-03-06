#ifndef __ONLINE_CTRL_H_H__
#define __ONLINE_CTRL_H_H__

#include <string>

#include "../src/RedisEvent.h"
#include "../src/RedisHash.h"

#include "../logic_driver/HookDef.h"

class OnlineCtrl {
  struct script_t {
    std::string sha1;
    std::string data;
  };

	LogOut *log_;
	RedisEvent *re_;
	RedisHash *rh_;


  std::string sp_;

  script_t s_session_info_;
  script_t s_sessions_;
  script_t s_multi_;
  script_t s_timeout_rm_;

  script_t s_syn_;
  script_t s_fin_;
  script_t s_fin_delay_;
  script_t s_upidx_;

 private:
  void single_uid_commend(const char *fun,
                          int timeout,
                          const std::string &suid, std::vector<std::string> &args,
                          const std::string &lua_code,
                          RedisRvs &rv);

  void one_uid_session(long actor, const RedisRv &rv, std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions);
  void load_script(const std::string &path, script_t &scp);
 public:
 OnlineCtrl(
            LogOut *log,

            RedisEvent *re,
            RedisHash *rh,

            const char *script_path
            );

 //-------------------logic connection level interface-----------------------
 int syn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx);
 int fin(int timeout, long uid, const proto_fin &proto, std::string &cli_info);
 int fin_delay(int timeout, long uid, const proto_fin_delay &proto);
 int upidx(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx);
 int timeout_rm(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs);
 // void msg(int timeout, long uid, const proto_msg &proto);
 //--------bussiness level interface--------------------
 // ...

 //-----------------------------
 /*
 void online(int timeout, long uid, const std::string &session, int type, const std::vector<std::string> &kvs);
	void offline(int timeout, long uid, const std::string &session);
 */
	void get_sessions(int timeout, long uid, std::vector<std::string> &sessions);
	void get_session_info(int timeout, long uid, const std::string &session, const std::vector<std::string> &ks,
                        std::map<std::string, std::string> &kvs);

	void get_multi(int timeout, long actor, const std::vector<long> &uids,
                 std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions
                 );



};


#endif
