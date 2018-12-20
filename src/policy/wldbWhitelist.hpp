//The one and only  mongodb "whitelist" collection.
#pragma once

#include "wldbCollection.hpp"

class wldbWhitelist: public wldbCollection{
public:
  wldbWhitelist();
  ~wldbWhitelist();
  static wldbWhitelist* getInstance();

private:
  //boost::recursive_mutex _mtx;

  static wldbWhitelist* _instance;
};
