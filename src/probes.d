/* Bitcoin DTrace (and SystemTap) provider */

provider bitcoin {
  probe flushcache(size_t);
};

#pragma D attributes Evolving/Evolving/Common provider bitcoin provider
#pragma D attributes Evolving/Evolving/Common provider bitcoin module
#pragma D attributes Evolving/Evolving/Common provider bitcoin function
#pragma D attributes Evolving/Evolving/Common provider bitcoin name
#pragma D attributes Evolving/Evolving/Common provider bitcoin args
