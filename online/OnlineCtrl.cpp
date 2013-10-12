#include <openssl/sha.h>

#include <boost/lexical_cast.hpp>

#include "OnlineCtrl.h"

using namespace std;

static const char *online_ope = "/online.lua";
static const char *offline_ope = "/offline.lua";
static const char *get_session_info_ope = "/get_session_info.lua";
static const char *get_session_ope = "/get_session_ope.lua";
static const char *get_multi_ope = "/get_multi.lua";


static bool check_sha1(const char *path, string &data, string &sha1)
{
	char buf[512];
	SHA_CTX s;
	unsigned char hash[20];

	FILE *fp = fopen(path,"rb");
	if (!fp) return false;

	SHA1_Init(&s);
	int size;
	while ((size=fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
		SHA1_Update(&s, buf, size);
		data.append(buf, size);
	} 
	SHA1_Final(hash, &s);
	fclose(fp);

	sha1.clear();
	char tmp[10] = {0};
	for (int i=0; i < 20; i++) {
		sprintf (tmp, "%.2x", (int)hash[i]);
		sha1 += tmp;
	}

	return true;
 
}

OnlineCtrl::OnlineCtrl(void (*log_t)(const char *),
                       void (*log_d)(const char *),
                       void (*log_i)(const char *),
                       void (*log_w)(const char *),
                       void (*log_e)(const char *),

                       const char *zk_addr,
                       const char *zk_path,

                       const char *script_path
                       ) : log_(log_t, log_d, log_i, log_w, log_e),
                           re_(&log_), rh_(&log_, &re_, zk_addr, zk_path),
                           sp_(script_path)
{
  re_.start();
  rh_.start();

  // init script
  string path = sp_ + online_ope;
	if (!check_sha1(path.c_str(), s_online_.data, s_online_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}

	log_.info("data:%s sha1:%s", s_online_.data.c_str(), s_online_.sha1.c_str());

  path = sp_ + offline_ope;
	if (!check_sha1(path.c_str(), s_offline_.data, s_offline_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_offline_.data.c_str(), s_offline_.sha1.c_str());

  path = sp_ + get_session_info_ope;
	if (!check_sha1(path.c_str(), s_session_info_.data, s_session_info_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_session_info_.data.c_str(), s_session_info_.sha1.c_str());

  path = sp_ + get_session_ope;
	if (!check_sha1(path.c_str(), s_sessions_.data, s_sessions_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_sessions_.data.c_str(), s_sessions_.sha1.c_str());


  path = sp_ + get_multi_ope;
	if (!check_sha1(path.c_str(), s_multi_.data, s_multi_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_multi_.data.c_str(), s_multi_.sha1.c_str());




}

void OnlineCtrl::single_uid_commend(
                                    const char *fun,
                                    int timeout,
                                    const std::string &suid, std::vector<std::string> &args,
                                    const string &lua_code,
                                    RedisRvs &rv
                                    )
{

  uint64_t rd_addr = rh_.redis_addr(suid);
  if (!rd_addr) {
    log_.error("%s-->error hash uid:%s", fun, suid.c_str());
    return;
  }

  map< uint64_t, vector<string> > addr_cmd;

  addr_cmd.insert(pair< uint64_t, vector<string> >(rd_addr, vector<string>())).first->second.swap(args);

  re_.cmd(rv, suid.c_str(), addr_cmd, timeout, lua_code, false);


	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr = fun + boost::lexical_cast<string>(it->first);
    it->second.dump(&log_, addr.c_str(), 0);
	}


}

void OnlineCtrl::offline(int timeout, long uid, const std::string &session)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::offline";

	RedisRvs rv;
  vector<string> args;

  string suid = boost::lexical_cast<string>(uid);

  args.push_back("EVALSHA");
  args.push_back(s_offline_.sha1);
  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);

  single_uid_commend(fun, timeout, suid, args, s_offline_.data, rv);

	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());

}


void OnlineCtrl::online(int timeout, long uid,
			const string &session,
			const vector<string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::online";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

	size_t sz = kvs.size();
	if (sz < 2 || sz % 2 != 0) {
		log_.error("kvs % 2 0 size:%lu", sz);
		return;
	}

  
  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_online_.sha1);

  args.push_back("3");
  args.push_back(suid);
  args.push_back(session);
  args.push_back(boost::lexical_cast<string>(time(NULL)));
  args.insert(args.end(), kvs.begin(), kvs.end());

	log_.info("%s-->arg uid:%ld tm:%ld", fun, uid, tu.intv_reset());
	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());
  
  single_uid_commend(fun, timeout, suid, args, s_online_.data, rv);

	log_.info("%s-->over uid:%ld tm:%ld", fun, uid, tu.intv());


}

void OnlineCtrl::get_sessions(int timeout, long uid, vector<string> &sessions)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_sessions";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_sessions_.sha1);
  args.push_back("1");
  args.push_back(suid);


	log_.debug("%s-->uid:%ld rv.size:%lu", fun, uid, rv.size());

  single_uid_commend(fun, timeout, suid, args, s_sessions_.data, rv);  

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    const RedisRv &tmp = it->second;

    if (tmp.type == REDIS_REPLY_ARRAY) {
      for (vector<RedisRv>::const_iterator jt = tmp.element.begin();
         jt != tmp.element.end(); ++jt) {

        if (jt->type == REDIS_REPLY_STRING) {
          sessions.push_back(jt->str);
        }
    
      }

    }
  }
	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
}


void OnlineCtrl::get_session_info(int timeout, long uid, const string &session, const vector<string> &ks,
                                  map<string, string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_session_info";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_session_info_.sha1);
  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);
  args.insert(args.end(), ks.begin(), ks.end());

	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

  single_uid_commend(fun, timeout, suid, args, s_session_info_.data, rv);

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;

    if (tmp.type == REDIS_REPLY_ARRAY) {
      const vector<RedisRv> &tmp_eles = tmp.element;
      bool ispair = true;
      if (tmp_eles.size() % 2 != 0) {
        ispair = false;
      }

      int kvf = 0;
      const char *key = NULL;
      for (vector<RedisRv>::const_iterator jt = tmp_eles.begin();
         jt != tmp_eles.end(); ++jt) {

        if (ispair) {
          if (kvf++ % 2 == 0) {
            key = jt->str.c_str();
          } else {
            kvs[key] = jt->str;
          }

        } else {
          log_.error("%s-->return value not pair addr:%lu uid:%ld session:%s type:%d,int:%ld,str:%s",
                     fun, addr, uid, session.c_str(), jt->type, jt->integer, jt->str.c_str());

        }
      }


    }

    

	}

	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
}

void OnlineCtrl::one_uid_session(long actor, const RedisRv &tmp, std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions)
{
    const char *fun = "OnlineCtrl::one_uid_session";

      if (tmp.type != REDIS_REPLY_ARRAY
          || tmp.element.size() != 3
          || tmp.element.at(0).type != REDIS_REPLY_INTEGER
          || tmp.element.at(1).type != REDIS_REPLY_STRING
          || tmp.element.at(2).type != REDIS_REPLY_ARRAY
          ) {
        log_.error("%s-->retrun sessions invalid format actor:%ld", fun, actor);
        return;
      }

      map< string, map<string, string> > &msessions =
        uids_sessions.insert(
                             pair< long, map< string, map<string, string> > >
                             (tmp.element.at(0).integer, map< string, map<string, string> >() )
                               ).first->second;
      map<string, string> &sessions = msessions.insert(
                                                       pair< string, map<string, string> >
                                                         (tmp.element.at(1).str, map<string, string>() )
                                                          ).first->second;

      const vector<RedisRv> &tmp_eles = tmp.element.at(2).element;

      bool ispair = true;
      if (tmp_eles.size() % 2 != 0) {
        ispair = false;
      }

      int kvf = 0;
      const char *key = NULL;
      for (vector<RedisRv>::const_iterator jt = tmp_eles.begin();
           jt != tmp_eles.end(); ++jt) {

        if (ispair) {
          if (kvf++ % 2 == 0) {
            key = jt->str.c_str();
          } else {
            sessions[key] = jt->str;
          }
        } else {
          log_.error("%s-->retrun sessions not pair actor:%ld", fun, actor);

        }

      }


}

void OnlineCtrl::get_multi(int timeout, long actor, const vector<long> &uids,
                           std::map< long, map< std::string, std::map<std::string, std::string> > > &uids_sessions
                           )
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_multi";
  string sactor = boost::lexical_cast<string>(actor);
  map< uint64_t, set<string> > disp_uids;

  for (vector<long>::const_iterator it = uids.begin();
       it != uids.end(); ++it) {
    long uid = *it;
    string suid = boost::lexical_cast<string>(uid);
    uint64_t rd_addr = rh_.redis_addr(suid);
    if (!rd_addr) {
      log_.error("%s-->acotor:%s error hash uid:%s", fun, sactor.c_str(), suid.c_str());
      continue;
    }

    disp_uids.insert(
                     pair< uint64_t, set<string> >(rd_addr, set<string>())
                     ).first->second.insert(suid);

  }

  log_.info("%s-->addr.size:%lu uids.size:%lu", fun, disp_uids.size(), uids.size());
  map< uint64_t, vector<string> > addr_cmd;

  for (map< uint64_t, set<string> >::const_iterator it = disp_uids.begin();
       it != disp_uids.end(); ++it) {
    vector<string> &args = addr_cmd.insert(pair< uint64_t, vector<string> >(it->first, vector<string>())).first->second;
    args.push_back("EVALSHA");
    args.push_back(s_multi_.sha1);
    args.push_back(boost::lexical_cast<string>(it->second.size()));

    for (set<string>::const_iterator jt = it->second.begin();
         jt != it->second.end(); jt++) {
      args.push_back(*jt);
    }

    log_.info("%s-->addr:%lu uids.size:%lu", fun, it->first, args.size()-3);
  }

  RedisRvs rv;
  re_.cmd(rv, sactor.c_str(), addr_cmd, timeout, s_multi_.data, false);
  // ================================
  for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;
    string saddr = fun + boost::lexical_cast<string>(addr);
    tmp.dump(&log_, saddr.c_str(), 0);

    if (tmp.type != REDIS_REPLY_ARRAY) {
      log_.error("%s-->retrun sessions invalid format actor:%ld addr:%lu", fun, actor, addr);
      continue;
    }



    const vector<RedisRv> &tmp_eles = tmp.element;
    for (vector<RedisRv>::const_iterator jt = tmp_eles.begin();
         jt != tmp_eles.end(); ++jt) {
      one_uid_session(actor, *jt, uids_sessions);      
    }


  }
  log_.info("%s-->actor:%ld tm:%ld", fun, actor, tu.intv());
}