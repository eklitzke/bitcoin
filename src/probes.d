/* Bitcoin DTrace (and SystemTap) provider */

provider bitcoin {
  probe cache__flush_start(size_t coins, size_t num_bytes);
  probe cache__flush_end();
  probe cache__hit();
  probe cache__miss();
  probe finish__ibd();
  probe cdb__write__batch(size_t size, bool sync);
  probe update__tip(size_t cache_bytes, size_t utxo_entries);
};

#pragma D attributes Evolving/Evolving/Common provider bitcoin provider
#pragma D attributes Evolving/Evolving/Common provider bitcoin module
#pragma D attributes Evolving/Evolving/Common provider bitcoin function
#pragma D attributes Evolving/Evolving/Common provider bitcoin name
#pragma D attributes Evolving/Evolving/Common provider bitcoin args
