/* -*- mode: dtrace-script; -*- */

provider bitcoin
{
    probe init_main(string datadir, string configpath);
    probe cache_flush(size_t num_coins, size_t num_bytes);
    probe cache_hit();
    probe cache_miss();
    probe finish_ibd();
    probe read_block_from_disk(int height, int filenum);
    probe update_tip(string block_hash, uint32_t height, size_t cache_size, size_t cache_bytes, size_t progress);
};

#pragma D attributes Evolving / Evolving / Common provider bitcoin provider
#pragma D attributes Evolving / Evolving / Common provider bitcoin module
#pragma D attributes Evolving / Evolving / Common provider bitcoin function
#pragma D attributes Evolving / Evolving / Common provider bitcoin name
#pragma D attributes Evolving / Evolving / Common provider bitcoin args
