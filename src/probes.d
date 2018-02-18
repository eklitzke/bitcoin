/* Bitcoin DTrace (and SystemTap) provider */

provider bitcoin {
  probe cache__flush(size_t num_coins, size_t num_bytes);
  probe cache__hit();
  probe cache__miss();
  probe finish__ibd();
  probe update__tip(uint32_t height, size_t cache_size, size_t cache_bytes, size_t progress);
};

#pragma D attributes Evolving/Evolving/Common provider bitcoin provider
#pragma D attributes Evolving/Evolving/Common provider bitcoin module
#pragma D attributes Evolving/Evolving/Common provider bitcoin function
#pragma D attributes Evolving/Evolving/Common provider bitcoin name
#pragma D attributes Evolving/Evolving/Common provider bitcoin args
